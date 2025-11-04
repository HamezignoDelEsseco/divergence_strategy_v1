#include "sierrachart.h"
#include "helpers.h"

SCSFExport scsf_ColorBreaksColorWithSlopes(SCStudyInterfaceRef sc) {

    SCInputRef NumberBarsStudy = sc.Input[0];
    SCInputRef SlopesStudy = sc.Input[1];

    SCInputRef FilterByLocalOpt = sc.Input[2];
    SCInputRef LocalOptBars = sc.Input[3];
    SCInputRef LocalOptOffsetInTicks = sc.Input[4];
    SCInputRef RiskTickOffset = sc.Input[5];



    SCSubgraphRef TradeSignalEOS = sc.Subgraph[0];
    SCSubgraphRef TradeSignalSlopes = sc.Subgraph[1];
    SCSubgraphRef CombinedSignal = sc.Subgraph[2];

    SCSubgraphRef StopLoss = sc.Subgraph[3];
    SCSubgraphRef Go = sc.Subgraph[4];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Color breaks color with slopes";

        NumberBarsStudy.Name = "Number bars calculated values study";
        NumberBarsStudy.SetStudyID(6);

        SlopesStudy.Name = "Slopes study";
        SlopesStudy.SetStudyID(9);

        FilterByLocalOpt.Name = "Filter by local opt";
        FilterByLocalOpt.SetYesNo(1);

        LocalOptBars.Name = "Local opt of N bars";
        LocalOptBars.SetIntLimits(1, 100);
        LocalOptBars.SetInt(10);

        LocalOptOffsetInTicks.Name = "Buffer in ticks to local opt";
        LocalOptOffsetInTicks.SetIntLimits(0, 100);
        LocalOptOffsetInTicks.SetInt(5);

        RiskTickOffset.Name = "Stop limit offset in ticks";
        RiskTickOffset.SetIntLimits(0, 100);
        RiskTickOffset.SetInt(5);

        TradeSignalEOS.Name = "EOS signal";
        TradeSignalEOS.DrawStyle = DRAWSTYLE_LINE;

        TradeSignalSlopes.Name = "Slopes signal";
        TradeSignalSlopes.DrawStyle = DRAWSTYLE_LINE;

        CombinedSignal.Name = "Combined signal";
        CombinedSignal.DrawStyle = DRAWSTYLE_LINE;

        StopLoss.Name = "Stop loss";
        StopLoss.DrawStyle = DRAWSTYLE_IGNORE;

        Go.Name = "GO !!!!";
        Go.DrawStyle = DRAWSTYLE_LINE;

        sc.GraphRegion = 1;

    }
    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);

    SCFloatArray SlopeHigh;
    SCFloatArray SlopeLow;
    sc.GetStudyArrayUsingID(SlopesStudy.GetStudyID(), 0, SlopeLow);
    sc.GetStudyArrayUsingID(SlopesStudy.GetStudyID(), 1, SlopeHigh);

    const int i = sc.Index;
    bool localLow = true;
    bool localHigh = true;

    float& stopLoss = sc.GetPersistentFloat(1);
    float& eosSignal = sc.GetPersistentFloat(2);
    float& slopesSignal = sc.GetPersistentFloat(3);

    if (sc.IsNewTradingDay(i) || i <= 0) {
        stopLoss = 0;
        eosSignal = 0;
        slopesSignal = 0;
    }

    if (FilterByLocalOpt.GetYesNo() == 1) {
        localLow = lowestOfNBarsWithBuffer(sc, LocalOptBars.GetInt(), i-2, LocalOptOffsetInTicks.GetFloat() * sc.TickSize);
        localHigh = highestOfNBarsWithBuffer(sc, LocalOptBars.GetInt(), i-2, LocalOptOffsetInTicks.GetFloat() * sc.TickSize);

    }

    if (
        AskVBidV[i-2] > 0 && AskVBidV[i-1] < 0
        && (
            (sc.Low[i] < sc.Low[i-1] && localHigh && IsCleanTick(sc.Low[i-1] - sc.TickSize, sc, 0))
            || sc.Low[i] < (sc.Low[i-1] - sc.TickSize && localHigh)
        )
        //&& SlopeLow[i] <= 0 && SlopeHigh[i] <= 0

    ) {
        eosSignal = -1;
        //TradeSignalEOS[i] = - 1;
        stopLoss = sc.High[i-1] + RiskTickOffset.GetFloat() * sc.TickSize;
    }


    if (
        AskVBidV[i-2] < 0  && AskVBidV[i-1] > 0
        && (
            (sc.High[i] > sc.High[i-1] && localLow && IsCleanTick(sc.High[i-1] + sc.TickSize, sc, 0))
            || (sc.High[i] > sc.High[i-1] + sc.TickSize && localLow)
        )
        //&& SlopeLow[i] >= 0 && SlopeHigh[i] >= 0
    ) {
        eosSignal = 1;

        //TradeSignalEOS[i] = 1;
        stopLoss = sc.Low[i-1] - RiskTickOffset.GetFloat() * sc.TickSize;
    }

    //if (TradeSignal[i] == 0) {
    //    TradeSignal[i] = TradeSignal[i-1];
    //}
//
    //if (TradeSignal[i] == 1 && SlopeLow[i] <= 0 && SlopeHigh[i] <= 0) {
    //    TradeSignal[i] = 0;
    //}
//
    //if (TradeSignal[i] == -1 && SlopeLow[i] >= 0 && SlopeHigh[i] >= 0) {
    //    TradeSignal[i] = 0;
    //}
    slopesSignal = SlopeLow[i] < 0 && SlopeHigh[i] < 0 ? -1 : SlopeLow[i] > 0 && SlopeHigh[i] > 0 ? 1 : 0;
    StopLoss[i] = stopLoss;
    TradeSignalSlopes[i] = slopesSignal;
    TradeSignalEOS[i] = eosSignal;
    CombinedSignal[i] = slopesSignal <0 &&  eosSignal<0? -1: slopesSignal >0 &&  eosSignal>0 ? 1 : 0;
    Go[i] = CombinedSignal[i] != CombinedSignal[i-1] ? CombinedSignal[i] : 0;
}


SCSFExport scsf_SwingTrader(SCStudyInterfaceRef sc) {
    SCInputRef GoStudy = sc.Input[0];
    SCInputRef NumberOfStack = sc.Input[1];
    SCInputRef EndTradingTime = sc.Input[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Swing trader";

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