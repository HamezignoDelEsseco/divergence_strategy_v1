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

}



SCSFExport scsf_FollowTheNStreakSignal(SCStudyInterfaceRef sc) {

    SCInputRef AskVBidVStudy = sc.Input[0];
    SCInputRef NStreak = sc.Input[1];
    SCInputRef RealStreaksOnly = sc.Input[2];
    SCInputRef StartAtSignal = sc.Input[3];
    SCInputRef MaxSignalsToLookAt = sc.Input[4];

    SCInputRef SignalToStartAt = sc.Input[5];
    SCInputRef SignalToStartAt2 = sc.Input[6];

    SCInputRef StartAtLocalOpt = sc.Input[7];
    SCInputRef LocalOptOfNBar = sc.Input[8];
    SCInputRef UseFullStreakForStop = sc.Input[9];
    SCInputRef UseLastStreakBarForStop = sc.Input[10];
    SCInputRef UseMaxRiskInTicks = sc.Input[11];
    SCInputRef MaxRiskInTicks = sc.Input[12];


    SCSubgraphRef StreakSignal = sc.Subgraph[0];
    SCSubgraphRef StopLossPrice = sc.Subgraph[1];
    SCSubgraphRef StreakHigh = sc.Subgraph[2];
    SCSubgraphRef StreakLow = sc.Subgraph[3];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Follow the N streak signal";

        // Inputs
        AskVBidVStudy.Name = "Delta volume study";
        AskVBidVStudy.SetStudySubgraphValues(5,1);

        NStreak.Name = "Streak length to flag";
        NStreak.SetIntLimits(0, 200);
        NStreak.SetInt(3);

        RealStreaksOnly.Name = "Real streak only";
        RealStreaksOnly.SetYesNo(1);

        StartAtSignal.Name = "Use only streaks that touch a signal";
        StartAtSignal.SetYesNo(0);

        MaxSignalsToLookAt.Name = "Maximum signals to consider";
        MaxSignalsToLookAt.SetIntLimits(1,2);
        MaxSignalsToLookAt.SetInt(2);

        SignalToStartAt.Name = "Signal1";
        SignalToStartAt.SetStudySubgraphValues(5,1);

        SignalToStartAt2.Name = "Signal2";
        SignalToStartAt2.SetStudySubgraphValues(5,2);

        StartAtLocalOpt.Name = "Use only streaks that start at a local min/max";
        StartAtLocalOpt.SetYesNo(0);

        LocalOptOfNBar.Name = "Window size for local min/mac computation";
        LocalOptOfNBar.SetIntLimits(5, 200);
        LocalOptOfNBar.SetInt(20);

        UseFullStreakForStop.Name = "Max of streak is used for stop";
        UseFullStreakForStop.SetYesNo(0);

        UseLastStreakBarForStop.Name = "Last streak bar is used for stoph";
        UseLastStreakBarForStop.SetYesNo(0);

        MaxRiskInTicks.Name = "Whether to cap the stop using a max number of ticks";
        MaxRiskInTicks.SetYesNo(0);

        MaxRiskInTicks.Name = "Maximum number of ticks for stop loss";
        MaxRiskInTicks.SetIntLimits(5, 200);
        MaxRiskInTicks.SetInt(30);

        // Outputs
        StreakSignal.Name = "N Streak signal";
        StreakSignal.DrawStyle = DRAWSTYLE_LINE;

        StopLossPrice.Name = "Stop loss price";
        StopLossPrice.DrawStyle = DRAWSTYLE_LINE;

        StreakHigh.Name = "Streak high";
        StreakHigh.DrawStyle = DRAWSTYLE_LINE;

        StreakLow.Name = "Streak low";
        StreakLow.DrawStyle = DRAWSTYLE_LINE;

        sc.GraphRegion = 1;
    }

    const int i = sc.Index;
    SCFloatArrayRef streakSizePosVol = StreakSignal.Arrays[0];
    SCFloatArrayRef streakSizeNegVol = StreakSignal.Arrays[1];
    SCFloatArrayRef streakSizePosIncr = StreakSignal.Arrays[2];
    SCFloatArrayRef streakSizeNegIncr = StreakSignal.Arrays[3];
    SCFloatArrayRef cumulativeDeltaVolPos = StreakSignal.Arrays[4];
    SCFloatArrayRef cumulativeDeltaVolNeg = StreakSignal.Arrays[5];

    SCFloatArrayRef cumulativeIncrPos = StreakSignal.Arrays[6];
    SCFloatArrayRef cumulativeIncrNeg = StreakSignal.Arrays[7];


    if (sc.IsNewTradingDay(i) || i<NStreak.GetInt()) {
        streakSizePosVol[i] = 0;
        streakSizeNegVol[i] = 0;
        streakSizePosIncr[i] = 0;
        streakSizeNegIncr[i] = 0;
        return;
    }

    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(AskVBidVStudy.GetStudyID(), AskVBidVStudy.GetSubgraphIndex(), AskVBidV);


    // Computing this only when unset
    if (streakSizePosVol[i-1] == 0 && streakSizeNegVol[i-1] == 0) {

        streakSizePosVol[i-1] = AskVBidV[i-1] > 0 ? 1 : 0;
        streakSizeNegVol[i-1] = AskVBidV[i-1] < 0 ? 1 : 0;

        streakSizePosIncr[i-1] = sc.High[i-1] > sc.High[i-2] && sc.Low[i-1] > sc.Low[i-2] ? 1 : 0;
        streakSizeNegIncr[i-1] = sc.High[i-1] < sc.High[i-2] && sc.Low[i-1] < sc.Low[i-2] ? 1 : 0;

        for (int idx = 0; idx < NStreak.GetInt(); idx++) {
            cumulativeDeltaVolPos[i-1] += streakSizePosVol[i-1-idx];
            cumulativeDeltaVolNeg[i-1] += streakSizeNegVol[i-1-idx];
            cumulativeIncrPos[i-1] += streakSizePosIncr[i-1-idx];
            cumulativeIncrNeg[i-1] += streakSizeNegIncr[i-1-idx];
        }

        StreakHigh[i] = sc.GetHighest(sc.BaseData[SC_HIGH], i-1, NStreak.GetInt());
        StreakLow[i] = sc.GetLowest(sc.BaseData[SC_LOW], i-1, NStreak.GetInt());

        if (RealStreaksOnly.GetYesNo()==1) {
            StreakSignal[i] = cumulativeIncrPos[i-1] >=  NStreak.GetInt() - 1 && cumulativeDeltaVolPos[i-1] >= NStreak.GetInt() ? 1 :
            cumulativeIncrNeg[i-1] >=  (NStreak.GetInt() - 1) && cumulativeDeltaVolNeg[i-1] >= NStreak.GetInt() ? -1 : 0;
        } else {
            StreakSignal[i] = cumulativeDeltaVolPos[i-1] >= NStreak.GetInt() ? 1 :cumulativeDeltaVolNeg[i-1] >= NStreak.GetInt() ? -1 : 0;
        }

        if (StartAtSignal.GetYesNo() == 1 && StreakSignal[i] != 0) {
            SCFloatArray Signal;
            sc.GetStudyArrayUsingID(SignalToStartAt.GetStudyID(), SignalToStartAt.GetSubgraphIndex(), Signal);
            bool condition = Signal[i] < StreakLow[i] || Signal[i] > StreakHigh[i];

            if (MaxSignalsToLookAt.GetInt() >= 2) {
                SCFloatArray Signal2;
                sc.GetStudyArrayUsingID(SignalToStartAt2.GetStudyID(), SignalToStartAt2.GetSubgraphIndex(), Signal2);
                condition = condition &&  Signal2[i] < StreakLow[i] || Signal2[i] > StreakHigh[i];
            }

            if (condition) {
                StreakSignal[i] = 0;
            }
        }

    }

}