#include "sierrachart.h"

SCSFExport scsf_BigBarScalping(SCStudyInterfaceRef sc) {
    SCInputRef GoStudy = sc.Input[0];
    SCInputRef NumberOfStack = sc.Input[1];
    SCInputRef EndTradingTime = sc.Input[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Big bar scalper";

        GoStudy.Name = "Go study";
        GoStudy.SetStudyID(10);

        NumberOfStack.Name = "Max stacks";
        NumberOfStack.SetInt(0);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15,45,0));

        sc.GraphRegion = 1;

        // Flags
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 1;
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = true;


    }

    const int i = sc.Index;

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }
    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

    SCFloatArray Go;
    sc.GetStudyArrayUsingID(GoStudy.GetStudyID(), 4, Go);



    s_SCNewOrder NewOrder;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.OrderQuantity = 1;
    NewOrder.TimeInForce = SCT_TIF_DAY;

    if (Go[i] == 1){// && PositionData.PositionQuantity < NumberOfStack.GetFloat()) {
        sc.BuyOrder(NewOrder);
    }

    if (Go[i] == -1){// && PositionData.PositionQuantity < NumberOfStack.GetFloat()) {
        sc.SellOrder(NewOrder);
    }
}