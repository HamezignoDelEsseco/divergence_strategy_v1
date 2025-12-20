#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_DeltaVolumeMomentumAlex(SCStudyInterfaceRef sc) {

    SCInputRef DeltaVolumeStudy = sc.Input[0];

    SCSubgraphRef BuySignal = sc.Subgraph[0];
    SCSubgraphRef SellSignal = sc.Subgraph[1];
    SCSubgraphRef Signal = sc.Subgraph[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Delta Volume Momentum Alex";

        DeltaVolumeStudy.Name = "Delta Volume Study";
        DeltaVolumeStudy.SetStudyID(1);



        BuySignal.Name = "Buy Signal";
        BuySignal.DrawStyle = DRAWSTYLE_LINE;
        BuySignal.PrimaryColor = RGB(0, 255, 0);
        BuySignal.LineWidth = 2;

        SellSignal.Name = "Sell Signal";
        SellSignal.DrawStyle = DRAWSTYLE_LINE;
        SellSignal.PrimaryColor = RGB(255, 0, 0);
        SellSignal.LineWidth = 2;

        Signal.Name = "Signal";
        Signal.DrawStyle = DRAWSTYLE_BAR;
        Signal.PrimaryColor = RGB(255, 255, 0);

        return;
    }

    const int i = sc.Index;

    // Get delta volume from the specified study
    SCFloatArray DeltaVolume;
    sc.GetStudyArrayUsingID(DeltaVolumeStudy.GetStudyID(), 0, DeltaVolume);

    // Initialize all signals to 0 first
    Signal[i] = 0;
    BuySignal[i] = 0;
    SellSignal[i] = 0;

    if (i == 0) {
        return;
    }



    // Short signal: Candle closes down and delta volume <= -600
    const bool candleClosedDown = sc.Close[i] < sc.Open[i];
    const bool shortCondition = candleClosedDown && DeltaVolume[i] < 0;

    // Long signal: Candle closes up and delta volume >= 600
    const bool candleClosedUp = sc.Close[i] > sc.Open[i];
    const bool longCondition = candleClosedUp && DeltaVolume[i] >0 ;

    if (shortCondition) {
        Signal[i] = -1;
        SellSignal[i] = sc.High[i];
    }
    else if (longCondition) {
        Signal[i] = 1;
        BuySignal[i] = sc.Low[i];
    }
    // else: signals already set to 0 above
}
