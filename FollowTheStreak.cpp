#include "sierrachart.h"
#include "helpers.h"


SCSFExport scsf_FollowTheStreakSignal(SCStudyInterfaceRef sc) {

    SCInputRef DVStreaksStudy = sc.Input[0];
    SCInputRef NStreak = sc.Input[1];
    SCInputRef StopLossBufferInTicks = sc.Input[2];
    SCInputRef MaxRiskInTicks = sc.Input[3];


    SCSubgraphRef StreakSignal = sc.Subgraph[0];
    SCSubgraphRef StopLossPrice = sc.Subgraph[1];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Follow the streak signal";

        // Inputs
        DVStreaksStudy.Name = "Delta volume streaks study";
        DVStreaksStudy.SetStudyID(2);

        NStreak.Name = "Streak length to follow";
        NStreak.SetIntLimits(0, 200);
        NStreak.SetInt(3);

        StopLossBufferInTicks.Name = "Stop loss buffer in ticks";
        StopLossBufferInTicks.SetIntLimits(0, 200);
        StopLossBufferInTicks.SetInt(2);

        MaxRiskInTicks.Name = "Max risk (in ticks)";
        MaxRiskInTicks.SetIntLimits(0, 200);
        MaxRiskInTicks.SetInt(20);

        // Outputs
        StreakSignal.Name = "N Streak signal";
        StreakSignal.DrawStyle = DRAWSTYLE_IGNORE;

        StopLossPrice.Name = "Stop loss price";
        StopLossPrice.DrawStyle = DRAWSTYLE_LINE;

        sc.GraphRegion = 0;
    }
    const int currIx = sc.Index;
    float estimatedEntryPrice;
    float hhll;
    float& stopLossPrice = sc.GetPersistentFloat(1);
    float& stopLossInTicks = sc.GetPersistentFloat(2);


    SCFloatArray StreakDirection;
    SCFloatArray StreakLow;
    SCFloatArray StreakHigh;
    SCFloatArray StreakLength;
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 0, StreakDirection);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 1, StreakHigh);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 2, StreakLow);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 3, StreakLength);

    if (StreakLength[currIx] >= NStreak.GetFloat() && StreakLength[currIx] <= NStreak.GetFloat() + 2) { // Equal condition makes sure only signal per streak
        hhll = higherHighsLowerLows(sc, currIx - 1, NStreak.GetInt());
        if (StreakDirection[currIx] > 0&& sc.High[currIx] > sc.High[currIx - 1]  && hhll > 0) {
            stopLossPrice = StreakLow[currIx] - StopLossBufferInTicks.GetFloat() * sc.TickSize;
            estimatedEntryPrice = sc.Open[currIx] + sc.TickSize;
            stopLossInTicks = (estimatedEntryPrice - stopLossPrice) / sc.TickSize;
        }
        if (StreakDirection[currIx] < 0 && sc.Low[currIx] < sc.Low[currIx - 1] && hhll < 0) {
            stopLossPrice = StreakHigh[currIx] + StopLossBufferInTicks.GetFloat() * sc.TickSize;
            estimatedEntryPrice = sc.Open[currIx] - sc.TickSize;
            stopLossInTicks = (stopLossPrice - estimatedEntryPrice) / sc.TickSize;
        }

        if (stopLossInTicks <= MaxRiskInTicks.GetFloat()) {
            StreakSignal[currIx] = hhll;
        }
    }

    StopLossPrice[currIx] = stopLossPrice;

    // if (StreakLength[currIx] >= NStreak.GetFloat() && higherHighsHigherLows(sc, currIx - 1, NStreak.GetInt())) {
    //     StreakSignal[currIx] = 1;
    // }


}