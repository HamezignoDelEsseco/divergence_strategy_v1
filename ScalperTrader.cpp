#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_ScalperTraderSignal(SCStudyInterfaceRef sc) {

    SCInputRef NumberBarsStudy = sc.Input[0];
    SCSubgraphRef InstantMode = sc.Subgraph[0];
    SCSubgraphRef VolDiff = sc.Subgraph[1];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Scalper trader signal";

        NumberBarsStudy.Name = "Number bars study";
        NumberBarsStudy.SetStudyID(1);

        InstantMode.Name = "Instant mode";
        InstantMode.DrawStyle = DRAWSTYLE_LINE;

        VolDiff.Name = "Vol diff";
        VolDiff.DrawStyle = DRAWSTYLE_LINE;

    }
    float instantMode;

    const int currIdx = sc.Index;
    const int i = currIdx - 1;

    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);

    const bool conditionBuy = AskVBidV[i] > 0 && sc.High[currIdx] > sc.High[i];
    const bool conditionSell = AskVBidV[i] < 0 && sc.Low[currIdx] < sc.Low[i];

    if (!conditionBuy && !conditionSell) {
        instantMode = -2;
    }
    if (conditionBuy) {
        instantMode = 1;
    }

    if (conditionSell) {
        instantMode = -1;
    }

    InstantMode[currIdx] = instantMode;
    VolDiff[currIdx] = AskVBidV[currIdx];

}


SCSFExport scsf_ScalperTrader(SCStudyInterfaceRef sc) {

    SCInputRef ScalperTraderSignal = sc.Input[0];
    SCInputRef NumberBars = sc.Input[1];
    SCInputRef MaxTarget = sc.Input[2];
    SCInputRef LongQuantity = sc.Input[3];
    SCInputRef EndTradingTime = sc.Input[4];
    SCInputRef MaxDailyPLInTicks = sc.Input[5];
    SCInputRef MinDailyPLInTicks = sc.Input[6];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Scalper trader basic";

        // Inputs
        ScalperTraderSignal.Name = "ScalperSignal";
        ScalperTraderSignal.SetStudyID(2);

        MaxTarget.Name = "Max. target";
        MaxTarget.SetIntLimits(1, 200);
        MaxTarget.SetInt(40);

        LongQuantity.Name = "Long qty";
        LongQuantity.SetIntLimits(1, 2);
        LongQuantity.SetInt(1);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15,45,0));

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day"; // Total quantity that breaks even
        MaxDailyPLInTicks.SetIntLimits(10, 1000);
        MaxDailyPLInTicks.SetInt(300);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day"; // Total quantity that breaks even
        MinDailyPLInTicks.SetIntLimits(-1000, -10);
        MinDailyPLInTicks.SetInt(-100);

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = false;
        sc.AllowOnlyOneTradePerBar = true;

        sc.MaximumPositionAllowed = 3;

        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = true;
        sc.AllowEntryWithWorkingOrders = true;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

    const int i = sc.Index;
    int& messagePrinted = sc.GetPersistentInt(2);
    int& flattenBar = sc.GetPersistentInt(1);

    if (i == flattenBar) {
        return;
    }

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    if (
        (PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
            || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0
        ) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("You have exceeded your daily loss / profit", 1);
        }
        messagePrinted ++;

        return;
        }


    SCFloatArray AskVBidV;
    SCFloatArray TradeSignal;
    sc.GetStudyArrayUsingID(ScalperTraderSignal.GetStudyID(), 0, TradeSignal);
    sc.GetStudyArrayUsingID(ScalperTraderSignal.GetStudyID(), 1, AskVBidV);


    s_SCNewOrder newOrder;
    newOrder.OrderQuantity = 1;
    newOrder.TimeInForce = SCT_TIF_DAY;
    newOrder.OrderType = SCT_ORDERTYPE_MARKET;
    newOrder.Stop1Offset = sc.TickSize * 40;
    newOrder.Target1Offset = sc.TickSize * MaxTarget.GetFloat();
    newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_STOP_LIMIT;
    newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_STOP_LIMIT;


    if (TradeSignal[i] == 1 && PositionData.PositionQuantity >= 0) {
        sc.BuyOrder(newOrder);
        flattenBar = i;
        return;
    }

    if (TradeSignal[i] == -1 && PositionData.PositionQuantity <= 0) {
        sc.SellOrder(newOrder);
        flattenBar = i;
        return;

    }

    if (AskVBidV[i-1] < 0 &&  PositionData.PositionQuantity > 0h) {
        sc.FlattenAndCancelAllOrders();
        flattenBar = i;
        return;

    }

    if (AskVBidV[i-1] > 0 &&  PositionData.PositionQuantity < 0) {
        sc.FlattenAndCancelAllOrders();
        flattenBar = i;

    }

    //if (PositionData.OpenProfitLoss / sc.CurrencyValuePerTick >= 100) {
    //    sc.FlattenAndCancelAllOrders();
    //}
//
    //if (PositionData.OpenProfitLoss / sc.CurrencyValuePerTick + PositionData.DailyProfitLoss / sc.CurrencyValuePerTick>= 100) {
    //    sc.FlattenAndCancelAllOrders();
    //}
}