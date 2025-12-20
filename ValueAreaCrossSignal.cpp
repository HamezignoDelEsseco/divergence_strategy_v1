#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_ValueAreaCrossSignal(SCStudyInterfaceRef sc) {

    SCInputRef VolumeValueAreaStudy = sc.Input[0];

    SCSubgraphRef BuySignal = sc.Subgraph[0];
    SCSubgraphRef SellSignal = sc.Subgraph[1];
    SCSubgraphRef Signal = sc.Subgraph[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Value Area Cross Signal";

        VolumeValueAreaStudy.Name = "Volume Value Area Lines Study";
        VolumeValueAreaStudy.SetStudyID(1);

        BuySignal.Name = "Buy Signal";
        BuySignal.DrawStyle = DRAWSTYLE_LINE;
        BuySignal.PrimaryColor = RGB(0, 255, 0);
        BuySignal.LineWidth = 2;

        SellSignal.Name = "Sell Signal";
        SellSignal.DrawStyle = DRAWSTYLE_LINE;
        SellSignal.PrimaryColor = RGB(255, 0, 0);
        SellSignal.LineWidth = 2;

        Signal.Name = "Signal";
        Signal.DrawStyle = DRAWSTYLE_BAR;;
        Signal.PrimaryColor = RGB(255, 255, 0);

        return;
    }

    const int i = sc.Index;

    // Get the three subgraphs from Volume Value Area Lines study
    SCFloatArray POC;
    SCFloatArray ValueAreaHigh;
    SCFloatArray ValueAreaLow;

    sc.GetStudyArrayUsingID(VolumeValueAreaStudy.GetStudyID(), 0, POC);
    sc.GetStudyArrayUsingID(VolumeValueAreaStudy.GetStudyID(), 1, ValueAreaHigh);
    sc.GetStudyArrayUsingID(VolumeValueAreaStudy.GetStudyID(), 2, ValueAreaLow);

    // Initialize all signals to 0 first (ensure clean reset every bar)
    Signal[i] = 0;
    BuySignal[i] = 0;
    SellSignal[i] = 0;

    if (i == 0) {
        return;
    }

    // Short signal: Price is above VAH and crosses down through VAH
    // Cross detection: previous close was above VAH, current close is at or below VAH
    const bool shortCondition = sc.Open[i-1] > ValueAreaHigh[i-1] && sc.Close[i] <= ValueAreaHigh[i];

    // Buy signal: Price is below VAL and crosses up through VAL (re-entry)
    // Cross detection: previous close was below VAL, current close is at or above VAL
    const bool buyCondition = sc.Open[i-1] < ValueAreaLow[i-1] && sc.Close[i] >= ValueAreaLow[i];

    if (shortCondition) {
        Signal[i] = -1;
        SellSignal[i] = sc.High[i];
    }
    else if (buyCondition) {
        Signal[i] = 1;
        BuySignal[i] = sc.Low[i];
    }
    // else: signals already set to 0 above
}
