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
#include "TradeWrapper.h"

SCDLLName("MACD TRADING")

static std::unique_ptr<TradeWrapper> g_trade;


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
    SCInputRef VolumeEMEAWindow = sc.Input[2];
    SCInputRef AllowTradingAlways = sc.Input[3];



    SCSubgraphRef TradeId = sc.Subgraph[0];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Divergence trading executor";

        // The Study
        Signal.Name = "Trading Signal";
        Signal.SetStudyID(0);

        RangeBarPredictors.Name = "Range bar predictors";
        RangeBarPredictors.SetStudyID(1);

        VolumeEMEAWindow.Name = "Volume EMEA Window";
        VolumeEMEAWindow.SetIntLimits(1, 200);
        VolumeEMEAWindow.SetInt(10);

        AllowTradingAlways.Name = "Allow trading always";
        AllowTradingAlways.SetYesNo(0);

        TradeId.Name = "Trade ID";

        RangeBarPredictors.Name = "Range bar predictor study";
        RangeBarPredictors.SetStudyID(0);
    }

    // Common study specs
    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = 1;
    NewOrder.OrderType = SCT_ORDERTYPE_LIMIT;
    NewOrder.TimeInForce = SCT_TIF_DAY;

    // Retrieving ID
    int64_t &InternalOrderID = sc.GetPersistentInt64(1);

    // Trading allowed bool
    bool TradingAllowed = AllowTradingAlways.GetInt() == 1 ? true : tradingAllowedCash(sc);


    const int i = sc.Index;
    double peakMin;
    double peakMax;
    highLowCleanPricesInBar(sc, peakMin, peakMax);

    // Retrieving Studies
    SCFloatArray SignalValue;
    SCFloatArray ASkVBidV;
    SCFloatArray UpDownTVolDiff;

    SCFloatArray TopBarPredictor;
    SCFloatArray LowBarPredictor;
    sc.GetStudyArrayUsingID(Signal.GetStudyID(), 0, SignalValue);
    sc.GetStudyArrayUsingID(Signal.GetStudyID(), 1, ASkVBidV);
    sc.GetStudyArrayUsingID(Signal.GetStudyID(), 2, UpDownTVolDiff);

    sc.GetStudyArrayUsingID(RangeBarPredictors.GetStudyID(), 0, TopBarPredictor);
    sc.GetStudyArrayUsingID(RangeBarPredictors.GetStudyID(), 1, LowBarPredictor);

    sc.ExponentialMovAvg(sc.Volume, TradeId.Arrays[0], VolumeEMEAWindow.GetInt());

    const int allGreen = (ASkVBidV[i] > 0 && UpDownTVolDiff[i] > 0) ? 1 : 0;
    const int allRed = (ASkVBidV[i] < 0 && UpDownTVolDiff[i] < 0) ? 1 : 0;
    const bool volCondition = TradeId.Arrays[0][i] * 0.5 <= sc.Volume[i];

    const bool buyCondition = allGreen && SignalValue[i] == 1 && volCondition && TradingAllowed;
    const bool sellCondition = allRed && SignalValue[i] == -1 && volCondition && TradingAllowed;
    int orderSubmitted = 0;

    if (buyCondition) {
        NewOrder.Price1 = sc.LastTradePrice;
        NewOrder.Target1Price = std::max<double>(NewOrder.Price1 + sc.TickSize * 3, TopBarPredictor[i]);
        NewOrder.Stop1Offset = sc.TickSize * 3;

        orderSubmitted = static_cast<int>(sc.BuyOrder(NewOrder));
        if (orderSubmitted > 0) {
            InternalOrderID = NewOrder.InternalOrderID;
            sc.Subgraph[0][sc.Index] = static_cast<float>(InternalOrderID);
            SCString Buffer;
            Buffer.Format("ADDED ORDER WITH ID %d", InternalOrderID);
            sc.AddMessageToLog(Buffer, 1);
        }
    }
    else if (sellCondition) {
        NewOrder.Price1 = sc.LastTradePrice;
        NewOrder.Target1Price = std::min<double>(NewOrder.Price1 - sc.TickSize * 3, LowBarPredictor[i]);
        NewOrder.Stop1Offset = sc.TickSize * 3;

        orderSubmitted = static_cast<int>(sc.SellOrder(NewOrder));
        if (orderSubmitted > 0) {
            InternalOrderID = NewOrder.InternalOrderID;
            sc.Subgraph[0][sc.Index] = static_cast<float>(InternalOrderID);
            SCString Buffer;
            Buffer.Format("ADDED ORDER WITH ID %d", InternalOrderID);
            sc.AddMessageToLog(Buffer, 1);
        }
    }
}

SCSFExport scsf_StrategyMACDShort(SCStudyInterfaceRef sc) {
    /*
     Shorting Red MACD when:
    - The last xover is RED AND MACD goes negative
    - OR there is green xover and red xover in next bar
    - Trading either up to N Target ticks OR to green MACD
    - Can choose whether to only short when the price is below the EWA in the chart
    - Make sure the MACD is negative enough for the strategy. Having a MACD bouncing around 0 is a recipe for failure
    - The number of ticks that we take should be a function of the MACD value: if VERY negative then we can aim for higher ticks (long)
    - If very positive: we can aim for more ticks (Short)
    - Exit rules: make sure that if at some point in the trade the max P&L has down-ticked by 10, get out to at least get some snacks
    */
    SCInputRef PriceEMWAStudy = sc.Input[0];
    SCInputRef MACDXStudy = sc.Input[1];
    SCInputRef ATRStudy = sc.Input[2];

    // SCInputRef ADXStudy = sc.Input[1];

    // SCInputRef RangeTrendADXThresh = sc.Input[3];
    SCInputRef MaxMACDDiff = sc.Input[3];
    SCInputRef PriceEMWAMinOffset = sc.Input[4];
    SCInputRef UseRangeOnly = sc.Input[5];
    SCInputRef OneTradePerPeriod = sc.Input[6]; // One the trade ended in the "red period", don't perform any other trade in this period
    SCInputRef UseEWAThresh = sc.Input[7];
    SCInputRef GiveBackTicks = sc.Input[8];
    SCInputRef MaxTicksEntryFromCrossOVer = sc.Input[9];
    SCInputRef AllowTradingAlways = sc.Input[10];

    SCSubgraphRef TradeId = sc.Subgraph[0];
    SCSubgraphRef CumMaxOpenPnL = sc.Subgraph[1];
    SCSubgraphRef CurrentOpenPnL = sc.Subgraph[2];
    SCSubgraphRef posAveragePrice = sc.Subgraph[3];
    SCSubgraphRef posLowPrice = sc.Subgraph[4];
    SCSubgraphRef posHighPrice = sc.Subgraph[5];
    SCSubgraphRef lastTradeIndex = sc.Subgraph[6];
    SCSubgraphRef lastXOverIndex = sc.Subgraph[7];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trading MACD Short Exec";

        // The Study
        PriceEMWAStudy.Name = "PriceEMWA";
        PriceEMWAStudy.SetStudyID(6);

        //ADXStudy.Name = "ADX";
        //ADXStudy.SetStudyID(0);

        MACDXStudy.Name = "MACD CrossOver";
        MACDXStudy.SetStudyID(7);

        ATRStudy.Name = "ATR";
        ATRStudy.SetStudyID(2);

        MaxMACDDiff.Name = "Max MACDDiff";
        MaxMACDDiff.SetFloatLimits(-1., 0.0);
        MaxMACDDiff.SetFloat(-0.2);

        PriceEMWAMinOffset.Name = "Price EMWA Tick Offset";
        PriceEMWAMinOffset.SetIntLimits(0, 20);
        PriceEMWAMinOffset.SetInt(5);

        MaxTicksEntryFromCrossOVer.Name = "Maximum number of ticks entry after cross over";
        MaxTicksEntryFromCrossOVer.SetIntLimits(1, 20);
        MaxTicksEntryFromCrossOVer.SetInt(6);


        UseRangeOnly.Name = "Trade in range only";
        UseRangeOnly.SetYesNo(0);

        UseEWAThresh.Name = "Trade above/below EWA only";
        UseEWAThresh.SetYesNo(0);

        OneTradePerPeriod.Name = "Trade above/below EWA only";
        OneTradePerPeriod.SetYesNo(0);

        GiveBackTicks.Name = "Give back in ticks";
        GiveBackTicks.SetIntLimits(0, 20);
        GiveBackTicks.SetInt(10);

        AllowTradingAlways.Name = "Allow trading always";
        AllowTradingAlways.SetYesNo(0);

        TradeId.Name = "Trade ID";
        CumMaxOpenPnL.Name = "Cumulative maximum open PnL";
        CurrentOpenPnL.Name = "Current open PnL";

        posAveragePrice.Name = "Pos Avg Price";
        posAveragePrice.DrawStyle = DRAWSTYLE_IGNORE;

        posLowPrice.Name = "Pos Low Price";
        posLowPrice.DrawStyle = DRAWSTYLE_IGNORE;

        posHighPrice.Name = "Pos High Price";
        posHighPrice.DrawStyle = DRAWSTYLE_IGNORE;

        lastTradeIndex.Name = "Last trade index";
        lastTradeIndex.DrawStyle = DRAWSTYLE_LINE;

        lastXOverIndex.Name = "Last cross over index";
        lastXOverIndex.DrawStyle = DRAWSTYLE_LINE;

        // sc.MaximumPositionAllowed = 1;

    }
    const int i = sc.Index;

    // Common study specs
    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = 1;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.TimeInForce = SCT_TIF_DAY;

    // Retrieving ID
    int64_t &InternalOrderID = sc.GetPersistentInt64(1);
    int &LastCrossOverSellIndex = sc.GetPersistentInt(2);
    int &LastSellTradeIndex = sc.GetPersistentInt(3);
    double &FillPrice = sc.GetPersistentDouble(1);

    if (sc.IsFullRecalculation) {
        LastSellTradeIndex = 0;
    }

    // Trading allowed bool
    bool TradingAllowed = AllowTradingAlways.GetInt() == 1 ? true : tradingAllowedCash(sc);

    // Retrieving Studies
    SCFloatArray PriceEMWA;
    SCFloatArray MACD;
    SCFloatArray MACDMA;
    SCFloatArray MACDDiff;
    SCFloatArray ATR;

    sc.GetStudyArrayUsingID(PriceEMWAStudy.GetStudyID(), 0, PriceEMWA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 0, MACD);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 1, MACDMA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 2, MACDDiff);
    sc.GetStudyArrayUsingID(ATRStudy.GetStudyID(), 0, ATR);

    const int Xover = sc.CrossOver(MACD, MACDMA, i);
    if (Xover==CROSS_FROM_TOP) {
        LastCrossOverSellIndex = i;
    }

    const bool EWACond = UseEWAThresh.GetYesNo() == 1
    ? sc.Close[i] < PriceEMWA[i] && sc.Close[LastCrossOverSellIndex] < PriceEMWA[LastCrossOverSellIndex]
    : true;

    const bool sellCondition = EWACond
            && TradingAllowed
            && LastSellTradeIndex < LastCrossOverSellIndex
            && MACD[i] <= 0
            && MACDDiff[i] <= MaxMACDDiff.GetFloat() // && MACDDiff[i-1] <= MaxMACDDiff.GetFloat() // To avoid entry periods of 1 bar only...
            && i - LastCrossOverSellIndex <= MaxTicksEntryFromCrossOVer.GetInt();

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sellCondition && PositionData.PositionQuantity == 0) {
        int orderSubmitted = 0;
        NewOrder.Target1Offset = 2 * ATR[i];
        NewOrder.Stop1Offset = 2 * ATR[i];
        // NewOrder.Stop1Price = sc.High[LastCrossOverSellIndex] + sc.TickSize * 3;

        orderSubmitted = static_cast<int>(sc.SellOrder(NewOrder));
        LastSellTradeIndex = LastCrossOverSellIndex;
        if (orderSubmitted > 0) {
            FillPrice = NewOrder.Price1;
            InternalOrderID = NewOrder.InternalOrderID;
            // TradeId[i] = static_cast<float>(InternalOrderID);
            SCString Buffer;
            Buffer.Format("ADDED ORDER WITH ID %d", InternalOrderID);
            sc.AddMessageToLog(Buffer, 1);
        }
    }

    int& MaxPnLForTradeInTicks = sc.GetPersistentInt(1);
    if (int CurrentPnLTicks; PositionData.PositionQuantity != 0) {
        CurrentPnLTicks = static_cast<int>(PositionData.OpenProfitLoss  / sc.CurrencyValuePerTick);
        CurrentOpenPnL[i] = static_cast<float>(CurrentPnLTicks);
        if (CurrentPnLTicks > MaxPnLForTradeInTicks) {
            MaxPnLForTradeInTicks = CurrentPnLTicks;
            // const double NewStop = PositionData.PriceLowDuringPosition + (GiveBackTicks.GetFloat() * sc.TickSize);
            //SCString Buffer;
            //Buffer.Format("Need to change the stop to %f", static_cast<float>(NewStop));
            //sc.AddMessageToLog(Buffer, 1);
            //ModifyAttachedStop(InternalOrderID, NewStop, sc);
            //LogAttachedStop(sc.GetPersistentInt64(1), sc);

            // if (s_SCTradeOrder ParentOrder, StopOrder; sc.GetOrderByOrderID(sc.GetPersistentInt64(1), ParentOrder) == 1) {
            //     const int stopOrderSuccess = sc.GetOrderByOrderID(ParentOrder.StopChildInternalOrderID, StopOrder);
            //     if (stopOrderSuccess == 1) {
            //         s_SCNewOrder ModifyOrder;
            //         ModifyOrder.InternalOrderID = StopOrder.InternalOrderID;
            //         ModifyOrder.Price1 = ParentOrder.Price1 - (GiveBackTicks.GetInt() - MaxPnLForTradeInTicks) * sc.TickSize;
//
            //         SCString Buffer;
            //         Buffer.Format("Modified stop limit order price (%d) to %d", StopOrder.InternalOrderID, ModifyOrder.Price1);
            //         sc.AddMessageToLog(Buffer, 1);
//p
            //         sc.ModifyOrder(ModifyOrder);
            //     }
            // }
        }
    } else {
        MaxPnLForTradeInTicks = 0;
        CurrentOpenPnL[i] = 0;
        CumMaxOpenPnL[i] = 0;
        InternalOrderID = 0;
        FillPrice = 0;
    }
    posAveragePrice[i] = PositionData.AveragePrice;
    posLowPrice[i] = PositionData.PriceLowDuringPosition;
    posAveragePrice[i] = PositionData.PriceHighDuringPosition;
    lastTradeIndex[i] = LastSellTradeIndex;
    lastXOverIndex[i] = LastCrossOverSellIndex;

    CumMaxOpenPnL[i] = MaxPnLForTradeInTicks;
    TradeId[i] = static_cast<float>(InternalOrderID);

    flattenAllAfterCash(sc);
}

SCSFExport scsf_StrategyMACDShortFromManager(SCStudyInterfaceRef sc) {
    /*
     Shorting Red MACD when:
    - The last xover is RED AND MACD goes negative
    - OR there is green xover and red xover in next bar
    - Trading either up to N Target ticks OR to green MACD
    - Can choose whether to only short when the price is below the EWA in the chart
    - Make sure the MACD is negative enough for the strategy. Having a MACD bouncing around 0 is a recipe for failure
    - The number of ticks that we take should be a function of the MACD value: if VERY negative then we can aim for higher ticks (long)
    - If very positive: we can aim for more ticks (Short)
    - Exit rules: make sure that if at some point in the trade the max P&L has down-ticked by 10, get out to at least get some snacks
    */
    SCInputRef PriceEMWAStudy = sc.Input[0];
    SCInputRef MACDXStudy = sc.Input[1];
    SCInputRef ATRStudy = sc.Input[2];

    // SCInputRef ADXStudy = sc.Input[1];

    // SCInputRef RangeTrendADXThresh = sc.Input[3];
    SCInputRef MaxMACDDiff = sc.Input[3];
    SCInputRef PriceEMWAMinOffset = sc.Input[4];
    SCInputRef UseRangeOnly = sc.Input[5];
    SCInputRef OneTradePerPeriod = sc.Input[6]; // One the trade ended in the "red period", don't perform any other trade in this period
    SCInputRef UseEWAThresh = sc.Input[7];
    SCInputRef GiveBackTicks = sc.Input[8];
    SCInputRef MaxTicksEntryFromCrossOVer = sc.Input[9];
    SCInputRef AllowTradingAlways = sc.Input[10];

    SCSubgraphRef TradeId = sc.Subgraph[0];
    SCSubgraphRef CumMaxOpenPnL = sc.Subgraph[1];
    SCSubgraphRef CurrentOpenPnL = sc.Subgraph[2];
    SCSubgraphRef lastTradeIndex = sc.Subgraph[3];
    SCSubgraphRef lastXOverIndex = sc.Subgraph[4];
    SCSubgraphRef tradeFilledPrice = sc.Subgraph[5];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trading MACD Short Exec - MANAGER";

        // The Study
        PriceEMWAStudy.Name = "PriceEMWA";
        PriceEMWAStudy.SetStudyID(6);

        //ADXStudy.Name = "ADX";
        //ADXStudy.SetStudyID(0);

        MACDXStudy.Name = "MACD CrossOver";
        MACDXStudy.SetStudyID(7);

        ATRStudy.Name = "ATR";
        ATRStudy.SetStudyID(2);

        MaxMACDDiff.Name = "Max MACDDiff";
        MaxMACDDiff.SetFloatLimits(-1., 0.0);
        MaxMACDDiff.SetFloat(-0.2);

        PriceEMWAMinOffset.Name = "Price EMWA Tick Offset";
        PriceEMWAMinOffset.SetIntLimits(0, 20);
        PriceEMWAMinOffset.SetInt(5);

        MaxTicksEntryFromCrossOVer.Name = "Maximum number of ticks entry after cross over";
        MaxTicksEntryFromCrossOVer.SetIntLimits(1, 20);
        MaxTicksEntryFromCrossOVer.SetInt(30);


        UseRangeOnly.Name = "Trade in range only";
        UseRangeOnly.SetYesNo(0);

        UseEWAThresh.Name = "Trade above/below EWA only";
        UseEWAThresh.SetYesNo(0);

        OneTradePerPeriod.Name = "Trade above/below EWA only";
        OneTradePerPeriod.SetYesNo(0);

        GiveBackTicks.Name = "Give back in ticks";
        GiveBackTicks.SetIntLimits(0, 20);
        GiveBackTicks.SetInt(10);

        AllowTradingAlways.Name = "Allow trading always";
        AllowTradingAlways.SetYesNo(0);

        TradeId.Name = "Trade ID";
        TradeId.DrawStyle = DRAWSTYLE_IGNORE;


        CumMaxOpenPnL.Name = "Cumulative maximum open PnL";
        CumMaxOpenPnL.DrawStyle = DRAWSTYLE_IGNORE;

        CurrentOpenPnL.Name = "Current open PnL";
        CurrentOpenPnL.DrawStyle = DRAWSTYLE_IGNORE;

        lastTradeIndex.Name = "Last trade index";
        lastTradeIndex.DrawStyle = DRAWSTYLE_IGNORE;

        lastXOverIndex.Name = "Last cross over index";
        lastXOverIndex.DrawStyle = DRAWSTYLE_IGNORE;

        tradeFilledPrice.Name = "Trade filled price";
        tradeFilledPrice.DrawStyle = DRAWSTYLE_LINE;

    }
    const int i = sc.Index;

    auto trade = reinterpret_cast<TradeWrapper*>(sc.GetPersistentPointer(1));
    if (sc.IsFullRecalculation) {
        trade = NULL;
        sc.SetPersistentPointer(1, trade);
    }

    // Common study specs
    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = 1;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.TimeInForce = SCT_TIF_DAY;

    // Retrieving ID
    int64_t &InternalOrderID = sc.GetPersistentInt64(1);
    int &LastCrossOverSellIndex = sc.GetPersistentInt(2);
    int &LastSellTradeIndex = sc.GetPersistentInt(3);
    double &FillPrice = sc.GetPersistentDouble(1);

    if (sc.IsFullRecalculation) {
        LastSellTradeIndex = 0;
    }

    // Trading allowed bool
    bool TradingAllowed = AllowTradingAlways.GetInt() == 1 ? true : tradingAllowedCash(sc);

    // Retrieving Studies
    SCFloatArray PriceEMWA;
    SCFloatArray MACD;
    SCFloatArray MACDMA;
    SCFloatArray MACDDiff;
    SCFloatArray ATR;

    sc.GetStudyArrayUsingID(PriceEMWAStudy.GetStudyID(), 0, PriceEMWA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 0, MACD);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 1, MACDMA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 2, MACDDiff);
    sc.GetStudyArrayUsingID(ATRStudy.GetStudyID(), 0, ATR);

    const int Xover = sc.CrossOver(MACD, MACDMA, i);
    if (Xover==CROSS_FROM_TOP) {
        LastCrossOverSellIndex = i;
    }

    const bool EWACond = UseEWAThresh.GetYesNo() == 1
    ? sc.Close[i] < PriceEMWA[i] && sc.Close[LastCrossOverSellIndex] < PriceEMWA[LastCrossOverSellIndex]
    : true;

    const bool sellCondition = EWACond
            && TradingAllowed
            && LastSellTradeIndex < LastCrossOverSellIndex
            && MACD[i] <= 0
            && MACDDiff[i] <= MaxMACDDiff.GetFloat() // && MACDDiff[i-1] <= MaxMACDDiff.GetFloat() // To avoid entry periods of 1 bar only...
            && i - LastCrossOverSellIndex <= MaxTicksEntryFromCrossOVer.GetInt();

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sellCondition && trade == NULL) {
        int orderSubmitted = 0;
        NewOrder.Target1Offset = 2 * ATR[i];
        NewOrder.Stop1Offset = 2 * ATR[i];
        // NewOrder.Stop1Price = sc.High[LastCrossOverSellIndex] + sc.TickSize * 3;

        orderSubmitted = static_cast<int>(sc.SellOrder(NewOrder));

        if (orderSubmitted > 0) {
            LastSellTradeIndex = LastCrossOverSellIndex;
            FillPrice = NewOrder.Price1;
            InternalOrderID = NewOrder.InternalOrderID;
            trade = new TradeWrapper(InternalOrderID, TargetMode::Flat, 2., 2., BSE_SELL);
            sc.SetPersistentPointer(1, trade);
            TradeId[i] = static_cast<float>(InternalOrderID);
            SCString Buffer;
            Buffer.Format("ADDED ORDER WITH ID %d", InternalOrderID);
            sc.AddMessageToLog(Buffer, 1);
        }
    }

    if (trade != NULL) {
        switch (trade->getRealStatus()) {
            case TradeStatus::Terminated:
                delete trade;
                trade = NULL;
                break;
            default:
                trade->updateOrders(sc);
                trade->updateAttributes(sc.Close[i], ATR[i]);
                tradeFilledPrice[i] = static_cast<float>(trade->getFilledPrice());
                break;
        }
    }

    lastTradeIndex[i] = static_cast<float>(LastSellTradeIndex);
    lastXOverIndex[i] = static_cast<float>(LastCrossOverSellIndex);

    TradeId[i] = static_cast<float>(InternalOrderID);
}