#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_DeltaVolShortSignal(SCStudyInterfaceRef sc) {

    SCInputRef NumPreceedingPosBars = sc.Input[0];
    SCInputRef MaxEntryBarTickSize = sc.Input[1];
    SCInputRef HighestOfNBars = sc.Input[2];
    SCInputRef ContradictingSignalBar = sc.Input[3];
    SCInputRef NumberBarsStudy = sc.Input[4];

    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef Stop = sc.Subgraph[1];
    SCSubgraphRef TargetFirst = sc.Subgraph[2];
    SCSubgraphRef MITEntry = sc.Subgraph[3];
    //SCSubgraphRef ContradictingIdx = sc.Subgraph[4];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume short signal";

        // The Study
        NumPreceedingPosBars.Name = "Number of preceeding bars with pos delta";
        NumPreceedingPosBars.SetIntLimits(0, 10);
        NumPreceedingPosBars.SetInt(0);

        MaxEntryBarTickSize.Name = "Maximum tick size of entry bar";
        MaxEntryBarTickSize.SetIntLimits(0, 30);
        MaxEntryBarTickSize.SetInt(30);

        HighestOfNBars.Name = "Highest of N bars";
        HighestOfNBars.SetIntLimits(0, 30);
        HighestOfNBars.SetInt(5);

        ContradictingSignalBar.Name = "Number of bars since contradicting signal (0 = deactivated)";
        ContradictingSignalBar.SetIntLimits(0, 20);
        ContradictingSignalBar.SetInt(0);

        NumberBarsStudy.Name = "Number bars calculated values study";
        NumberBarsStudy.SetStudyID(6);

        TradeSignal.Name = "Enter short signal";
        TradeSignal.DrawStyle = DRAWSTYLE_IGNORE;

        Stop.Name = "Stop loss price";
        Stop.DrawStyle = DRAWSTYLE_IGNORE;

        TargetFirst.Name = "Target first (in ticks)";
        TargetFirst.DrawStyle = DRAWSTYLE_LINE;

        MITEntry.Name = "MIT entry price";
        MITEntry.DrawStyle = DRAWSTYLE_LINE;

        //ContradictingIdx.Name = "Contradicting signal index";
        //ContradictingIdx.DrawStyle = DRAWSTYLE_LINE;
    }
    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);
    const int i = sc.Index;
    bool contradictingSignalCondition;

    int& cumPosDeltaVol = sc.GetPersistentInt(1);
    int& contradictingSignalIdx = sc.GetPersistentInt(2);

    double& stopLoss = sc.GetPersistentDouble(1);
    double& targetFirst = sc.GetPersistentDouble(2);
    double& mitEntryPrice = sc.GetPersistentDouble(3);

    if (AskVBidV[i-2] > 0) {
        cumPosDeltaVol++;
    } else{cumPosDeltaVol=0;}

    // Building the contradicting signal
    const bool lowestCondition = lowestOfNBars(sc, HighestOfNBars.GetInt(), i-1);
    const bool positiveDeltaCondiction = AskVBidV[i-1] > 0;
    if (lowestCondition && positiveDeltaCondiction) {contradictingSignalIdx=i;}

    if (ContradictingSignalBar.GetInt() == 0) {
        contradictingSignalCondition=true;
    } else {
        contradictingSignalCondition = i - contradictingSignalIdx > ContradictingSignalBar.GetInt();
    }

    // We assume the chart has well calibrated delta volume bars for the symbol
    const bool preceedingCondition = cumPosDeltaVol >= NumPreceedingPosBars.GetInt();
    const bool negDeltaVolCondition = AskVBidV[i-1] < 0;
    const bool highestCondition = highestOfNBars(sc, HighestOfNBars.GetInt(), i-1);
    const bool barSizeCondition = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize <= MaxEntryBarTickSize.GetFloat();
    const bool downTickCondition = sc.Low[i] < sc.Low[i-1];

    if (highestCondition & preceedingCondition & negDeltaVolCondition & barSizeCondition & downTickCondition & contradictingSignalCondition) {
        TradeSignal[i] = 1;
    }
    if (TradeSignal[i] == 1) {
        stopLoss = sc.High[i-1] + sc.TickSize; // Small buffer for greed :D
        targetFirst = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize;
        mitEntryPrice = sc.Low[i-1] - sc.TickSize;
    }
    Stop[i] = static_cast<float>(stopLoss);
    TargetFirst[i] = static_cast<float>(targetFirst);
    MITEntry[i] = static_cast<float>(mitEntryPrice);
}

SCSFExport scsf_DeltaVolLongShortSignal(SCStudyInterfaceRef sc) {

    SCInputRef NumPreceedingPosBars = sc.Input[0];
    SCInputRef MaxEntryBarTickSize = sc.Input[1];
    SCInputRef HighestOfNBars = sc.Input[2];
    SCInputRef NumberBarsStudy = sc.Input[3];

    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef Stop = sc.Subgraph[1];
    SCSubgraphRef TargetFirst = sc.Subgraph[2];
    SCSubgraphRef MITEntry = sc.Subgraph[3];
    //SCSubgraphRef ContradictingIdx = sc.Subgraph[4];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume long short signal";

        // The Study
        NumPreceedingPosBars.Name = "Number of preceeding bars with pos/neg delta";
        NumPreceedingPosBars.SetIntLimits(0, 10);
        NumPreceedingPosBars.SetInt(0);

        MaxEntryBarTickSize.Name = "Maximum tick size of entry bar";
        MaxEntryBarTickSize.SetIntLimits(0, 30);
        MaxEntryBarTickSize.SetInt(30);

        HighestOfNBars.Name = "Highest of N bars";
        HighestOfNBars.SetIntLimits(0, 30);
        HighestOfNBars.SetInt(5);

        NumberBarsStudy.Name = "Number bars calculated values study";
        NumberBarsStudy.SetStudyID(6);

        TradeSignal.Name = "Enter short signal";
        TradeSignal.DrawStyle = DRAWSTYLE_IGNORE;

        Stop.Name = "Stop loss price";
        Stop.DrawStyle = DRAWSTYLE_IGNORE;

        TargetFirst.Name = "Target first (in ticks)";
        TargetFirst.DrawStyle = DRAWSTYLE_LINE;

        MITEntry.Name = "MIT entry price";
        MITEntry.DrawStyle = DRAWSTYLE_LINE;
    }
    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);
    const int i = sc.Index;

    int& cumPosDeltaVol = sc.GetPersistentInt(1);
    int& cumNegDeltaVol = sc.GetPersistentInt(2);

    double& stopLoss = sc.GetPersistentDouble(1);
    double& targetFirstInTickOffset = sc.GetPersistentDouble(2);
    double& mitEntryPrice = sc.GetPersistentDouble(3);

    if (AskVBidV[i-2] > 0) {
        cumPosDeltaVol++;
    } else{cumPosDeltaVol=0;}

    if (AskVBidV[i-2] < 0) {
        cumNegDeltaVol++;
    } else{cumNegDeltaVol=0;}


    // We assume the chart has well calibrated delta volume bars for the symbol
    const bool preceedingConditionShort = cumPosDeltaVol >= NumPreceedingPosBars.GetInt();
    const bool preceedingConditionLong = cumNegDeltaVol >= NumPreceedingPosBars.GetInt();

    const bool negDeltaVolConditionShort = AskVBidV[i-1] < 0;
    const bool negDeltaVolConditionLong = AskVBidV[i-1] > 0;

    const bool highestConditionShort = highestOfNBars(sc, HighestOfNBars.GetInt(), i-1);
    const bool highestConditionLong = lowestOfNBars(sc, HighestOfNBars.GetInt(), i-1);

    const bool downTickConditionShort = sc.Low[i] < sc.Low[i-1];
    const bool downTickConditionLong = sc.High[i] > sc.High[i-1];

    const bool barSizeCondition = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize <= MaxEntryBarTickSize.GetFloat();

    if (highestConditionShort & preceedingConditionShort & negDeltaVolConditionShort & barSizeCondition & downTickConditionShort) {
        TradeSignal[i] = -1;
    }
    if (highestConditionLong & preceedingConditionLong & negDeltaVolConditionLong & barSizeCondition & downTickConditionLong) {
        TradeSignal[i] = 1;

    }
    if (TradeSignal[i] == -1) {
        stopLoss = sc.High[i-1] + sc.TickSize; // Small buffer for greed :D
        targetFirstInTickOffset = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize;
        mitEntryPrice = sc.Low[i-1] + sc.TickSize;
    }

    if (TradeSignal[i] == 1) {
        stopLoss = sc.Low[i-1] - sc.TickSize;
        targetFirstInTickOffset = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize;
        mitEntryPrice = sc.High[i-1] - sc.TickSize;
    }

    Stop[i] = static_cast<float>(stopLoss);
    TargetFirst[i] = static_cast<float>(targetFirstInTickOffset);
    MITEntry[i] = static_cast<float>(mitEntryPrice);
}


/*
SCSFExport scsf_DeltaVolTbT(SCStudyInterfaceRef sc) {
    SCInputRef DeltaTargetSinceSwing = sc.Input[0];

    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef StopPriceShort = sc.Subgraph[1];
    SCSubgraphRef StopPriceLong = sc.Subgraph[2];
    SCSubgraphRef SignalID = sc.Subgraph[3];

    //SCSubgraphRef ContradictingIdx = sc.Subgraph[4];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume TbT";

        // The Study
        DeltaTargetSinceSwing.Name = "Delta volume we're looking for entry";
        DeltaTargetSinceSwing.SetIntLimits(20, 1000);
        DeltaTargetSinceSwing.SetInt(100);

        TradeSignal.Name = "Trade signal";
        TradeSignal.DrawStyle = DRAWSTYLE_IGNORE;

        StopPriceShort.Name = "Stop loss price short";
        StopPriceShort.DrawStyle = DRAWSTYLE_IGNORE;

        StopPriceLong.Name = "Stop loss price long";
        StopPriceLong.DrawStyle = DRAWSTYLE_IGNORE;

        StopPriceLong.Name = "Stop loss price long";
        StopPriceLong.DrawStyle = DRAWSTYLE_IGNORE;
    }
    const int i = sc.Index;
    double& priceAtMin = sc.GetPersistentDouble(1);
    double& priceAtMax = sc.GetPersistentDouble(2);
    double& dvLocalMax = sc.GetPersistentDouble(3);
    double& dvLocalMin = sc.GetPersistentDouble(4);
    double& dv = sc.GetPersistentDouble(6);

    int& tradeDirection = sc.GetPersistentInt(1);
    int& signalId = sc.GetPersistentInt(2);


    if (sc.IsNewTradingDay(i) || i <= 0) {
        priceAtMin = 0;
        dvLocalMax = 0;
        dvLocalMin = 0;
        tradeDirection = 0;
    }

    const double prevMin = dvLocalMin;
    const double prevMax = dvLocalMax;

    dv += sc.BidVolume[i] - sc.AskVolume[i];

    dvLocalMin = std::min<double>(dv, dvLocalMin);
    dvLocalMax = std::max<double>(dv, dvLocalMax);

    // Trade signal trigger for long
    if (dv - dvLocalMin >= DeltaTargetSinceSwing.GetDouble()) {
        priceAtMin = dvLocalMin < prevMin ? sc.Close[i] : priceAtMin;
        tradeDirection = 1;
        signalId++;

        dv = 0;
        dvLocalMin = 0;
        dvLocalMax = 0;
    }

    // Trade signal trigger for short
    if (dvLocalMax - dv >= DeltaTargetSinceSwing.GetDouble()) {
        priceAtMax = dvLocalMax > prevMax ? sc.Close[i] : priceAtMax;
        tradeDirection = -1;
        signalId++;

        dv = 0;
        dvLocalMin = 0;
        dvLocalMax = 0;
    }

    StopPriceShort[i] = priceAtMax;
    StopPriceLong[i] = priceAtMin;
    TradeSignal[i] = tradeDirection;
    SignalID[i] = signalId;
} */



SCSFExport scsf_DeltaVolShortTrader(SCStudyInterfaceRef sc) {

    // Trading with max 5 contracts:

    SCInputRef SignalStudy = sc.Input[0];
    SCInputRef MaxTargetInTicks = sc.Input[1];
    SCInputRef StopTriggerOffsetInTicks = sc.Input[2];
    SCInputRef StopTrailOffsetInTicks = sc.Input[3];
    SCInputRef NumBarsBeforeCancelOrder = sc.Input[4];
    SCInputRef EndTradingTime = sc.Input[5];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume trader 1 contract";

        // Inputs
        SignalStudy.Name = "Signal study";
        SignalStudy.SetStudyID(2);

        MaxTargetInTicks.Name = "Maximum target in ticks";
        MaxTargetInTicks.SetIntLimits(5, 500);
        MaxTargetInTicks.SetInt(30);

        StopTriggerOffsetInTicks.Name = "Stop trigger offset in ticks";
        StopTriggerOffsetInTicks.SetIntLimits(5, 500);
        StopTriggerOffsetInTicks.SetInt(30);

        StopTrailOffsetInTicks.Name = "Stop trail offset in ticks";
        StopTrailOffsetInTicks.SetIntLimits(5, 500);
        StopTrailOffsetInTicks.SetInt(30);

        NumBarsBeforeCancelOrder.Name = "Number of bars till order cancel";
        NumBarsBeforeCancelOrder.SetIntLimits(1, 20);
        NumBarsBeforeCancelOrder.SetInt(5);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(16,55,0));

        sc.HideStudy = true;

        // Outputs
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 1;
        sc.MaintainTradeStatisticsAndTradesData = true;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

    const int i = sc.Index;

    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        sc.FlattenAndCancelAllOrders();
        return;
    }


    int& barOderPlaced = sc.GetPersistentInt(1);

    SCFloatArray TradeSignal;
    SCFloatArray StopLossPrice;
    SCFloatArray TargetFirstInTicks;
    SCFloatArray EntryPrice;

    sc.GetStudyArrayUsingID(SignalStudy.GetStudyID(), 0, TradeSignal);
    sc.GetStudyArrayUsingID(SignalStudy.GetStudyID(), 1, StopLossPrice);
    sc.GetStudyArrayUsingID(SignalStudy.GetStudyID(), 2, TargetFirstInTicks);
    sc.GetStudyArrayUsingID(SignalStudy.GetStudyID(), 3, EntryPrice);

    //uint32_t workingParentOrderId = 0;
    //const int hasWorkingParent = workingParentOrder(sc, workingParentOrderId);

    if (TradeSignal[i] == 1 && !isInsideTrade(sc)) {
        // if (i - barOderPlaced >= NumBarsBeforeCancelOrder.GetInt()) {
        //     sc.FlattenAndCancelAllOrders();
        // }
        s_SCNewOrder newOrder;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.TimeInForce = SCT_TIF_GOOD_TILL_CANCELED;
        newOrder.OrderQuantity = 1;
        // newOrder.Price1 = EntryPrice[i];
        newOrder.Target1Offset = MaxTargetInTicks.GetFloat() * sc.TickSize;
        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_MARKET_IF_TOUCHED;

        // The "3" means all the definitions of the stop are expressed in terms of offset
        newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_TRIGGERED_TRAILING_STOP_LIMIT_3_OFFSETS;
        newOrder.StopAllOffset = TargetFirstInTicks[i] * sc.TickSize;
        newOrder.TriggeredTrailStopTriggerPriceOffset = StopTriggerOffsetInTicks.GetFloat() * sc.TickSize;
        newOrder.TriggeredTrailStopTrailPriceOffset = TargetFirstInTicks[i] * sc.TickSize;

        if (const double orderPlaced = sc.BuyEntry(newOrder); orderPlaced > 0) {
            barOderPlaced = i;
        }

    }
}