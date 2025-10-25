#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_DeltaVolShortSignal(SCStudyInterfaceRef sc) {

    SCInputRef NumPreceedingPosBars = sc.Input[0];
    SCInputRef MaxEntryBarTickSize = sc.Input[1];
    SCInputRef HighestOfNBars = sc.Input[2];
    SCInputRef MaxTicksFronDayHighest = sc.Input[3];
    SCInputRef NumberBarsStudy = sc.Input[4];


    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef Stop = sc.Subgraph[1];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume short signal";

        // The Study
        NumPreceedingPosBars.Name = "Number of preceeding bars with pos delta";
        NumPreceedingPosBars.SetIntLimits(0, 10);
        NumPreceedingPosBars.SetInt(4);

        MaxEntryBarTickSize.Name = "Maximum tick size of entry bar";
        MaxEntryBarTickSize.SetIntLimits(0, 30);
        MaxEntryBarTickSize.SetInt(15);


        HighestOfNBars.Name = "Highest of N bars";
        HighestOfNBars.SetIntLimits(0, 30);
        HighestOfNBars.SetInt(10);

        MaxTicksFronDayHighest.Name = "Max ticks from day highest";
        MaxTicksFronDayHighest.SetIntLimits(-1, 1000);
        MaxTicksFronDayHighest.SetInt(-1);

        NumberBarsStudy.Name = "Number bars calculated values study";
        NumberBarsStudy.SetStudyID(6);

        TradeSignal.Name = "Enter short signal";
        TradeSignal.DrawStyle = DRAWSTYLE_IGNORE;

        Stop.Name = "Stop loss price";
        Stop.DrawStyle = DRAWSTYLE_IGNORE;
    }
    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);
    const int i = sc.Index;

    int& cumPosDeltaVol = sc.GetPersistentInt(1);
    double& stopLoss = sc.GetPersistentDouble(1);

    if (AskVBidV[i-2] > 0) {
        cumPosDeltaVol++;
    } else{cumPosDeltaVol=0;}

    // We assume the chart has well calibrated delta volume bars for the symbol
    bool preceedingCondition = cumPosDeltaVol >= NumPreceedingPosBars.GetInt();
    bool negDeltaVolCondition = AskVBidV[i-1] < 0;
    bool highestCondition = highestOfNBars(sc, HighestOfNBars.GetInt(), i-1);
    bool barSizeCondition = (sc.High[i-1] - sc.Low[i-1]) / sc.TickSize <= MaxEntryBarTickSize.GetInt();
    bool downTickCondition = (sc.Low[i] < sc.Low[i-1]);

    if (highestCondition & preceedingCondition & negDeltaVolCondition & barSizeCondition & downTickCondition) {
        TradeSignal[i] = 1;
    }
    if (TradeSignal[i] == 1) {
        stopLoss = sc.High[i-1] + sc.TickSize; // Small buffer for greed :D
    }
    Stop[i] = static_cast<float>(stopLoss);
}


SCSFExport scsf_DeltaVolShortTrader5(SCStudyInterfaceRef sc) {

    // Trading with max 5 contracts:

    SCInputRef SignalStudy = sc.Input[0];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Delta volume trader 5 contracts";

        // Inputs
        SignalStudy.Name = "Spignal study";
        SignalStudy.SetStudyID(2);

        // Outputs

    }
}