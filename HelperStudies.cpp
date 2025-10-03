#include "helpers.h"
#include "sierrachart.h"


SCSFExport scsf_IsCleanTick(SCStudyInterfaceRef sc) {

    SCInputRef InputStudy = sc.Input[0];
    SCInputRef CleanTicksForCumCum = sc.Input[1];

    SCSubgraphRef CleanTick = sc.Subgraph[0];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Clean tick flag";

        // The Study
        InputStudy.Name = "Study to sum";
        InputStudy.SetStudyID(1);

        CleanTicksForCumCum.Name = "Clean ticks for cumulative sum";
        CleanTicksForCumCum.SetIntLimits(1, 4);
        CleanTicksForCumCum.SetInt(1);

        CleanTick.Name = "Clean tick flag";

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

    const float isCleanTick = isDown ? IsCleanTick(priceOfInterestLow, sc) :IsCleanTick(priceOfInterestHigh, sc);

    CleanTick[i] = isCleanTick;
}