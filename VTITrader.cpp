#include "sierrachart.h"

SCSFExport scsf_VTITrader(SCStudyInterfaceRef sc) {
    SCInputRef VTISignalStudy = sc.Input[0];
    SCInputRef EndTradingTime = sc.Input[1];
    SCInputRef StartTradingTime = sc.Input[2];

    SCInputRef MaxDailyPLInTicks = sc.Input[3];
    SCInputRef MinDailyPLInTicks = sc.Input[4];
    SCInputRef MaxTargetInTicks = sc.Input[5];
    SCInputRef MaxStopInTicks = sc.Input[6];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "VTI - Trader";

        VTISignalStudy.Name = "VTI signal";
        VTISignalStudy.SetStudyID(8);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(14,45,0));

        StartTradingTime.Name = "Start trading time";
        StartTradingTime.SetTime(HMS_TIME(7,0,0));

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day"; // Total quantity that breaks even
        MaxDailyPLInTicks.SetIntLimits(10, 1000);
        MaxDailyPLInTicks.SetInt(300);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day"; // Total quantity that breaks even
        MinDailyPLInTicks.SetIntLimits(-1000, -10);
        MinDailyPLInTicks.SetInt(-100);

        MaxTargetInTicks.Name = "Maximum target in ticks";
        MaxTargetInTicks.SetIntLimits(10, 1000);
        MaxTargetInTicks.SetInt(100);

        MaxStopInTicks.Name = "Maximum stop in ticks";
        MaxStopInTicks.SetIntLimits(5, 1000);
        MaxStopInTicks.SetInt(10);


        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;

        sc.MaximumPositionAllowed = 3;

        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = true;
        sc.AllowEntryWithWorkingOrders = true;

    }


    const int i = sc.Index;

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

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
        ){return;}

    SCFloatArray TradeSignal;
    SCFloatArray LimitPrice;

    sc.GetStudyArrayUsingID(VTISignalStudy.GetStudyID(),0, TradeSignal);
    sc.GetStudyArrayUsingID(VTISignalStudy.GetStudyID(),0, LimitPrice);


    s_SCNewOrder newOrder;
    newOrder.OrderQuantity = 1;
    newOrder.TimeInForce = SCT_TIF_DAY;
    newOrder.OrderType = SCT_ORDERTYPE_MARKET;
    newOrder.Stop1Offset = MaxStopInTicks.GetFloat();
    newOrder.Target1Offset = MaxTargetInTicks.GetFloat();
    newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_STOP;
    newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_STOP_LIMIT;

    if (TradeSignal[i] == -1) {
        sc.SellOrder(newOrder);
    }

    if (TradeSignal[i] == 1) {
        sc.BuyOrder(newOrder);
    }

}