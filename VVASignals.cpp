#include "sierrachart.h"
#include "helpers.h"


SCSFExport scsf_StreakCrossingLevel(SCStudyInterfaceRef sc) {

    SCInputRef DVStreaksStudy = sc.Input[0];
    SCInputRef StudyLevel = sc.Input[1];
    SCInputRef StopLossBufferInTicks = sc.Input[2];
    SCInputRef MaxRiskInTicks = sc.Input[3];


    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef StopLossPrice = sc.Subgraph[1];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Streak crossing the level";

        // Inputs
        DVStreaksStudy.Name = "Delta volume streaks study";
        DVStreaksStudy.SetStudyID(2);

        StudyLevel.Name = "Study containing level";
        StudyLevel.SetStudySubgraphValues(1,0);

        StopLossBufferInTicks.Name = "Stop loss buffer in ticks";
        StopLossBufferInTicks.SetIntLimits(0, 200);
        StopLossBufferInTicks.SetInt(1);

        MaxRiskInTicks.Name = "Max risk (in ticks)";
        MaxRiskInTicks.SetIntLimits(0, 200);
        MaxRiskInTicks.SetInt(1);

        // Outputs
        TradeSignal.Name = "Trade signal";
        TradeSignal.DrawStyle = DRAWSTYLE_IGNORE;

        StopLossPrice.Name = "Stop loss price";
        StopLossPrice.DrawStyle = DRAWSTYLE_LINE;

        sc.GraphRegion = 0;

    }
    const int currIx = sc.Index - 1;
    const int prevIx = currIx - 1;

    if (currIx == 0 || sc.IsNewTradingDay(currIx)) {
        return;
    }

    SCFloatArray LevelArray;
    sc.GetStudyArrayUsingID(StudyLevel.GetStudyID(), StudyLevel.GetSubgraphIndex(), LevelArray);


    SCFloatArray StreakDirection;
    SCFloatArray StreakLow;
    SCFloatArray StreakHigh;
    SCFloatArray StreakLength;
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 0, StreakDirection);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 1, StreakHigh);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 2, StreakLow);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 3, StreakLength);

    float& stopLossPrice = sc.GetPersistentFloat(1);


    if (StreakLength[currIx] >= 3) {
        double lowest;
        double highest;
        highestLowestOfNBars(sc, 3, prevIx, lowest, highest);

        if (// Cross from above
            StreakLow[prevIx] < LevelArray[prevIx]
            && StreakLow[currIx - 3] > LevelArray[currIx - 3]
            && sc.Close[currIx] < LevelArray[currIx]
            && StreakDirection[currIx] < 0
            ) {
            TradeSignal[currIx + 1] = -1;
        }
        if (// Cross from below
            StreakHigh[prevIx] > LevelArray[prevIx]
            && StreakHigh[currIx - 3] < LevelArray[currIx - 3]
            && sc.Close[currIx] > LevelArray[currIx]
            && StreakDirection[currIx] > 0
            ) {
            TradeSignal[currIx + 1] = 1;

        }
    }
    StopLossPrice[currIx + 1] = stopLossPrice;
}