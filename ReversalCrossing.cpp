#include "sierrachart.h"

SCSFExport scsf_ReversalCrossingVVA(SCStudyInterfaceRef sc) {
    // This is WIP (Strategy 9)

    SCInputRef VVAStudy = sc.Input[0];
    SCInputRef NBStudy = sc.Input[1];
    SCInputRef DVThresh = sc.Input[2];
    SCInputRef SignalHitOffsetInTicks = sc.Input[3];
    SCInputRef StopLossOffsetInTicks = sc.Input[4];
    SCInputRef MaxRiskInTicks = sc.Input[5];

    SCSubgraphRef SignalDirection = sc.Subgraph[0];
    SCSubgraphRef SignalLocation = sc.Subgraph[1];
    SCSubgraphRef StopLoss = sc.Subgraph[2];
    SCSubgraphRef FirstTargetInTicks = sc.Subgraph[3];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Reversal crossing the VVA";

        VVAStudy.Name = "VVA indicator";
        VVAStudy.SetStudySubgraphValues(5,1);

        NBStudy.Name = "Number bars study";
        NBStudy.SetStudyID(1);

        DVThresh.Name = "DV Threshold";
        DVThresh.SetIntLimits(0, 1000);
        DVThresh.SetInt(150);

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
    int& waitForUptick = sc.GetPersistentInt(1);
    float& priceToUptick = sc.GetPersistentFloat(1);

    int& waitForDownTick = sc.GetPersistentInt(2);
    float& priceToDownTick = sc.GetPersistentFloat(2);

    float& stopLoss = sc.GetPersistentFloat(3);
    float& stopLossInTicks = sc.GetPersistentFloat(4);


    SCFloatArray AskVolBidVolDiff;
    sc.GetStudyArrayUsingID(NBStudy.GetStudyID(), 0, AskVolBidVolDiff);


    SignalLocation[currIdx] = priceToUptick;
    // Only running the study if the bar has enough imbalance
    if (std::abs(AskVolBidVolDiff[prevIdx]) < DVThresh.GetFloat()) {
        return;
    }

    SCFloatArray VVA;
    sc.GetStudyArrayUsingID(VVAStudy.GetStudyID(), VVAStudy.GetSubgraphIndex(), VVA);

    if (sc.High[prevIdx] >= VVA[prevIdx] && sc.Close[prevIdx] > sc.Open[prevIdx]) {
        priceToUptick = sc.High[prevIdx];
        waitForUptick = 1;
    }

    if (sc.Close[currIdx] > priceToUptick) {
        SignalDirection[currIdx] = 1;
        waitForUptick = 0;
        priceToUptick = 0;
    }

    if (sc.Low[prevIdx] <= VVA[prevIdx] && sc.Close[prevIdx] > sc.Open[prevIdx]) {
        priceToUptick = sc.High[prevIdx];
        waitForUptick = 1;
    }

    if (sc.Close[currIdx] > priceToUptick) {
        SignalDirection[currIdx] = 1;
        waitForUptick = 0;
        priceToUptick=0;
    }


}