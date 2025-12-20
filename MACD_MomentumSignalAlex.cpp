#include "sierrachart.h"

SCSFExport scsf_MomentumSignalAlex(SCStudyInterfaceRef sc) {
    /*
     * Momentum Signal based on MACD
     *
     * This signal uses the MACD study which contains:
     * - Subgraph 0: MACD line (Fast EMA - Slow EMA)
     * - Subgraph 1: Signal line (MA of MACD)
     * - Subgraph 2: MACD Diff/Histogram (MACD - Signal)
     *
     * Signal logic:
     * - When MACDDiff goes from positive to negative (or zero) -> Sell signal (-1)
     * - When MACDDiff goes from negative to positive (or zero) -> Buy signal (+1)
     */

    SCInputRef MACDStudy = sc.Input[0];

    SCSubgraphRef Signal = sc.Subgraph[0];
    SCSubgraphRef BuySignal = sc.Subgraph[1];
    SCSubgraphRef SellSignal = sc.Subgraph[2];
    SCSubgraphRef MACDDiffOutput = sc.Subgraph[3];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "MACD Momentum Signal Alex";

        MACDStudy.Name = "MACD Study";
        MACDStudy.SetStudyID(1);

        Signal.Name = "Signal";
        Signal.DrawStyle = DRAWSTYLE_BAR;
        Signal.PrimaryColor = RGB(255, 255, 0);
        Signal.LineWidth = 2;

        BuySignal.Name = "Buy Signal";
        BuySignal.DrawStyle = DRAWSTYLE_POINT_ON_HIGH;
        BuySignal.PrimaryColor = RGB(0, 255, 0);
        BuySignal.LineWidth = 3;

        SellSignal.Name = "Sell Signal";
        SellSignal.DrawStyle = DRAWSTYLE_POINT_ON_LOW;
        SellSignal.PrimaryColor = RGB(255, 0, 0);
        SellSignal.LineWidth = 3;

        MACDDiffOutput.Name = "MACD Diff";
        MACDDiffOutput.DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int i = sc.Index;

    // Retrieve MACD study arrays
    SCFloatArray MACD;      // Fast EMA - Slow EMA
    SCFloatArray MACDMA;    // Signal line (MA of MACD)
    SCFloatArray MACDDiff;  // MACD - Signal line (histogram)

    sc.GetStudyArrayUsingID(MACDStudy.GetStudyID(), 2, MACD);
    sc.GetStudyArrayUsingID(MACDStudy.GetStudyID(), 1, MACDMA);
    sc.GetStudyArrayUsingID(MACDStudy.GetStudyID(), 0, MACDDiff);

    // Initialize signals to 0
    Signal[i] = 0;
    BuySignal[i] = 0;
    SellSignal[i] = 0;
    MACDDiffOutput[i] = MACDDiff[i];

    // Need at least 2 bars to detect crossover
    if (i < 1) {
        return;
    }

    // Detect crossover from positive to negative (sell signal)
    // Previous bar: MACDDiff > 0, Current bar: MACDDiff <= 0
    const bool sellCondition = MACDDiff[i - 1] > 0 && MACDDiff[i] <= 0;

    // Detect crossover from negative to positive (buy signal)
    // Previous bar: MACDDiff < 0, Current bar: MACDDiff >= 0
    const bool buyCondition = MACDDiff[i - 1] < 0 && MACDDiff[i] >= 0;

    if (sellCondition) {
        Signal[i] = -1;
        SellSignal[i] = sc.High[i];
    }
    else if (buyCondition) {
        Signal[i] = 1;
        BuySignal[i] = sc.Low[i];
    }
}
