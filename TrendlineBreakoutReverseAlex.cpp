
#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_TrendlineBreakoutReverseAlex(SCStudyInterfaceRef sc) {

    SCInputRef TrendlineBreakoutSignalStudy = sc.Input[0];

    SCSubgraphRef Signal = sc.Subgraph[0];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trendline Breakout Reverse Alex";
        sc.GraphRegion = 1;

        // Inputs
        TrendlineBreakoutSignalStudy.Name = "Trendline Breakout Signal Study";
        TrendlineBreakoutSignalStudy.SetStudyID(1);

        // Subgraphs
        Signal.Name = "Signal";
        Signal.DrawStyle = DRAWSTYLE_LINE;
        Signal.PrimaryColor = RGB(0, 255, 0);
        Signal.LineWidth = 2;

        return;
    }

    if (sc.LastCallToFunction)
        return;

    const int i = sc.Index;

    // Get the original signal from Trendline Breakout Signal study
    SCFloatArray OriginalSignal;
    sc.GetStudyArrayUsingID(TrendlineBreakoutSignalStudy.GetStudyID(), 0, OriginalSignal);

    // Initialize on first bar
    if (i == 0) {
        Signal[i] = 0;
        return;
    }

    float currentOriginalSignal = OriginalSignal[i];
    float prevOriginalSignal = OriginalSignal[i - 1];

    // Default: no signal
    Signal[i] = 0;

    // Long signal: transition to 1 (from anything that's not 1)
    if (currentOriginalSignal == 1 && prevOriginalSignal != 1) {
        Signal[i] = 1;
    }
    // Short signal: transition from 1 to -1 (reversal)
    else if (currentOriginalSignal == -1 && prevOriginalSignal == 1) {
        Signal[i] = -1;
    }
}
