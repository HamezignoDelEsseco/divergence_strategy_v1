#include "sierrachart.h"

SCSFExport scsf_VTISignal(SCStudyInterfaceRef sc) {
    SCInputRef VTIStudy = sc.Input[0];
    SCInputRef MaxStopInTicks = sc.Input[1];
    SCInputRef RemoveRiskySignals = sc.Input[2];

    SCSubgraphRef TradeEnterSignal = sc.Subgraph[0];
    SCSubgraphRef StopPrice = sc.Subgraph[1];
    SCSubgraphRef RiskCompliantLimitEntryPrice = sc.Subgraph[2];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "VTI - Signal";

        VTIStudy.Name = "VTIStudy";
        VTIStudy.SetStudyID(7);

        MaxStopInTicks.Name = "Max stop in ticks";
        MaxStopInTicks.SetIntLimits(5, 1000);
        MaxStopInTicks.SetInt(20);

        RemoveRiskySignals.Name = "Remove risky signal";
        RemoveRiskySignals.SetYesNo(0);

        TradeEnterSignal.Name = "Trade enter signal";
        TradeEnterSignal.DrawStyle = DRAWSTYLE_HIDDEN;

        StopPrice.Name = "Stop price";
        StopPrice.DrawStyle = DRAWSTYLE_HIDDEN;

        RiskCompliantLimitEntryPrice.Name = "Risk compliant limit entry price";
        RiskCompliantLimitEntryPrice.DrawStyle = DRAWSTYLE_LINE;

        sc.GraphRegion = 0;

    }
    const int i = sc.Index;
    float& riskCompliantEntryPrice = sc.GetPersistentFloat(1);
    SCFloatArrayRef InstantSignal = TradeEnterSignal.Arrays[0];
    SCFloatArray VTI;
    sc.GetStudyArrayUsingID(VTIStudy.GetStudyID(),0, VTI);

    StopPrice[i] = VTI[i];

    if (sc.Open[i-1] > VTI[i-1] && sc.Close[i-1] > VTI[i-1]) {
        InstantSignal[i] = 1;

    }
    if (sc.Open[i-1] < VTI[i-1] && sc.Close[i-1] < VTI[i-1]) {
        InstantSignal[i] = -1;

    }

    if (InstantSignal[i] != 0) {
        TradeEnterSignal[i] = InstantSignal[i-1] != InstantSignal[i] ? InstantSignal[i] : 0;
    }

    if (
        TradeEnterSignal[i] != 0 && RemoveRiskySignals.GetYesNo() == 1
        && std::abs(sc.Open[i] - StopPrice[i]) / sc.TickSize > MaxStopInTicks.GetFloat()
        ) {
        TradeEnterSignal[i] = 0;
    }

    if (TradeEnterSignal[i] == 1) {
        riskCompliantEntryPrice = VTI[i] + MaxStopInTicks.GetFloat() * sc.TickSize;
    }

    if (TradeEnterSignal[i] == -1) {
        riskCompliantEntryPrice = VTI[i] - MaxStopInTicks.GetFloat() * sc.TickSize;
    }

    RiskCompliantLimitEntryPrice[i] = riskCompliantEntryPrice;
}