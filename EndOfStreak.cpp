#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_DeltaVolStreaks(SCStudyInterfaceRef sc) {

    SCInputRef NumberBarsStudy = sc.Input[0];


    SCSubgraphRef StreakDirection = sc.Subgraph[0];
    SCSubgraphRef StreakHigh = sc.Subgraph[1];
    SCSubgraphRef StreakLow = sc.Subgraph[2];
    SCSubgraphRef CurrentStreakLength = sc.Subgraph[3];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume streaks";

        // Inputs
        NumberBarsStudy.Name = "Number bars study";
        NumberBarsStudy.SetStudyID(1);

        // Outputs
        StreakDirection.Name = "Streak direction";
        StreakDirection.DrawStyle = DRAWSTYLE_IGNORE;

        StreakHigh.Name = "Streak high";
        StreakHigh.DrawStyle = DRAWSTYLE_LINE;
        StreakHigh.PrimaryColor = RGB(0,255,0);

        StreakLow.Name = "Streak low";
        StreakLow.DrawStyle = DRAWSTYLE_LINE;
        StreakLow.PrimaryColor = RGB(0,255,0);

        CurrentStreakLength.Name = "Current streak length";
        CurrentStreakLength.DrawStyle = DRAWSTYLE_IGNORE;
    }
    const int i = sc.Index;

    if (StreakDirection[i] != 0) { // Makes sure we don't compute at every tick but just once per new bar
        return;
    }

    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);

    int& cumPosDeltaVol = sc.GetPersistentInt(1);
    int& cumNegDeltaVol = sc.GetPersistentInt(2);
    int& direction = sc.GetPersistentInt(3);

    double& highStreak = sc.GetPersistentDouble(1);
    double& lowStreak = sc.GetPersistentDouble(2);

    if (sc.IsNewTradingDay(i) || i <= 0) {
        cumPosDeltaVol = 0;
        cumNegDeltaVol = 0;
        direction = 0;
        highStreak = 0;
        lowStreak = 0;
        return;
    }

    if (AskVBidV[i-1] > 0) {
        cumPosDeltaVol++;
        direction = 1;
    } else{cumPosDeltaVol=0;}

    if (AskVBidV[i-1] < 0) {
        cumNegDeltaVol++;
        direction = -1;
    } else{cumNegDeltaVol=0;}

    const int streakLength = std::max<int>(cumPosDeltaVol, cumNegDeltaVol);
    CurrentStreakLength[i] = streakLength;

    highestLowestOfNBars(sc, streakLength, i-1, lowStreak, highStreak);
    StreakHigh[i-1] = highStreak;
    StreakLow[i-1] = lowStreak;
    StreakDirection[i] = direction;
}

SCSFExport scsf_EndOfNStreaksSignal(SCStudyInterfaceRef sc) {

    SCInputRef DVStreaksStudy = sc.Input[0];
    SCInputRef NumberBarsStudy = sc.Input[1];
    SCInputRef NStreak = sc.Input[2];
    SCInputRef StopLossBufferInTicks = sc.Input[3];


    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef StopLossPrice = sc.Subgraph[1];
    SCSubgraphRef FirstTargetInTicks = sc.Subgraph[2];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume end of N streaks signal";

        // Inputs
        DVStreaksStudy.Name = "Delta volume streaks study";
        DVStreaksStudy.SetStudyID(2);

        NumberBarsStudy.Name = "Number bars study";
        NumberBarsStudy.SetStudyID(1);

        NStreak.Name = "Minimum N Streaks to end";
        NStreak.SetIntLimits(3, 10);
        NStreak.SetInt(5);

        StopLossBufferInTicks.Name = "Stop loss buffer in ticks";
        StopLossBufferInTicks.SetIntLimits(0, 200);
        StopLossBufferInTicks.SetInt(0);

        // Outputs
        TradeSignal.Name = "Trade signal";
        TradeSignal.DrawStyle = DRAWSTYLE_IGNORE;

        StopLossPrice.Name = "Stop loss price";
        StopLossPrice.DrawStyle = DRAWSTYLE_IGNORE;

        FirstTargetInTicks.Name = "First target in ticks";
        FirstTargetInTicks.DrawStyle = DRAWSTYLE_IGNORE;

    }
    const int i = sc.Index;
    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);

    SCFloatArray StreakDirection;
    SCFloatArray StreakLow;
    SCFloatArray StreakHigh;
    SCFloatArray StreakLength;
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 0, StreakDirection);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 1, StreakHigh);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 2, StreakLow);
    sc.GetStudyArrayUsingID(DVStreaksStudy.GetStudyID(), 3, StreakLength);

    float& stopLossPrice = sc.GetPersistentFloat(1);
    float& targetInTicks = sc.GetPersistentFloat(2);


    if (StreakLength[i] >= NStreak.GetFloat()) {

        if (AskVBidV[i] > 0 && StreakDirection[i] < 0 && sc.Close[i] > sc.High[i-1]) {
            TradeSignal[i] = 1;
            stopLossPrice = std::min<float>(StreakLow[i-1], sc.Low[i]);
            StopLossPrice[i] = stopLossPrice - sc.TickSize * StopLossBufferInTicks.GetFloat();
            if (FirstTargetInTicks[i] == 0) {
                targetInTicks = (sc.Close[i] - sc.Low[i-1]) / sc.TickSize;
            }
        }
        if (AskVBidV[i] < 0 && StreakDirection[i] > 0 && sc.Close[i] < sc.Low[i-1]) {
            TradeSignal[i] = -1;
            stopLossPrice = std::max<float>(StreakHigh[i-1], sc.High[i]);
            StopLossPrice[i] = stopLossPrice + sc.TickSize * StopLossBufferInTicks.GetFloat();
            if (FirstTargetInTicks[i] == 0) {
                targetInTicks = (sc.High[i-1] - sc.Close[i]) / sc.TickSize;
            }
        }
    }

    FirstTargetInTicks[i] = targetInTicks;
}