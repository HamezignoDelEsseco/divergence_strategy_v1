#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_VVAZoneId(SCStudyInterfaceRef sc) {

    SCInputRef VVAStudy = sc.Input[0];

    SCSubgraphRef ZoneID = sc.Subgraph[0];
    //SCSubgraphRef ContradictingIdx = sc.Subgraph[4];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "VVA Zone ID";

        VVAStudy.Name = "VVA ZoneID";
        VVAStudy.SetStudyID(6);

        ZoneID.Name = "Zone ID";
        ZoneID.DrawStyle = DRAWSTYLE_IGNORE;
    }
    SCFloatArray VVAH;
    SCFloatArray VVAL;
    sc.GetStudyArrayUsingID(VVAStudy.GetStudyID(), 1, VVAH);
    sc.GetStudyArrayUsingID(VVAStudy.GetStudyID(), 2, VVAL);

    const int i = sc.Index;

    float& zoneID = sc.GetPersistentFloat(1);
    if (sc.IsNewTradingDay(i)) {
        zoneID = 0;
    }
    if (sc.CrossOver(sc.Close, VVAH, i) != NO_CROSS || sc.CrossOver(sc.Close, VVAL, i) != NO_CROSS) {
        zoneID++;
    }

    ZoneID[i] = zoneID;

}

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
    SCSubgraphRef Reversal = sc.Subgraph[4];

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


        TargetFirst.Name = "Target first (in ticks)";
        TargetFirst.DrawStyle = DRAWSTYLE_IGNORE;

        MITEntry.Name = "MIT entry price";
        MITEntry.DrawStyle = DRAWSTYLE_IGNORE;

        Reversal.Name = "Reversal signal";
        Reversal.DrawStyle = DRAWSTYLE_LINE;
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

    // The reversal condition is an IMMEDIATE switch
    const bool reverseToShort = TradeSignal[i] == 1 && AskVBidV[i] < 0 && sc.Close[i] - sc.TickSize <= sc.Low[i-1];
    const bool reverseToLong = TradeSignal[i] == -1 && AskVBidV[i] > 0 && sc.Close[i] + sc.TickSize >= sc.High[i-1];

    if (reverseToLong) {
        Reversal[i] = 1;
    }
    if (reverseToShort) {
        Reversal[i] = -1;
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



SCSFExport scsf_DeltaVolLongTrader(SCStudyInterfaceRef sc) {

    SCInputRef SignalStudy = sc.Input[0];
    SCInputRef MaxTargetInTicks = sc.Input[1];
    SCInputRef StopTriggerOffsetInTicks = sc.Input[2];
    SCInputRef StopTrailOffsetInTicks = sc.Input[3];
    SCInputRef NumBarsBeforeCancelOrder = sc.Input[4];
    SCInputRef EndTradingTime = sc.Input[5];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume trader dummy (long)";

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


    if (TradeSignal[i] == 1 && !isInsideTrade(sc)) {
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

SCSFExport scsf_DeltaVolLongTrader3OCO(SCStudyInterfaceRef sc) {

    SCInputRef SignalStudy = sc.Input[0];
    SCInputRef MaxTargetInTicksForLongPos = sc.Input[1];
    SCInputRef NumBarsBeforeCancelOrder = sc.Input[2];
    SCInputRef EndTradingTime = sc.Input[3];
    SCInputRef Quantity = sc.Input[4];
    SCInputRef QuantityBE = sc.Input[5];
    SCInputRef QuantityLong = sc.Input[6];
    SCInputRef MaxDailyPLInTicks = sc.Input[7];
    SCInputRef MinDailyPLInTicks = sc.Input[8];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume trader with 3 OCO (long)";

        // Inputs
        SignalStudy.Name = "Signal study";
        SignalStudy.SetStudyID(2);

        MaxTargetInTicksForLongPos.Name = "Maximum target in ticks for long contract";
        MaxTargetInTicksForLongPos.SetIntLimits(5, 500);
        MaxTargetInTicksForLongPos.SetInt(100);

        NumBarsBeforeCancelOrder.Name = "Number of bars till order cancel";
        NumBarsBeforeCancelOrder.SetIntLimits(1, 20);
        NumBarsBeforeCancelOrder.SetInt(5);

        Quantity.Name = "Quantity mult"; // Total trade quantity
        Quantity.SetIntLimits(1, 5);
        Quantity.SetInt(2);

        QuantityLong.Name = "Quantity Long"; // Total trade quantity
        QuantityLong.SetIntLimits(1, 5);
        QuantityLong.SetInt(1);

        QuantityBE.Name = "Quantity BE"; // Total quantity that breaks even
        QuantityBE.SetIntLimits(1, 5);
        QuantityBE.SetInt(3);

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day"; // Total quantity that breaks even
        MaxDailyPLInTicks.SetIntLimits(10, 250);
        MaxDailyPLInTicks.SetInt(200);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day"; // Total quantity that breaks even
        MinDailyPLInTicks.SetIntLimits(-150, -10);
        MinDailyPLInTicks.SetInt(-100);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15,45,0));

        sc.HideStudy = true;

        // Outputs
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 10;
        sc.MaintainTradeStatisticsAndTradesData = true;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

    const int i = sc.Index;


    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    if (
        (PositionData.DailyProfitLoss >= MaxDailyPLInTicks.GetInt() || PositionData.DailyProfitLoss <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity ==0
        ) {
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

    if (TradeSignal[i] == 1 && PositionData.PositionQuantity == 0) {
        // if (i - barOderPlaced >= NumBarsBeforeCancelOrder.GetInt()) {
        //     sc.FlattenAndCancelAllOrders();
        // }
        s_SCNewOrder newOrder;

        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.TimeInForce = SCT_TIF_DAY;
        newOrder.OrderQuantity = QuantityBE.GetInt() + Quantity.GetInt() + QuantityLong.GetInt();
        newOrder.OCOGroup1Quantity = QuantityBE.GetInt();
        newOrder.OCOGroup2Quantity = Quantity.GetInt();
        newOrder.OCOGroup3Quantity = QuantityLong.GetInt();

        newOrder.Stop1Price = StopLossPrice[i] - sc.TickSize;
        newOrder.Stop2Price = StopLossPrice[i] - sc.TickSize;
        newOrder.Stop3Price = StopLossPrice[i] - sc.TickSize;

        newOrder.Target1Offset = TargetFirstInTicks[i] * sc.TickSize;
        newOrder.Target2Offset = TargetFirstInTicks[i] * sc.TickSize * 2;
        newOrder.Target3Offset = MaxTargetInTicksForLongPos.GetFloat() * sc.TickSize;

        newOrder.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP;
        newOrder.AttachedOrderStop2Type = SCT_ORDERTYPE_STOP;
        newOrder.AttachedOrderStop3Type = SCT_ORDERTYPE_STOP;

        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_MARKET_IF_TOUCHED;
        newOrder.AttachedOrderTarget2Type = SCT_ORDERTYPE_MARKET_IF_TOUCHED;
        newOrder.AttachedOrderTarget3Type = SCT_ORDERTYPE_MARKET_IF_TOUCHED;

        newOrder.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        newOrder.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 10;
        newOrder.MoveToBreakEven.TriggerOffsetInTicks = 25;

        if (const double orderPlaced = sc.BuyEntry(newOrder); orderPlaced > 0) {
            barOderPlaced = i;
        }

    }
}