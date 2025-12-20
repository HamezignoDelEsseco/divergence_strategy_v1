#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_MACDMomentumSignalAlexTrading(SCStudyInterfaceRef sc) {

    SCInputRef MACDMomentumSignalStudy = sc.Input[0];
    SCInputRef StopLoss = sc.Input[1];
    SCInputRef TakeProfit = sc.Input[2];
    SCInputRef StartTradingTime = sc.Input[3];
    SCInputRef EndTradingTime = sc.Input[4];
    SCInputRef MaxDailyPLInTicks = sc.Input[5];
    SCInputRef MinDailyPLInTicks = sc.Input[6];
    SCInputRef FlattenOnZeroSignal = sc.Input[7];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "MACD Momentum Signal Alex Trading";

        // Inputs
        MACDMomentumSignalStudy.Name = "MACD Momentum Signal Study";
        MACDMomentumSignalStudy.SetStudyID(1);

        StopLoss.Name = "Stop Loss (ticks)";
        StopLoss.SetIntLimits(1, 500);
        StopLoss.SetInt(100);

        TakeProfit.Name = "Take Profit (ticks)";
        TakeProfit.SetIntLimits(1, 500);
        TakeProfit.SetInt(100);

        StartTradingTime.Name = "Start trading time";
        StartTradingTime.SetTime(HMS_TIME(8, 0, 0));

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15, 45, 0));

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day";
        MaxDailyPLInTicks.SetIntLimits(10, 1000);
        MaxDailyPLInTicks.SetInt(300);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day";
        MinDailyPLInTicks.SetIntLimits(-1000, -10);
        MinDailyPLInTicks.SetInt(-100);

        FlattenOnZeroSignal.Name = "Flatten Position When Signal = 0";
        FlattenOnZeroSignal.SetYesNo(0);
        FlattenOnZeroSignal.SetDescription("Yes = Exit when signal goes to 0, No = Exit only on SL/TP");

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 1;
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = true;
        sc.AllowEntryWithWorkingOrders = true;

        return;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize - 1)
        return;

    const int i = sc.Index;
    int& messagePrinted = sc.GetPersistentInt(1);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    // Check if within trading hours
    SCDateTime currentTime = sc.BaseDateTimeIn[i].GetTime();

    // Don't trade before start time
    if (currentTime < StartTradingTime.GetTime()) {
        return;
    }

    // Stop trading after end time and flatten any open positions
    if (currentTime > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    // Check daily P/L limits
    if (
        (PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
            || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0
        ) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("You have exceeded your daily loss / profit", 1);
        }
        messagePrinted++;
        return;
    }

    // Get the signal from MACD Momentum Signal study (Subgraph 0 = Signal)
    SCFloatArray Signal;
    sc.GetStudyArrayUsingID(MACDMomentumSignalStudy.GetStudyID(), 0, Signal);

    // Setup new order with SL and TP
    s_SCNewOrder newOrder;
    newOrder.OrderQuantity = 1;
    newOrder.TimeInForce = SCT_TIF_DAY;
    newOrder.OrderType = SCT_ORDERTYPE_MARKET;
    newOrder.Stop1Offset = sc.TickSize * StopLoss.GetInt();
    newOrder.Target1Offset = sc.TickSize * TakeProfit.GetInt();
    newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_STOP_LIMIT;
    newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_LIMIT;

    // Signal = 0 (Neutral): Behavior based on FlattenOnZeroSignal parameter
    if (Signal[i-1] == 0) {
        if (FlattenOnZeroSignal.GetYesNo() && PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    // Signal = 1 (Long): Go long or add to long position
    if (Signal[i-1] == 1) {
        if (PositionData.PositionQuantity < 0) {
            sc.FlattenAndCancelAllOrders();
        }
        sc.BuyOrder(newOrder);
        return;
    }

    // Signal = -1 (Short): Go short or add to short position
    if (Signal[i-1] == -1) {
        if (PositionData.PositionQuantity > 0) {
            sc.FlattenAndCancelAllOrders();
        }
        sc.SellOrder(newOrder);
        return;
    }
}
