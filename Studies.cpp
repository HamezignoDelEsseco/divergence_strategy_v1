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

SCSFExport scsf_StrategyBasicFlagTable(SCStudyInterfaceRef sc) {

    SCInputRef InputStudy = sc.Input[0];
    SCInputRef CleanTicksForCumCum = sc.Input[1];
    SCInputRef CleanTicksForOrderSignal = sc.Input[2];
    SCInputRef CumulativeThresholdBuy = sc.Input[3];
    SCInputRef CumulativeThresholdSell = sc.Input[4];
    SCInputRef UseAskVBidV = sc.Input[5];
    SCInputRef VolumeEMEAWindow = sc.Input[6];

    SCSubgraphRef Grid = sc.Subgraph[0];
    SCSubgraphRef CumSumAskVBidV = sc.Subgraph[3];
    SCSubgraphRef CumSumTotalV = sc.Subgraph[4];
    SCSubgraphRef CumSumAskTBidT = sc.Subgraph[5];
    SCSubgraphRef CumSumUpDownT = sc.Subgraph[6];
    SCSubgraphRef FracSignedImbalance = sc.Subgraph[7];
    SCSubgraphRef CumMinAskVBidV = sc.Subgraph[8];
    SCSubgraphRef CumMaxAskVBidV = sc.Subgraph[9];
    SCSubgraphRef MinMaxDiff = sc.Subgraph[10];
    SCSubgraphRef VolEMEA = sc.Subgraph[11];


    SCSubgraphRef UpOrDownCLean = sc.Subgraph[2];
    SCSubgraphRef EnterSignal = sc.Subgraph[1];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Strategy basic flag debug";
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

        VolumeEMEAWindow.Name = "Volume EMEA Window";
        VolumeEMEAWindow.SetIntLimits(1, 200);
        VolumeEMEAWindow.SetInt(20);

        UseAskVBidV.Name = "Use AskV - BidV";
        UseAskVBidV.SetYesNo(0);

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

        CumMinAskVBidV.Name = "Min(AskV - BidV)";
        CumMinAskVBidV.DrawStyle = DRAWSTYLE_LINE;
        CumMinAskVBidV.PrimaryColor = COLOR_WHITE;

        CumMaxAskVBidV.Name = "Max(AskV - BidV)";
        CumMaxAskVBidV.DrawStyle = DRAWSTYLE_LINE;
        CumMaxAskVBidV.PrimaryColor = COLOR_WHITE;

        MinMaxDiff.Name = "MaxAskVBidV - MinAskVBidV";
        MinMaxDiff.DrawStyle = DRAWSTYLE_LINE;
        MinMaxDiff.PrimaryColor = COLOR_WHITE;

        VolEMEA.Name = "Volume EMEA";
        VolEMEA.DrawStyle = DRAWSTYLE_LINE;
        VolEMEA.PrimaryColor = COLOR_WHITE;

        UpOrDownCLean.Name = "Up or down clean";
        EnterSignal.Name = "Enter signal";
        return;
    }
    if (sc.Index == 0) {
        //sc.ValueFormat = sc.BaseGraphValueFormat;

        s_NumericInformationGraphDrawTypeConfig NumericInformationGraphDrawTypeConfig;
        NumericInformationGraphDrawTypeConfig.TransparentTextBackground = false;
        NumericInformationGraphDrawTypeConfig.ShowPullback = false;

        NumericInformationGraphDrawTypeConfig.GridlineStyleSubgraphIndex = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[1] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[2] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[3] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[4] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[5] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[6] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[7] = 24;
        NumericInformationGraphDrawTypeConfig.ValueFormat[8] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[9] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[10] = 0;
        NumericInformationGraphDrawTypeConfig.ValueFormat[11] = 0;
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
    SCFloatArray MinAskVBidV;
    SCFloatArray MaxAskVBidV;
    int retrieveSuccess = sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 0, AskVBidV);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 12, TotalV);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 23, AskTBidT);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 49, UpDownT);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 8, MinAskVBidV);
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 7, MaxAskVBidV);

    if (retrieveSuccess == 6) {

        if (i == 0) {
            // If it's the first bar, the spot and the cumulative are the same
            CumSumAskVBidV[i] = AskVBidV[i];
            CumSumTotalV[i] = TotalV[i];
            CumSumAskTBidT[i] = AskTBidT[i];
            CumSumUpDownT[i] = UpDownT[i];
            CumMaxAskVBidV[i] = MaxAskVBidV[i];
            CumMaxAskVBidV[i] = MaxAskVBidV[i];

        } else {
            // Otherwise we implement the cumulative logic
            if (const float priceOfInterest = isDown ? priceOfInterestLow : priceOfInterestHigh; IsCleanTick(priceOfInterest, sc)) {
                CumSumAskVBidV[i] = AskVBidV[i];
                CumSumTotalV[i] = TotalV[i];
                CumSumAskTBidT[i] = AskTBidT[i];
                CumSumUpDownT[i] = UpDownT[i];
            } else {
                CumSumAskVBidV[i] = AskVBidV[i] + CumSumAskVBidV[i-1];
                CumSumTotalV[i] = TotalV[i]; // Not cum summing this one for EMEA consistency
                CumSumAskTBidT[i] = AskTBidT[i]+ CumSumAskTBidT[i-1];
                CumSumUpDownT[i] = UpDownT[i] + CumSumUpDownT[i-1];
            }
        }
        CumMaxAskVBidV[i] = MaxAskVBidV[i];
        CumMinAskVBidV[i] = MinAskVBidV[i];
        MinMaxDiff[i] = MaxAskVBidV[i] + MinAskVBidV[i];
        FracSignedImbalance[i] = CumSumAskVBidV[i] / TotalV[i];
        sc.ExponentialMovAvg(sc.Volume, VolEMEA, VolumeEMEAWindow.GetInt());
    }

    colorAllSubGraphs(sc, i, CumSumAskVBidV, CumSumAskTBidT, CumSumUpDownT, MinMaxDiff);


    //Building the Up / Down clean flag
    const int isCleanOrder = isDown ? static_cast<int>(IsCleanTick(priceOfInterestOrderLow, sc)): static_cast<int>(IsCleanTick(priceOfInterestOrderHigh, sc));
    UpOrDownCLean[i] = isCleanOrder;

    // Building the order entry flag
    int orderEntryFlag = 0;
    if (isDown && isCleanOrder == 1 && (UseAskVBidV.GetInt() == 1 ?CumSumAskVBidV[i-1]:CumSumUpDownT[i-1]) >= CumulativeThresholdSell.GetFloat()) {
        orderEntryFlag = -1;
    } else if (!isDown && isCleanOrder == 1 && (UseAskVBidV.GetInt() == 1 ?CumSumAskVBidV[i-1]:CumSumUpDownT[i-1]) <= CumulativeThresholdBuy.GetFloat()) {
        orderEntryFlag = 1;
    }

    EnterSignal[i] = orderEntryFlag;
}

SCSFExport scsf_StrategyBasicFlag(SCStudyInterfaceRef sc) {

    SCInputRef InputStudy = sc.Input[0];
    SCInputRef CleanTicksForCumCum = sc.Input[1];
    SCInputRef CleanTicksForOrderSignal = sc.Input[2];
    SCInputRef CumulativeThresholdBuy = sc.Input[3];
    SCInputRef CumulativeThresholdSell = sc.Input[4];
    SCInputRef UseAskVBidV = sc.Input[5];
    SCInputRef UseAskVBidVAndUpDownT = sc.Input[6];

    SCInputRef VolumeEMEAWindow = sc.Input[7];

    SCSubgraphRef EnterSignal = sc.Subgraph[0];
    SCSubgraphRef CumSumAskVBidV = sc.Subgraph[1];
    SCSubgraphRef CumSumUpDownTVolDiff = sc.Subgraph[2];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Strategy basic flag";

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

        VolumeEMEAWindow.Name = "Volume EMEA Window";
        VolumeEMEAWindow.SetIntLimits(1, 200);
        VolumeEMEAWindow.SetInt(20);

        UseAskVBidV.Name = "Use AskV - BidV";
        UseAskVBidV.SetYesNo(0);

        UseAskVBidVAndUpDownT.Name = "Use (AskV - BidV) or (UpT - DownT vol diff)";
        UseAskVBidVAndUpDownT.SetYesNo(1);

        EnterSignal.Name = "Enter signal";
        CumSumAskVBidV.Name = "CumSumAskVBidV";
        CumSumUpDownTVolDiff.Name = "CumSumUpDownTVolDiff";

        return;
    }
    // Result of the study (-1 or 1)
    int orderEntryFlag = 0;

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

    SCFloatArray MinAskVBidV;
    SCFloatArray MaxAskVBidV;

    int retrieveSuccess = sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 0, EnterSignal.Arrays[0]); // AskV - BidV
    // retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 12, EnterSignal.Arrays[1]); // Total V
    // retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 23, EnterSignal.Arrays[2]); // AskT - BidT
    retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 49, EnterSignal.Arrays[1]); // UpDownT
    // retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 8, MinAskVBidV);
    // retrieveSuccess += sc.GetStudyArrayUsingID(InputStudy.GetStudyID(), 7, MaxAskVBidV);

    if (retrieveSuccess == 2) {
            if (const float priceOfInterest = isDown ? priceOfInterestLow : priceOfInterestHigh; !IsCleanTick(priceOfInterest, sc)) {
                EnterSignal.Arrays[0][i] += EnterSignal.Arrays[0][i-1];
                // EnterSignal.Arrays[1][i] += EnterSignal.Arrays[1][i-1]; // We don't sum total Volume as this would falsify EMEA
                // EnterSignal.Arrays[2][i] += EnterSignal.Arrays[2][i-1];
                EnterSignal.Arrays[1][i] += EnterSignal.Arrays[1][i-1];
            }
        // EnterSignal.Arrays[4][i] = MaxAskVBidV[i] + MinAskVBidV[i];
    }

    // sc.ExponentialMovAvg(sc.Volume, EnterSignal.Arrays[2], VolumeEMEAWindow.GetInt());

    const int isCleanOrder = isDown ? static_cast<int>(IsCleanTick(priceOfInterestOrderLow, sc)): static_cast<int>(IsCleanTick(priceOfInterestOrderHigh, sc));
    if (isDown && isCleanOrder) {
        if (UseAskVBidVAndUpDownT.GetInt() == 1) {
            orderEntryFlag = EnterSignal.Arrays[0][i-1] >= CumulativeThresholdSell.GetFloat() | EnterSignal.Arrays[3][i-1] >= CumulativeThresholdSell.GetFloat() ? -1: 0;
        } else if (UseAskVBidV.GetInt() == 1 ) {
            orderEntryFlag = EnterSignal.Arrays[0][i-1] >= CumulativeThresholdSell.GetFloat() ? -1: 0;
        } else {
            orderEntryFlag = EnterSignal.Arrays[3][i-1] >= CumulativeThresholdSell.GetFloat() ? -1: 0;
        }
    }

    if (!isDown && isCleanOrder) {
        if (UseAskVBidVAndUpDownT.GetInt() == 1) {
            orderEntryFlag = EnterSignal.Arrays[0][i-1] <= CumulativeThresholdBuy.GetFloat() | EnterSignal.Arrays[3][i-1] <= CumulativeThresholdBuy.GetFloat() ? 1: 0;
        } else if (UseAskVBidV.GetInt() == 1 ) {
            orderEntryFlag = EnterSignal.Arrays[0][i-1] <= CumulativeThresholdBuy.GetFloat() ? 1: 0;
        } else {
            orderEntryFlag = EnterSignal.Arrays[3][i-1] <= CumulativeThresholdBuy.GetFloat() ? 1: 0;
        }
    }

    EnterSignal[i] = static_cast<float>(orderEntryFlag);
    CumSumAskVBidV[i] = EnterSignal.Arrays[0][i];
    CumSumUpDownTVolDiff[i] = EnterSignal.Arrays[1][i];

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
