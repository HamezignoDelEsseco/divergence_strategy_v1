#include "sierrachart.h"

SCSFExport scsf_ReversalCrossingVVA(SCStudyInterfaceRef sc) {
    // This is WIP (Strategy 9)

    SCInputRef LevelToCrossStudy = sc.Input[0];
    SCInputRef NBStudy = sc.Input[1];
    SCInputRef ATRStudy = sc.Input[2];
    SCInputRef UseATRRatioInsteadOfDV = sc.Input[3];
    SCInputRef DVThresh = sc.Input[4];
    SCInputRef ATRatio = sc.Input[5];
    SCInputRef SignalHitOffsetInTicks = sc.Input[6];
    SCInputRef StopLossOffsetInTicks = sc.Input[7];
    SCInputRef MaxRiskInTicks = sc.Input[8];
    SCInputRef UseLevelAsStop = sc.Input[9];


    SCSubgraphRef SignalDirection = sc.Subgraph[0];
    SCSubgraphRef SignalLocation = sc.Subgraph[1];
    SCSubgraphRef StopLoss = sc.Subgraph[2];
    SCSubgraphRef TradeId = sc.Subgraph[3];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "RV X lvl";

        LevelToCrossStudy.Name = "Level";
        LevelToCrossStudy.SetStudySubgraphValues(5,1);

        NBStudy.Name = "Number bars study";
        NBStudy.SetStudyID(1);

        ATRStudy.Name = "ATR Study";
        ATRStudy.SetStudyID(7);

        DVThresh.Name = "DV Threshold";
        DVThresh.SetIntLimits(0, 1000);
        DVThresh.SetInt(150);

        ATRatio.Name = "ATR ratio";
        ATRatio.SetFloatLimits(1, 1000);
        ATRatio.SetFloat(1.5);

        SignalHitOffsetInTicks.Name = "Signal hit offset (in ticks)";
        SignalHitOffsetInTicks.SetIntLimits(0, 50);
        SignalHitOffsetInTicks.SetInt(0);

        StopLossOffsetInTicks.Name = "Stop loss offset (in ticks)";
        StopLossOffsetInTicks.SetIntLimits(0, 50);
        StopLossOffsetInTicks.SetInt(0);

        MaxRiskInTicks.Name = "Maximum risk (in ticks)";
        MaxRiskInTicks.SetIntLimits(0, 50);
        MaxRiskInTicks.SetInt(10);

        UseLevelAsStop.Name = "Use level as stop";
        UseLevelAsStop.SetYesNo(0);

        UseATRRatioInsteadOfDV.Name = "Use ATR ratio instead of DV";
        UseATRRatioInsteadOfDV.SetYesNo(0);

        // Outputs
        SignalDirection.Name = "Dir";
        SignalDirection.DrawStyle = DRAWSTYLE_HIDDEN;

        SignalLocation.Name = "Loc";
        SignalLocation.DrawStyle = DRAWSTYLE_DASH;

        StopLoss.Name = "Stop";
        StopLoss.DrawStyle = DRAWSTYLE_DASH;

        TradeId.Name = "Tid";
        TradeId.DrawStyle = DRAWSTYLE_HIDDEN;

        sc.GraphRegion = 0;
        sc.ScaleRangeType = SCALE_SAMEASREGION;
    }

    const int currIdx = sc.Index;
    const int prevIdx = currIdx - 1;
    int& locationToSignal = sc.GetPersistentInt(1);
    float& signalDirection = sc.GetPersistentFloat(1);
    float& signalLocation = sc.GetPersistentFloat(2);
    float& stopLossPrice = sc.GetPersistentFloat(3);
    float& tradeId = sc.GetPersistentFloat(4);

    SCFloatArray AskVolBidVolDiff;
    SCFloatArray Level;
    SCFloatArray ATR;
    sc.GetStudyArrayUsingID(NBStudy.GetStudyID(), 0, AskVolBidVolDiff);
    sc.GetStudyArrayUsingID(ATRatio.GetStudyID(), 0, ATR);
    sc.GetStudyArrayUsingID(LevelToCrossStudy.GetStudyID(), LevelToCrossStudy.GetSubgraphIndex(), Level);


    SignalLocation[currIdx] = signalLocation;
    SignalDirection[currIdx] = signalDirection;
    StopLoss[currIdx] = stopLossPrice;
    TradeId[currIdx] = tradeId;

    locationToSignal = sc.Close[prevIdx-1] < Level[prevIdx-1] ? -1 : sc.Close[prevIdx-1] > Level[prevIdx-1] ? 1 : locationToSignal;

    // Only running the study if the bar has enough imbalance
    if (UseATRRatioInsteadOfDV.GetYesNo() == 0) {
        if (std::abs(AskVolBidVolDiff[prevIdx]) < DVThresh.GetFloat()) {
            return;
        }
    } else {
        if (std::abs(sc.Close[prevIdx] - sc.Open[prevIdx]) < ATRatio.GetFloat() * ATR[prevIdx]) {
            return;
        }
    }

    const int upDownBar = sc.Close[prevIdx] > sc.Open[prevIdx] ? 1 : -1;
    const bool crossFromBelow = locationToSignal == -1 && sc.Close[prevIdx] > Level[prevIdx] && upDownBar == 1;
    const bool crossFromAbove = locationToSignal == 1 && sc.Close[prevIdx] < Level[prevIdx] && upDownBar == -1;

    if (crossFromBelow) {
        signalLocation = sc.High[prevIdx];
        signalDirection = 1;
        tradeId = currIdx;
        stopLossPrice = std::max<float>(signalLocation - MaxRiskInTicks.GetFloat() * sc.TickSize, sc.Low[prevIdx])
        - StopLossOffsetInTicks.GetFloat() * sc.TickSize;
        if (UseLevelAsStop.GetYesNo() == 1) {
            stopLossPrice = Level[prevIdx]
            - StopLossOffsetInTicks.GetFloat() * sc.TickSize;
        }
    }
    if (crossFromAbove) {
        signalLocation = sc.Low[prevIdx];
        signalDirection = -1;
        tradeId = currIdx;
        stopLossPrice = std::min<float>(signalLocation + MaxRiskInTicks.GetFloat() * sc.TickSize, sc.High[prevIdx])
        + StopLossOffsetInTicks.GetFloat() * sc.TickSize;
        if (UseLevelAsStop.GetYesNo() == 1) {
            stopLossPrice = Level[prevIdx]
            + StopLossOffsetInTicks.GetFloat() * sc.TickSize;
        }
    }
}