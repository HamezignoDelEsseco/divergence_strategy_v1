/*
 * We build pairs of studies:
 * 1. The trading SIGNAL (that can depend on other custom or native studies)
 * 2. The trading EXECUTION (that must depend on the corresponding SIGNAL)
 * Naming conventions are Strategy<NAME><TYPE>Signal and Strategy<NAME>Exec
 * As you can see, different strategies can be tied to the same SIGNAL, they must be differentiated by a different TYPE
 * in the naming
 * Anything that's related to execution, such as the price to target, the stops and limits etc. are defined in the
 * EXECUTION
 * The signal can hoewever pass information into the executor when needed
 */

#include "helpers.h"
#include "sierrachart.h"

SCDLLName("TRADE DIVERGENCE MODE 1")

SCSFExport scsf_StrategyBasicFlagDraft(SCStudyInterfaceRef sc) {
    /*
     * This indicator is stating whether a trade should be CONSIDERED. It is NOT there
     *
     */
    SCInputRef BidAskDiffStudy = sc.Input[0];
    SCInputRef RangeBarPredictors = sc.Input[1];
    SCInputRef MinCleanTicks = sc.Input[2];
    SCInputRef BuyThreshold = sc.Input[3];
    SCInputRef SellThreshold = sc.Input[4];

    SCSubgraphRef TradeSignal = sc.Subgraph[0];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Divergence trading indicator";

        // The Study
        BidAskDiffStudy.Name = "Bid ask diff study";
        BidAskDiffStudy.SetStudyID(0);

        RangeBarPredictors.Name = "Range bar predictor study";
        RangeBarPredictors.SetStudyID(0);

        MinCleanTicks.Name = "Minimum printed clean ticks";
        MinCleanTicks.SetIntLimits(1, 5);
        MinCleanTicks.SetInt(2);

        BuyThreshold.Name = "Threshold for buy";
        BuyThreshold.SetIntLimits(-5000, -1);
        BuyThreshold.SetInt(-100);

        SellThreshold.Name = "Threshold for sell";
        SellThreshold.SetIntLimits(1, 5000);
        SellThreshold.SetInt(100);

        TradeSignal.Name = "Enter signal";
    }

    // Retrieving Studies
    SCFloatArray BidAskDiff;
    SCFloatArray TopBarPredictor;
    SCFloatArray LowBarPredictor;
    sc.GetStudyArrayUsingID(BidAskDiffStudy.GetStudyID(), 0, BidAskDiff);
    sc.GetStudyArrayUsingID(RangeBarPredictors.GetStudyID(), 0, TopBarPredictor);
    sc.GetStudyArrayUsingID(RangeBarPredictors.GetStudyID(), 1, LowBarPredictor);

    const float prevHigh = sc.High[sc.Index - 1];
    const float prevLow = sc.Low[sc.Index - 1];
    const float open = sc.Open[sc.Index];
    const bool isDown = open <= prevLow;

    float priceOfInterest;

    if (
        isDown
        && BidAskDiff[sc.Index - 1] >= SellThreshold.GetFloat()
        ) {
        priceOfInterest = prevLow - MinCleanTicks.GetFloat() * sc.TickSize;
        if (IsCleanTick(priceOfInterest, sc)) {
            TradeSignal[sc.Index] = -1;
        }
    } else if (
        !isDown
        && BidAskDiff[sc.Index - 1] <= BuyThreshold.GetFloat()
        ) {
        priceOfInterest = prevHigh + MinCleanTicks.GetFloat() * sc.TickSize;
        if (IsCleanTick(priceOfInterest, sc)) {
            TradeSignal[sc.Index] = 1;
        }

    }
}

SCSFExport scsf_StrategyBasicFlag(SCStudyInterfaceRef sc) {

    SCInputRef InputStudy = sc.Input[0];
    SCInputRef CleanTicksForCumCum = sc.Input[1];
    SCInputRef CleanTicksForOrderSignal = sc.Input[2];
    SCInputRef CumulativeThresholdBuy = sc.Input[3];
    SCInputRef CumulativeThresholdSell = sc.Input[4];
    SCInputRef UseThreeKPIs = sc.Input[5];
    SCInputRef MinBarVolume = sc.Input[6];

    SCSubgraphRef Grid = sc.Subgraph[0];
    SCSubgraphRef CumSumAskVBidV = sc.Subgraph[1];
    SCSubgraphRef CumSumTotalV = sc.Subgraph[2];
    SCSubgraphRef CumSumAskTBidT = sc.Subgraph[3];
    SCSubgraphRef CumSumUpDownT = sc.Subgraph[4];
    SCSubgraphRef FracSignedImbalance = sc.Subgraph[5];

    // SCSubgraphRef UpOrDownCLean = sc.Subgraph[5];
    // SCSubgraphRef EnterSignal = sc.Subgraph[6];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Strategy basic flag";
        sc.GraphDrawType = GDT_NUMERIC_INFORMATION;

        // The Study
        InputStudy.Name = "Study to sum";
        InputStudy.SetStudyID(1);

        CleanTicksForCumCum.Name = "Clean ticks for cumulative sum";
        CleanTicksForCumCum.SetIntLimits(1, 4);
        CleanTicksForCumCum.SetInt(3);

        CleanTicksForOrderSignal.Name = "Clean ticks for order signal";
        CleanTicksForOrderSignal.SetIntLimits(1, 4);
        CleanTicksForOrderSignal.SetInt(2);

        CumulativeThresholdBuy.Name = "BUY max study amount";
        CumulativeThresholdBuy.SetIntLimits(-5000, -1);
        CumulativeThresholdBuy.SetInt(-200);

        CumulativeThresholdSell.Name = "SELL min study amount";
        CumulativeThresholdSell.SetIntLimits(1, 5000);
        CumulativeThresholdSell.SetInt(200);

        MinBarVolume.Name = "Min Volume for trade entry";
        MinBarVolume.SetIntLimits(0, 10000);
        MinBarVolume.SetInt(1000);

        UseThreeKPIs.Name = "Use all three volume KPIs";
        UseThreeKPIs.SetYesNo(0);

        Grid.Name = "Grid style";
        Grid.DrawStyle = DRAWSTYLE_LINE;
        Grid.PrimaryColor = COLOR_WHITE;

        CumSumAskVBidV.Name = "AskV - BidV";
        CumSumAskVBidV.DrawStyle = DRAWSTYLE_LINE;
        CumSumAskVBidV.PrimaryColor = COLOR_WHITE;

        CumSumTotalV.Name = "Total V";
        CumSumTotalV.DrawStyle = DRAWSTYLE_LINE;
        CumSumTotalV.PrimaryColor = COLOR_WHITE;

        CumSumAskTBidT.Name = "AskT - BidT";
        CumSumAskTBidT.DrawStyle = DRAWSTYLE_LINE;
        CumSumAskTBidT.PrimaryColor = COLOR_WHITE;

        CumSumUpDownT.Name = "Up Down Tick Volume Difference";
        CumSumUpDownT.DrawStyle = DRAWSTYLE_LINE;
        CumSumUpDownT.PrimaryColor = COLOR_WHITE;

        FracSignedImbalance.Name = "(AskV - BidV) / TotalV";
        FracSignedImbalance.DrawStyle = DRAWSTYLE_LINE;
        FracSignedImbalance.PrimaryColor = COLOR_WHITE;

        // UpOrDownCLean.Name = "Up or down clean";
        // EnterSignal.Name = "Enter signal";
        return;
    }
    if (sc.Index == 0) {
        //sc.ValueFormat = sc.BaseGraphValueFormat;

        s_NumericInformationGraphDrawTypeConfig NumericInformationGraphDrawTypeConfig;
        NumericInformationGraphDrawTypeConfig.TransparentTextBackground = false;
        NumericInformationGraphDrawTypeConfig.ShowPullback = true;

        NumericInformationGraphDrawTypeConfig.GridlineStyleSubgraphIndex = 0;  // Subgraph 1 to specify grid line style
        NumericInformationGraphDrawTypeConfig.ValueFormat[1] = 0;  // set value format for volume, others are inherited
        NumericInformationGraphDrawTypeConfig.ValueFormat[2] = 0;  // set value format for volume, others are inherited
        NumericInformationGraphDrawTypeConfig.ValueFormat[3] = 0;  // set value format for volume, others are inherited
        NumericInformationGraphDrawTypeConfig.ValueFormat[4] = 0;  // set value format for volume, others are inherited

        NumericInformationGraphDrawTypeConfig.ValueFormat[5] = 24;  // set value format for volume, others are inherited


        sc.SetNumericInformationGraphDrawTypeConfig(NumericInformationGraphDrawTypeConfig);
    }


    // Index
    const int i = sc.Index;

    // Getting the direction of the current bar
    const float O = sc.Open[i];
    const float prevHigh = sc.High[i - 1];
    const float prevLow = sc.Low[i - 1];
    const bool isDown = (O <= prevLow);

    const float priceOfInterestHigh = prevHigh + sc.TickSize * CleanTicksForCumCum.GetFloat();
    const float priceOfInterestLow = prevLow - sc.TickSize * CleanTicksForCumCum.GetFloat();
    const float priceOfInterestOrderHigh = prevHigh + sc.TickSize * CleanTicksForOrderSignal.GetFloat();
    const float priceOfInterestOrderLow = prevLow - sc.TickSize * CleanTicksForOrderSignal.GetFloat();

    // Building the cumulative sum for difference indicators
    SCFloatArray AskVBidV;
    SCFloatArray TotalV;
    SCFloatArray AskTBidT;
    SCFloatArray UpDownT;
    int retrieveSuccess = sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 0, AskVBidV);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 12, TotalV);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 23, AskTBidT);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 49, UpDownT);

    if (retrieveSuccess == 4) {

        if (i == 0) {
            // If it's the first bar, the spot and the cumulative are the same
            CumSumAskVBidV[i] = AskVBidV[i];
            CumSumTotalV[i] = TotalV[i];
            CumSumAskTBidT[i] = AskTBidT[i];
            CumSumUpDownT[i] = UpDownT[i];

        } else {
            // Otherwise we implement the cumulative logic
            if (const float priceOfInterest = isDown ? priceOfInterestLow : priceOfInterestHigh; IsCleanTick(priceOfInterest, sc)) {
                CumSumAskVBidV[i] = AskVBidV[i];
                CumSumTotalV[i] = TotalV[i];
                CumSumAskTBidT[i] = AskTBidT[i];
                CumSumUpDownT[i] = UpDownT[i];
            } else {
                CumSumAskVBidV[i] = AskVBidV[i] + CumSumAskVBidV[i-1];
                CumSumTotalV[i] = TotalV[i] + CumSumTotalV[i-1];
                CumSumAskTBidT[i] = AskTBidT[i]+ CumSumAskTBidT[i-1];
                CumSumUpDownT[i] = UpDownT[i] + CumSumUpDownT[i-1];
            }
        }
        FracSignedImbalance[i] = CumSumAskVBidV[i] / TotalV[i];
    }

    // Building the Up / Down clean flag
    //const int isCleanOrder = isDown ? static_cast<int>(IsCleanTick(priceOfInterestOrderLow, sc)): static_cast<int>(IsCleanTick(priceOfInterestOrderHigh, sc));
    //UpOrDownCLean[i] = isCleanOrder;
//
    //// Building the order entry flag
    //int orderEntryFlag = 0;
    //if (isDown && isCleanOrder == 1 && CumSumAskVBidV[i-1] >= CumulativeThresholdSell.GetFloat()) {
    //    orderEntryFlag = -1;
    //} else if (!isDown && isCleanOrder == 1 && CumSumAskVBidV[i-1] <= CumulativeThresholdBuy.GetFloat()) {
    //    orderEntryFlag = 1;
    //}
//
    //EnterSignal[i] = orderEntryFlag;
}


SCSFExport scsf_StrategyBasicPeakTypeVolumeExec(SCStudyInterfaceRef sc) {
    /*
     * This indicator is stating whether a trade should be CONSIDERED. It is NOT there
     *
     */
    SCInputRef Signal = sc.Input[0];
    SCInputRef RangeBarPredictors = sc.Input[1];

    SCSubgraphRef TradeId = sc.Subgraph[0];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Divergence trading executor";

        // The Study
        Signal.Name = "Trading Signal";
        Signal.SetStudyID(0);

        TradeId.Name = "Trade ID";

        RangeBarPredictors.Name = "Range bar predictor study";
        RangeBarPredictors.SetStudyID(0);
    }

    double peakMin;
    double peakMax;
    highLowCleanPricesInBar(sc, peakMin, peakMax);

    // Retrieving Studies
    SCFloatArray SignalValue;
    SCFloatArray TopBarPredictor;
    SCFloatArray LowBarPredictor;
    sc.GetStudyArrayUsingID(Signal.GetStudyID(), 2, SignalValue);
    sc.GetStudyArrayUsingID(RangeBarPredictors.GetStudyID(), 0, TopBarPredictor);
    sc.GetStudyArrayUsingID(RangeBarPredictors.GetStudyID(), 1, LowBarPredictor);

    if (SignalValue[sc.Index] == 1) {

    }
}
