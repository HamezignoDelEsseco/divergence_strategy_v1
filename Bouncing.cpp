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