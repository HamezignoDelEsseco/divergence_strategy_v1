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
    SCInputRef MinDeltaVol = sc.Input[4];



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
        StopLossBufferInTicks.SetInt(1);

        MinDeltaVol.Name = "Minimum delta volume for signal";
        MinDeltaVol.SetIntLimits(0, 200);
        MinDeltaVol.SetInt(100);

        // Outputs
        TradeSignal.Name = "Trade signal";
        TradeSignal.DrawStyle = DRAWSTYLE_HIDDEN;

        StopLossPrice.Name = "Stop loss price";
        StopLossPrice.DrawStyle = DRAWSTYLE_HIDDEN;

        FirstTargetInTicks.Name = "First target in ticks";
        FirstTargetInTicks.DrawStyle = DRAWSTYLE_HIDDEN;

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

        if (AskVBidV[i] > MinDeltaVol.GetFloat() && StreakDirection[i] < 0 && sc.Close[i] > sc.High[i-1]) {
            TradeSignal[i] = 1;
            stopLossPrice = std::min<float>(StreakLow[i-1], sc.Low[i]) - sc.TickSize * StopLossBufferInTicks.GetFloat();
            targetInTicks = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize + 1;
        }
        if (AskVBidV[i] < MinDeltaVol.GetFloat()  && StreakDirection[i] > 0 && sc.Close[i] < sc.Low[i-1]) {
            TradeSignal[i] = -1;
            stopLossPrice = std::max<float>(StreakHigh[i-1], sc.High[i]) + sc.TickSize * StopLossBufferInTicks.GetFloat();
            // StopLossPrice[i] = stopLossPrice ;
            targetInTicks = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize + 1;
        }
    }
    StopLossPrice[i] = stopLossPrice;

    FirstTargetInTicks[i] = targetInTicks;
}

SCSFExport scsf_EndOfNStreaksSignalLag(SCStudyInterfaceRef sc) {

    SCInputRef DVStreaksStudy = sc.Input[0];
    SCInputRef NumberBarsStudy = sc.Input[1];

    SCInputRef NStreak = sc.Input[2];
    SCInputRef StopLossBufferInTicks = sc.Input[3];
    SCInputRef MaxRiskInTicks = sc.Input[4];


    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef StopLossPrice = sc.Subgraph[1];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume end of N streaks signal (lag)";

        // Inputs
        DVStreaksStudy.Name = "Delta volume streaks study";
        DVStreaksStudy.SetStudyID(2);

        NumberBarsStudy.Name = "Number bars study";
        NumberBarsStudy.SetStudyID(2);

        NStreak.Name = "Minimum N Streaks to end";
        NStreak.SetIntLimits(3, 10);
        NStreak.SetInt(5);

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
    const int prevIx = sc.Index - 1;
    const int currIx = sc.Index;

    if (currIx == 0 || sc.IsNewTradingDay(currIx)) {
        return;
    }

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
    float& stopLossInTicks = sc.GetPersistentFloat(2);
    float& worstEntryPrice = sc.GetPersistentFloat(3);


    if (StreakLength[prevIx] >= NStreak.GetFloat()) {

        if (
            StreakDirection[prevIx] < 0 // Ending a negative streak
            && sc.High[prevIx] > sc.High[prevIx-1] && AskVBidV[prevIx] > 0 // Closing the neg streak with a positive volume bar
            && sc.Close[currIx] > sc.High[prevIx] // Current bar upticks
            ) {
            worstEntryPrice = sc.High[prevIx] + sc.TickSize;
            stopLossPrice = std::min<float>(StreakLow[prevIx-1], sc.Low[prevIx]) - sc.TickSize * StopLossBufferInTicks.GetFloat();
            stopLossInTicks = (worstEntryPrice - stopLossPrice) / sc.TickSize;

            if (stopLossInTicks <= MaxRiskInTicks.GetFloat()) {
                TradeSignal[currIx] = 1;
            }
        }
        if (
            StreakDirection[prevIx] > 0 // Ending a positive streak
            && sc.Low[prevIx] < sc.Low[prevIx-1] && AskVBidV[prevIx] < 0 // Closing the pos streak with a negative volume bar
            && sc.Close[currIx] < sc.Low[prevIx] // Current bar downticks
            ) {
            worstEntryPrice = sc.Low[prevIx] - sc.TickSize;
            stopLossPrice = std::max<float>(StreakHigh[prevIx-1], sc.High[prevIx]) + sc.TickSize * StopLossBufferInTicks.GetFloat();
            stopLossInTicks = (-worstEntryPrice + stopLossPrice) / sc.TickSize;

            if (stopLossInTicks <= MaxRiskInTicks.GetFloat()) {
                TradeSignal[currIx] = -1;
            }
        }
    }
    StopLossPrice[currIx] = stopLossPrice;
}

SCSFExport scsf_ColorBreaksColor(SCStudyInterfaceRef sc) {

    SCInputRef NumberBarsStudy = sc.Input[0];
    SCInputRef FilterByLocalOpt = sc.Input[1];
    SCInputRef LocalOptBars = sc.Input[2];
    SCInputRef LocalOptOffsetInTicks = sc.Input[3];
    SCInputRef RiskTickOffset = sc.Input[4];
    SCInputRef MaximumRisk = sc.Input[5];
    SCInputRef AdjustMaxStopForAllTrades = sc.Input[6];

    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef StopLoss = sc.Subgraph[1];
    SCSubgraphRef Reverse = sc.Subgraph[2];
    SCSubgraphRef StopLossReversal = sc.Subgraph[3];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Color breaks color";

        NumberBarsStudy.Name = "Number bars calculated values study";
        NumberBarsStudy.SetStudyID(6);

        FilterByLocalOpt.Name = "Filter by local opt";
        FilterByLocalOpt.SetYesNo(1);

        // UseCleanTicksCondition.Name = "Use clean tick condition";
        // UseCleanTicksCondition.SetYesNo(0);

        LocalOptBars.Name = "Local opt of N bars";
        LocalOptBars.SetIntLimits(1, 100);
        LocalOptBars.SetInt(10);

        LocalOptOffsetInTicks.Name = "Buffer in ticks to local opt";
        LocalOptOffsetInTicks.SetIntLimits(0, 100);
        LocalOptOffsetInTicks.SetInt(5);

        RiskTickOffset.Name = "Stop limit offset in ticks";
        RiskTickOffset.SetIntLimits(0, 100);
        RiskTickOffset.SetInt(5);

        MaximumRisk.Name = "Max stop loss (in ticks)";
        MaximumRisk.SetIntLimits(5, 200);
        MaximumRisk.SetInt(50);

        AdjustMaxStopForAllTrades.Name = "Adjust max stop loss for all trades";
        AdjustMaxStopForAllTrades.SetYesNo(1);

        TradeSignal.Name = "Enter short signal";
        TradeSignal.DrawStyle = DRAWSTYLE_IGNORE;

        StopLoss.Name = "Stop loss";
        StopLoss.DrawStyle = DRAWSTYLE_LINE;

        Reverse.Name = "Reverse signal";
        Reverse.DrawStyle = DRAWSTYLE_IGNORE;

        StopLossReversal.Name = "Stop loss reversal";
        StopLossReversal.DrawStyle = DRAWSTYLE_LINE;

        sc.GraphRegion = 0;

    }
    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);
    const int i = sc.Index;
    bool isLocalLow = true;
    bool isLocalHigh = true;
    float& stopLoss = sc.GetPersistentFloat(1);
    float& stopLossReversal = sc.GetPersistentFloat(2);
    float stopLossInTicks;


    const float highestOfN = sc.GetHighest(sc.BaseData[SC_HIGH], i-1, LocalOptBars.GetInt());
    const float lowestOfN = sc.GetLowest(sc.BaseData[SC_LOW], i-1, LocalOptBars.GetInt());

    if (FilterByLocalOpt.GetYesNo() == 1) {
        isLocalHigh = sc.High[i-1] >= highestOfN - LocalOptOffsetInTicks.GetFloat() * sc.TickSize || sc.High[i-2] >= highestOfN - LocalOptOffsetInTicks.GetFloat() * sc.TickSize;
        isLocalLow = sc.Low[i-1] <= lowestOfN + LocalOptOffsetInTicks.GetFloat() * sc.TickSize || sc.Low[i-2] <= lowestOfN + LocalOptOffsetInTicks.GetFloat() * sc.TickSize;
    }

    if (
        AskVBidV[i-2] > 0 && AskVBidV[i-1] < 0
        && sc.Low[i] < sc.Close[i-1] && isLocalHigh
        ) {
        stopLoss = sc.High[i-1] + RiskTickOffset.GetFloat() * sc.TickSize;
        stopLossInTicks = (stopLoss - sc.Open[i]) / sc.TickSize;
        TradeSignal[i] = -1;

        if (stopLossInTicks > MaximumRisk.GetFloat()) {
            if (AdjustMaxStopForAllTrades.GetYesNo() == 1) {
                stopLoss = sc.Open[i] + MaximumRisk.GetFloat() * sc.TickSize;
            } else {
                TradeSignal[i] = 0;
            }
        }
    }
    if (
        AskVBidV[i-2] < 0  && AskVBidV[i-1] > 0
        && sc.High[i] > sc.Close[i-1] && isLocalLow
    ) {
        stopLoss = sc.Low[i-1] - RiskTickOffset.GetFloat() * sc.TickSize;
        stopLossInTicks = (sc.Open[i] - stopLoss) / sc.TickSize;
        TradeSignal[i] = 1;

        if (stopLossInTicks > MaximumRisk.GetFloat()) {
            if (AdjustMaxStopForAllTrades.GetYesNo() == 1) {
                stopLoss = sc.Open[i] - MaximumRisk.GetFloat() * sc.TickSize;
            } else {
                TradeSignal[i] = 0;
            }
        }
    }

    StopLoss[i] = stopLoss;

    double previousEstimatedEntry, previousEstimatedStopLossInTicks;
    const double reversalEstimatedEntry = sc.Open[i];
    if (TradeSignal[i-2] == 1 && sc.Low[i-1] < sc.Low[i-2] && sc.High[i-1] < sc.High[i-2] && AskVBidV[i-1] < 0) {
        //previousEstimatedEntry = sc.High[i-3] + sc.TickSize;
        //previousEstimatedStopLossInTicks = (previousEstimatedEntry - StopLoss[i-2]) / sc.TickSize;
        stopLossReversal = sc.Open[i] + MaximumRisk.GetInt() * sc.TickSize;
        Reverse[i] = -1;
    }

    if (TradeSignal[i-2] == -1 && sc.Low[i-1] > sc.Low[i-2] && sc.High[i-1] > sc.High[i-2] && AskVBidV[i-1] > 0) {
        //previousEstimatedEntry = sc.Low[i-3] - sc.TickSize;
        //previousEstimatedStopLossInTicks = (StopLoss[i-2] - previousEstimatedEntry) / sc.TickSize;
        stopLossReversal = sc.Open[i] - MaximumRisk.GetInt() * sc.TickSize;
        Reverse[i] = 1;
    }
    StopLossReversal[i] = stopLossReversal;
}
