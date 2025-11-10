#include "sierrachart.h"

SCSFExport scsf_BouncingOnVVA(SCStudyInterfaceRef sc) {
    // This study outputs whether the price is bouncing on a signal, either from above or from below

    SCInputRef BouncingSignal = sc.Input[0];
    SCInputRef NBStudy = sc.Input[1];
    SCInputRef SignalLevelOffsetInTicks = sc.Input[2];


    SCSubgraphRef SignalDirection = sc.Subgraph[0];
    SCSubgraphRef SignalLocation = sc.Subgraph[1];
    SCSubgraphRef AboveBelowSignal = sc.Subgraph[2];
    SCSubgraphRef IndexSwitchAboveBelow = sc.Subgraph[3];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Bouncing on a signal";

        BouncingSignal.Name = "Bouncing signal";
        BouncingSignal.SetStudySubgraphValues(5,1);

        NBStudy.Name = "Delta volume";
        NBStudy.SetStudySubgraphValues(1,0);

        SignalLevelOffsetInTicks.Name = "Buffer in ticks w.r.t signal";
        SignalLevelOffsetInTicks.SetIntLimits(0, 100);
        SignalLevelOffsetInTicks.SetInt(5);

        SignalDirection.Name = "Signal direction";
        SignalDirection.DrawStyle = DRAWSTYLE_LINE;

        SignalLocation.Name = "Signal location";
        SignalLocation.DrawStyle = DRAWSTYLE_LINE;

        AboveBelowSignal.Name = "Above / below signal";
        AboveBelowSignal.DrawStyle = DRAWSTYLE_LINE;

        IndexSwitchAboveBelow.Name = "Index switch above / below";
        IndexSwitchAboveBelow.DrawStyle = DRAWSTYLE_LINE;

        sc.GraphRegion = 1;

    }
    SCFloatArray AskVBidV;
    SCFloatArray Signal;
    sc.GetStudyArrayUsingID(NBStudy.GetStudyID(), NBStudy.GetSubgraphIndex(), AskVBidV);
    sc.GetStudyArrayUsingID(BouncingSignal.GetStudyID(), BouncingSignal.GetSubgraphIndex(), Signal);

    const int i = sc.Index;
    float& flagAboveBelowSignal = sc.GetPersistentFloat(1);
    float& indexSwitch = sc.GetPersistentFloat(2);
    float& signalLoc = sc.GetPersistentFloat(3);
    float& signalDir = sc.GetPersistentFloat(4);

    if (sc.High[i-1] > Signal[i-1] && sc.Low[i-1] > Signal[i-1] && flagAboveBelowSignal != 1) { // Floating bar above signal
        flagAboveBelowSignal = 1;
        signalLoc = 0;
        signalDir = 0;
        indexSwitch = i;
    }

    if (sc.High[i-1] < Signal[i-1] && sc.Low[i-1] < Signal[i-1] && flagAboveBelowSignal != -1) { // Floating bar below signal
        flagAboveBelowSignal = -1;
        signalLoc = 0;
        signalDir = 0;
        indexSwitch = i;
    }

    AboveBelowSignal[i] = flagAboveBelowSignal;
    IndexSwitchAboveBelow[i] = indexSwitch;

    if (flagAboveBelowSignal == 1 && AskVBidV[i-1] > 0 && sc.High[i] > sc.High[i-1] && sc.Low[i-1] <= Signal[i-1] + SignalLevelOffsetInTicks.GetFloat() * sc.TickSize) {
        signalLoc = sc.High[i];
        signalDir = 1;
    }

    if (flagAboveBelowSignal == -1 && AskVBidV[i-1] < 0 && sc.Low[i] < sc.Low[i-1] && sc.High[i-1] >= Signal[i-1] - SignalLevelOffsetInTicks.GetFloat() * sc.TickSize) {
        signalLoc = sc.Low[i];
        signalDir = -1;
    }

    SignalLocation[i] = signalLoc;
    SignalDirection[i] = signalDir;
}


SCSFExport scsf_VWAPLevelIndicator(SCStudyInterfaceRef sc) {
    // This study outputs whether the price is bouncing on a signal, either from above or from below

    SCInputRef VWAPStudy = sc.Input[0];

    SCSubgraphRef LookingBandForAboveBounce = sc.Subgraph[0];
    SCSubgraphRef LookingBandForBelowBounce = sc.Subgraph[1];
    SCSubgraphRef VWAPLevelAbove = sc.Subgraph[2];
    SCSubgraphRef VWAPLevelBelow = sc.Subgraph[3];
    SCSubgraphRef VWAPZoneIdx = sc.Subgraph[4];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "VWAP level indicator";

        VWAPStudy.Name = "VWAP study";
        VWAPStudy.SetStudyID(6);


        LookingBandForAboveBounce.Name = "Above band index";
        LookingBandForAboveBounce.DrawStyle = DRAWSTYLE_IGNORE;

        LookingBandForBelowBounce.Name = "Below band index";
        LookingBandForBelowBounce.DrawStyle = DRAWSTYLE_IGNORE;

        VWAPLevelAbove.Name = "VWAP R";
        VWAPLevelAbove.DrawStyle = DRAWSTYLE_DASH;
        VWAPLevelAbove.PrimaryColor = RGB(255,0,0);

        VWAPLevelBelow.Name = "VWAP S";
        VWAPLevelBelow.DrawStyle = DRAWSTYLE_DASH;
        VWAPLevelBelow.PrimaryColor = RGB(0,255,0);

        VWAPZoneIdx.Name = "Zone ID";
        VWAPZoneIdx.DrawStyle = DRAWSTYLE_IGNORE;

        sc.GraphRegion = 0;
        sc.ScaleRangeType = SCALE_SAMEASREGION;

    }
    const int currIdx = sc.Index;
    const int prevIdx = currIdx - 1;

    int& lastProcessedIDx = sc.GetPersistentInt(1);
    if (currIdx == lastProcessedIDx) {
        return;
    }

    const std::vector VWAPOrdered = {9, 7, 5, 3, 1, 2, 4, 6, 8};

    float& bandIdxBelow = sc.GetPersistentFloat(1);
    float& bandIdxAbove = sc.GetPersistentFloat(2);

    if (sc.IsNewTradingDay(currIdx)) {
        bandIdxBelow = -1;
        bandIdxAbove = -1;
        return;
    }

    // --- Build map: VWAP ID → index in VWAPBands
    std::unordered_map<int, size_t> idToPos;
    idToPos.reserve(VWAPOrdered.size());
    for (size_t i = 0; i < VWAPOrdered.size(); ++i)
        idToPos[VWAPOrdered[i]] = i;

    // Preload all VWAP bands (no repeated GetStudyArrayUsingID calls)
    std::vector<SCFloatArray> VWAPBands(VWAPOrdered.size());
    for (size_t i = 0; i < VWAPOrdered.size(); ++i)
        sc.GetStudyArrayUsingID(VWAPStudy.GetStudyID(), VWAPOrdered[i] - 1, VWAPBands[i]);

    const float low  = sc.Low[prevIdx];
    const float high = sc.High[prevIdx];

    // Below the lowest band
    if (high < VWAPBands.front()[prevIdx])
    {
        bandIdxBelow = -1.0f;
        bandIdxAbove = static_cast<float>(VWAPOrdered.front());
    }
    // Above the highest band
    else if (low > VWAPBands.back()[prevIdx])
    {
        bandIdxBelow = static_cast<float>(VWAPOrdered.back());
        bandIdxAbove = -1.0f;
    }
    // Between two bands
    else
    {
        for (size_t idx = 0; idx < VWAPOrdered.size() - 1; ++idx)
        {
            const SCFloatArray& VWAPBandL = VWAPBands[idx];
            const SCFloatArray& VWAPBandH = VWAPBands[idx + 1];

            float bandLow  = VWAPBandL[prevIdx];
            float bandHigh = VWAPBandH[prevIdx];

            if (low > bandLow && high < bandHigh)
            {
                bandIdxBelow = static_cast<float>(VWAPOrdered[idx]);
                bandIdxAbove = static_cast<float>(VWAPOrdered[idx + 1]);
                break;
            }
        }
    }

    LookingBandForBelowBounce[currIdx] = bandIdxBelow;
    LookingBandForAboveBounce[currIdx] = bandIdxAbove;
    VWAPZoneIdx[currIdx] = VWAPZoneIdx[prevIdx];

    if (LookingBandForBelowBounce[currIdx] != LookingBandForBelowBounce[prevIdx]) {
        VWAPZoneIdx[currIdx]++;
    }

    // Retrieving the levels
    if (bandIdxBelow != -1.0f)
    {
        int idBelow = static_cast<int>(bandIdxBelow);
        auto it = idToPos.find(idBelow);
        if (it != idToPos.end())
        {
            size_t pos = it->second;
            VWAPLevelBelow[currIdx] = VWAPBands[pos][currIdx];
        }
    }

    if (bandIdxAbove != -1.0f)
    {
        int idAbove = static_cast<int>(bandIdxAbove);
        auto it = idToPos.find(idAbove);
        if (it != idToPos.end())
        {
            size_t pos = it->second;
            VWAPLevelAbove[currIdx] = VWAPBands[pos][currIdx];
        }
    }

    lastProcessedIDx = currIdx;
}

SCSFExport scsf_VVALevelIndicator(SCStudyInterfaceRef sc) {
    // This study outputs whether the price is bouncing on a signal, either from above or from below

    SCInputRef VVAStudy = sc.Input[0];

    SCSubgraphRef LookingBandForAboveBounce = sc.Subgraph[0];
    SCSubgraphRef LookingBandForBelowBounce = sc.Subgraph[1];
    SCSubgraphRef VVALevelAbove = sc.Subgraph[2];
    SCSubgraphRef VVALevelBelow = sc.Subgraph[3];
    SCSubgraphRef VVAZoneIdx = sc.Subgraph[4];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "VVA level indicator";

        VVAStudy.Name = "VVA study";
        VVAStudy.SetStudyID(5);


        LookingBandForAboveBounce.Name = "Above band index";
        LookingBandForAboveBounce.DrawStyle = DRAWSTYLE_IGNORE;

        LookingBandForBelowBounce.Name = "Below band index";
        LookingBandForBelowBounce.DrawStyle = DRAWSTYLE_IGNORE;

        VVALevelAbove.Name = "VVA R";
        VVALevelAbove.DrawStyle = DRAWSTYLE_DASH;
        VVALevelAbove.PrimaryColor = RGB(171, 57, 3);

        VVALevelBelow.Name = "VVA S";
        VVALevelBelow.DrawStyle = DRAWSTYLE_DASH;
        VVALevelBelow.PrimaryColor = RGB(2, 130, 172);

        VVAZoneIdx.Name = "Zone ID";
        VVAZoneIdx.DrawStyle = DRAWSTYLE_IGNORE;

        sc.GraphRegion = 0;
        sc.ScaleRangeType = SCALE_SAMEASREGION;

    }
    const int currIdx = sc.Index;
    const int prevIdx = currIdx - 1;

    int& lastProcessedIDx = sc.GetPersistentInt(1);
    if (currIdx == lastProcessedIDx) {
        return;
    }

    const std::vector VVAOrdered = {3, 1, 2};

    float& bandIdxBelow = sc.GetPersistentFloat(1);
    float& bandIdxAbove = sc.GetPersistentFloat(2);

    if (sc.IsNewTradingDay(currIdx)) {
        bandIdxBelow = -1;
        bandIdxAbove = -1;
        return;
    }

    // --- Build map: VVA ID → index in VWAPBands
    std::unordered_map<int, size_t> idToPos;
    idToPos.reserve(VVAOrdered.size());
    for (size_t i = 0; i < VVAOrdered.size(); ++i)
        idToPos[VVAOrdered[i]] = i;

    // Preload all VVA bands (no repeated GetStudyArrayUsingID calls)
    std::vector<SCFloatArray> VVABands(VVAOrdered.size());
    for (size_t i = 0; i < VVAOrdered.size(); ++i)
        sc.GetStudyArrayUsingID(VVAStudy.GetStudyID(), VVAOrdered[i] - 1, VVABands[i]);

    const float low  = sc.Low[prevIdx];
    const float high = sc.High[prevIdx];

    // Below the lowest band
    if (high < VVABands.front()[prevIdx])
    {
        bandIdxBelow = -1.0f;
        bandIdxAbove = static_cast<float>(VVAOrdered.front());
    }
    // Above the highest band
    else if (low > VVABands.back()[prevIdx])
    {
        bandIdxBelow = static_cast<float>(VVAOrdered.back());
        bandIdxAbove = -1.0f;
    }
    // Between two bands
    else
    {
        for (size_t idx = 0; idx < VVAOrdered.size() - 1; ++idx)
        {
            const SCFloatArray& VVABandL = VVABands[idx];
            const SCFloatArray& VVABandH = VVABands[idx + 1];

            float bandLow  = VVABandL[prevIdx];
            float bandHigh = VVABandH[prevIdx];

            if (low > bandLow && high < bandHigh)
            {
                bandIdxBelow = static_cast<float>(VVAOrdered[idx]);
                bandIdxAbove = static_cast<float>(VVAOrdered[idx + 1]);
                break;
            }
        }
    }

    LookingBandForBelowBounce[currIdx] = bandIdxBelow;
    LookingBandForAboveBounce[currIdx] = bandIdxAbove;
    VVAZoneIdx[currIdx] = VVAZoneIdx[prevIdx];

    if (LookingBandForBelowBounce[currIdx] != LookingBandForBelowBounce[prevIdx]) {
        VVAZoneIdx[currIdx]++;
    }

    // Retrieving the levels
    if (bandIdxBelow != -1.0f)
    {
        int idBelow = static_cast<int>(bandIdxBelow);
        auto it = idToPos.find(idBelow);
        if (it != idToPos.end())
        {
            size_t pos = it->second;
            VVALevelBelow[currIdx] = VVABands[pos][currIdx];
        }
    }

    if (bandIdxAbove != -1.0f)
    {
        int idAbove = static_cast<int>(bandIdxAbove);
        auto it = idToPos.find(idAbove);
        if (it != idToPos.end())
        {
            size_t pos = it->second;
            VVALevelAbove[currIdx] = VVABands[pos][currIdx];
        }
    }

    lastProcessedIDx = currIdx;
}

SCSFExport scsf_VVABouncer(SCStudyInterfaceRef sc) {
    // This study outputs whether the price is bouncing on a signal, either from above or from below

    SCInputRef VVALevelsIndicator = sc.Input[0];
    SCInputRef NBStudy = sc.Input[1];

    SCInputRef OneSignalPerZone = sc.Input[2];
    SCInputRef SignalHitOffsetInTicks = sc.Input[3];
    SCInputRef StopLossOffsetInTicks = sc.Input[4];
    SCInputRef MaxRiskInTicks = sc.Input[5];

    SCSubgraphRef SignalDirection = sc.Subgraph[0];
    SCSubgraphRef StopLoss = sc.Subgraph[1];
    SCSubgraphRef FirstTargetInTicks = sc.Subgraph[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "VVA Bouncer";

        VVALevelsIndicator.Name = "VWAP indicator";
        VVALevelsIndicator.SetStudyID(6);

        NBStudy.Name = "Number bars study";
        NBStudy.SetStudyID(1);

        OneSignalPerZone.Name = "One signal per zone";
        OneSignalPerZone.SetYesNo(1);

        SignalHitOffsetInTicks.Name = "Signal hit offset (in ticks)";
        SignalHitOffsetInTicks.SetIntLimits(0, 50);
        SignalHitOffsetInTicks.SetInt(0);

        StopLossOffsetInTicks.Name = "Stop loss offset (in ticks)";
        StopLossOffsetInTicks.SetIntLimits(0, 50);
        StopLossOffsetInTicks.SetInt(0);

        MaxRiskInTicks.Name = "Maximum risk (in ticks)";
        MaxRiskInTicks.SetIntLimits(0, 50);
        MaxRiskInTicks.SetInt(10);

        // Outputs
        SignalDirection.Name = "Signal direction";
        SignalDirection.DrawStyle = DRAWSTYLE_HIDDEN;

        StopLoss.Name = "Stop loss price";
        StopLoss.DrawStyle = DRAWSTYLE_LINE;
        StopLoss.PrimaryColor = RGB(0, 128, 128);

        FirstTargetInTicks.Name = "First target (in ticks)";
        FirstTargetInTicks.DrawStyle = DRAWSTYLE_HIDDEN;

        sc.GraphRegion = 0;
        sc.ScaleRangeType = SCALE_SAMEASREGION;

    }
    const int currIdx = sc.Index;
    const int prevIdx = currIdx - 1;
    float& stopLoss = sc.GetPersistentFloat(1);
    float& stopLossInTicks = sc.GetPersistentFloat(2);


    SCFloatArray Support;
    SCFloatArray Resistance;
    SCFloatArray ZoneID;
    SCFloatArray AskVolBidVolDiff;
    sc.GetStudyArrayUsingID(NBStudy.GetStudyID(), 0, AskVolBidVolDiff);

    sc.GetStudyArrayUsingID(VVALevelsIndicator.GetStudyID(), 2, Resistance);
    sc.GetStudyArrayUsingID(VVALevelsIndicator.GetStudyID(), 3, Support);
    sc.GetStudyArrayUsingID(VVALevelsIndicator.GetStudyID(), 4, ZoneID);

    const float highBarTouches = sc.High[currIdx - 2] + SignalHitOffsetInTicks.GetFloat() * sc.TickSize;
    const float lowBarTouches = sc.Low[currIdx - 2] - SignalHitOffsetInTicks.GetFloat() * sc.TickSize;

    const bool sameZone = ZoneID[currIdx - 2] == ZoneID[currIdx - 1] && ZoneID[prevIdx] == ZoneID[currIdx];
    const bool touchesResistance = highBarTouches >= Resistance[currIdx-2] && lowBarTouches <= Resistance[currIdx-2];
    const bool touchesSupport = highBarTouches >= Support[currIdx-2] && lowBarTouches <= Support[currIdx-2];

    bool volCondition;
    bool uptickCondition;
    bool closeCondition;

    StopLoss[currIdx] = stopLoss;
    FirstTargetInTicks[currIdx] = stopLossInTicks;


    if (!sameZone){return;}
    if (!touchesResistance && !touchesSupport){return;}

    if (touchesResistance) {
        volCondition = AskVolBidVolDiff[currIdx - 2] > 0 && AskVolBidVolDiff[prevIdx] < 0;
        closeCondition = sc.Close[prevIdx] < Resistance[prevIdx];
        uptickCondition = sc.Close[currIdx] < sc.Close[prevIdx];
        if (volCondition && uptickCondition && closeCondition) {
            SignalDirection[currIdx] = -1;
            stopLoss = sc.High[prevIdx] + StopLossOffsetInTicks.GetFloat() * sc.TickSize;
            stopLossInTicks = (stopLoss - (sc.Close[prevIdx] - sc.TickSize)) / sc.TickSize;
            if (stopLossInTicks > MaxRiskInTicks.GetFloat()) {
                SignalDirection[currIdx] = 0;
            }
        }
    } else {
        volCondition = AskVolBidVolDiff[currIdx - 2] < 0 && AskVolBidVolDiff[prevIdx] > 0;
        closeCondition = sc.Close[prevIdx] > Support[prevIdx];
        uptickCondition = sc.Close[currIdx] > sc.Close[prevIdx];
        if (volCondition && uptickCondition && closeCondition) {
            SignalDirection[currIdx] = 1;
            stopLoss = sc.Low[prevIdx] - StopLossOffsetInTicks.GetFloat() * sc.TickSize;
            stopLossInTicks = (sc.Close[prevIdx] + sc.TickSize - stopLoss) / sc.TickSize;
            if (stopLossInTicks > MaxRiskInTicks.GetFloat()) {
                SignalDirection[currIdx] = 0;
            }
        }
    }


}

SCSFExport scsf_TrampolineTrader(SCStudyInterfaceRef sc) {
    // THIS IS STILL WIP

    SCInputRef VVALevelsIndicator = sc.Input[0];
    SCInputRef NBStudy = sc.Input[1];

    SCInputRef OneSignalPerZone = sc.Input[2];
    SCInputRef SignalHitOffsetInTicks = sc.Input[3];
    SCInputRef StopLossOffsetInTicks = sc.Input[4];
    SCInputRef MaxRiskInTicks = sc.Input[5];


    SCSubgraphRef SignalDirection = sc.Subgraph[0];
    SCSubgraphRef SignalLocation = sc.Subgraph[1];
    SCSubgraphRef StopLoss = sc.Subgraph[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "VVA Bouncer";

        VVALevelsIndicator.Name = "VWAP indicator";
        VVALevelsIndicator.SetStudyID(6);

        NBStudy.Name = "Number bars study";
        NBStudy.SetStudyID(1);

        OneSignalPerZone.Name = "One signal per zone";
        OneSignalPerZone.SetYesNo(1);

        SignalHitOffsetInTicks.Name = "Signal hit offset (in ticks)";
        SignalHitOffsetInTicks.SetIntLimits(0, 50);
        SignalHitOffsetInTicks.SetInt(0);

        StopLossOffsetInTicks.Name = "Stop loss offset (in ticks)";
        StopLossOffsetInTicks.SetIntLimits(0, 50);
        StopLossOffsetInTicks.SetInt(0);

        MaxRiskInTicks.Name = "Maximum risk (in ticks)";
        MaxRiskInTicks.SetIntLimits(0, 50);
        MaxRiskInTicks.SetInt(0);

        SignalDirection.Name = "Signal direction";
        SignalDirection.DrawStyle = DRAWSTYLE_BAR;
        SignalLocation.Name = "Signal location";
        StopLoss.Name = "Stop loss price";

        sc.GraphRegion = 1;

    }
    const int currIdx = sc.Index;
    const int prevIdx = currIdx - 1;
    float& stopLoss = sc.GetPersistentFloat(1);

    SCFloatArray Support;
    SCFloatArray Resistance;
    SCFloatArray ZoneID;
    SCFloatArray AskVolBidVolDiff;
    sc.GetStudyArrayUsingID(NBStudy.GetStudyID(), 0, AskVolBidVolDiff);

    sc.GetStudyArrayUsingID(VVALevelsIndicator.GetStudyID(), 2, Resistance);
    sc.GetStudyArrayUsingID(VVALevelsIndicator.GetStudyID(), 3, Support);
    sc.GetStudyArrayUsingID(VVALevelsIndicator.GetStudyID(), 4, ZoneID);

    const float highBarTouches = sc.High[currIdx - 2] + SignalHitOffsetInTicks.GetFloat() * sc.TickSize;
    const float lowBarTouches = sc.Low[currIdx - 2] - SignalHitOffsetInTicks.GetFloat() * sc.TickSize;

    const bool sameZone = ZoneID[currIdx - 2] == ZoneID[currIdx - 1] && ZoneID[prevIdx] == ZoneID[currIdx];
    const bool touchesResistance = highBarTouches >= Resistance[currIdx-2] && lowBarTouches <= Resistance[currIdx-2];
    const bool touchesSupport = highBarTouches >= Support[currIdx-2] && lowBarTouches <= Support[currIdx-2];

    bool volCondition;
    bool uptickCondition;
    bool closeCondition;
    if (!sameZone){return;}
    if (!touchesResistance && !touchesSupport){return;}

    if (touchesResistance) {
        volCondition = AskVolBidVolDiff[currIdx - 2] > 0 && AskVolBidVolDiff[prevIdx] < 0;
        closeCondition = sc.Close[prevIdx] < Resistance[prevIdx];
        uptickCondition = sc.Close[currIdx] < sc.Close[prevIdx];
        if (volCondition && uptickCondition && closeCondition) {
            SignalDirection[currIdx] = -1;
            stopLoss = sc.High[prevIdx];
        }
    } else {
        volCondition = AskVolBidVolDiff[currIdx - 2] < 0 && AskVolBidVolDiff[prevIdx] > 0;
        closeCondition = sc.Close[prevIdx] > Support[prevIdx];
        uptickCondition = sc.Close[currIdx] > sc.Close[prevIdx];
        if (volCondition && uptickCondition && closeCondition) {
            SignalDirection[currIdx] = 1;
            stopLoss = sc.Low[prevIdx];

        }
    }

}