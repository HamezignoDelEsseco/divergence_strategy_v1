#include "sierrachart.h"

SCSFExport scsf_VASimpleCrossAlex(SCStudyInterfaceRef sc) {
    /*
     * VA Simple Cross Signal
     *
     * 4 configurable cross signals:
     * 1. Cross UP through VAH (breakout long)
     * 2. Cross DOWN through VAH (re-entry short)
     * 3. Cross DOWN through VAL (breakout short)
     * 4. Cross UP through VAL (re-entry long)
     */

    SCInputRef ValueAreaStudy = sc.Input[0];
    SCInputRef StartTradingTime = sc.Input[1];
    SCInputRef EndTradingTime = sc.Input[2];
    SCInputRef EnableCrossUpVAH = sc.Input[3];      // Long - breakout above VAH
    SCInputRef EnableCrossDownVAH = sc.Input[4];    // Short - re-entry from above VAH
    SCInputRef EnableCrossDownVAL = sc.Input[5];    // Short - breakout below VAL
    SCInputRef EnableCrossUpVAL = sc.Input[6];      // Long - re-entry from below VAL

    SCSubgraphRef Signal = sc.Subgraph[0];
    SCSubgraphRef BuySignal = sc.Subgraph[1];
    SCSubgraphRef SellSignal = sc.Subgraph[2];
    SCSubgraphRef CrossType = sc.Subgraph[3];  // 1=CrossUpVAH, 2=CrossDownVAH, 3=CrossDownVAL, 4=CrossUpVAL

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "VA Simple Cross Alex";

        ValueAreaStudy.Name = "Value Area Study (VAH=SG1, VAL=SG2)";
        ValueAreaStudy.SetStudyID(1);

        StartTradingTime.Name = "Start Trading Time";
        StartTradingTime.SetTime(HMS_TIME(8, 0, 0));

        EndTradingTime.Name = "End Trading Time";
        EndTradingTime.SetTime(HMS_TIME(15, 45, 0));

        EnableCrossUpVAH.Name = "Enable Cross UP VAH (Long)";
        EnableCrossUpVAH.SetYesNo(1);
        EnableCrossUpVAH.SetDescription("Long when: Open <= VAH, Close >= VAH");

        EnableCrossDownVAH.Name = "Enable Cross DOWN VAH (Short)";
        EnableCrossDownVAH.SetYesNo(1);
        EnableCrossDownVAH.SetDescription("Short when: Open >= VAH, Close <= VAH");

        EnableCrossDownVAL.Name = "Enable Cross DOWN VAL (Short)";
        EnableCrossDownVAL.SetYesNo(1);
        EnableCrossDownVAL.SetDescription("Short when: Open >= VAL, Close <= VAL");

        EnableCrossUpVAL.Name = "Enable Cross UP VAL (Long)";
        EnableCrossUpVAL.SetYesNo(1);
        EnableCrossUpVAL.SetDescription("Long when: Open <= VAL, Close >= VAL");

        Signal.Name = "Signal";
        Signal.DrawStyle = DRAWSTYLE_BAR;
        Signal.PrimaryColor = RGB(255, 255, 0);
        Signal.LineWidth = 2;

        BuySignal.Name = "Buy Signal";
        BuySignal.DrawStyle = DRAWSTYLE_POINT_ON_HIGH;
        BuySignal.PrimaryColor = RGB(0, 255, 0);
        BuySignal.LineWidth = 3;

        SellSignal.Name = "Sell Signal";
        SellSignal.DrawStyle = DRAWSTYLE_POINT_ON_LOW;
        SellSignal.PrimaryColor = RGB(255, 0, 0);
        SellSignal.LineWidth = 3;

        CrossType.Name = "Cross Type";
        CrossType.DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int i = sc.Index;

    // Initialize signals
    Signal[i] = 0;
    BuySignal[i] = 0;
    SellSignal[i] = 0;
    CrossType[i] = 0;

    // Skip the currently forming bar
    if (i == sc.ArraySize - 1) {
        return;
    }

    if (i < 1) {
        return;
    }

    // Get Value Area levels from study
    SCFloatArray VAH;
    SCFloatArray VAL;
    sc.GetStudyArrayUsingID(ValueAreaStudy.GetStudyID(), 1, VAH);
    sc.GetStudyArrayUsingID(ValueAreaStudy.GetStudyID(), 2, VAL);

    if (VAH[i] == 0 || VAL[i] == 0) {
        return;
    }

    // Check if within trading hours
    SCDateTime currentTime = sc.BaseDateTimeIn[i].GetTime();
    if (currentTime < StartTradingTime.GetTime() || currentTime > EndTradingTime.GetTime()) {
        return;
    }

    const float open = sc.Open[i];
    const float close = sc.Close[i];
    const float high = sc.High[i];
    const float low = sc.Low[i];

    // 1. Cross UP through VAH (Long - breakout)
    // Open <= VAH, Close >= VAH
    if (EnableCrossUpVAH.GetYesNo() && open <= VAH[i] && close >= VAH[i]) {
        Signal[i] = 1;
        BuySignal[i] = low;
        CrossType[i] = 1;
        return;
    }

    // 2. Cross DOWN through VAH (Short - re-entry)
    // Open >= VAH, Close <= VAH
    if (EnableCrossDownVAH.GetYesNo() && open >= VAH[i] && close <= VAH[i]) {
        Signal[i] = -1;
        SellSignal[i] = high;
        CrossType[i] = 2;
        return;
    }

    // 3. Cross DOWN through VAL (Short - breakout)
    // Open >= VAL, Close <= VAL
    if (EnableCrossDownVAL.GetYesNo() && open >= VAL[i] && close <= VAL[i]) {
        Signal[i] = -1;
        SellSignal[i] = high;
        CrossType[i] = 3;
        return;
    }

    // 4. Cross UP through VAL (Long - re-entry)
    // Open <= VAL, Close >= VAL
    if (EnableCrossUpVAL.GetYesNo() && open <= VAL[i] && close >= VAL[i]) {
        Signal[i] = 1;
        BuySignal[i] = low;
        CrossType[i] = 4;
        return;
    }
}
