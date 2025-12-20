#include "helpers.h"
#include "sierrachart.h"

/*
 * MOMENTUM AFTER STRIKE ALEX
 *
 * Signal uniquement après 3 bougies consécutives dans la même direction
 * + confirmation delta volume
 *
 * LONG:
 *   - 3 bougies haussières consécutives (Close > Open)
 *   - Delta volume positive sur la 3ème bougie
 *
 * SHORT:
 *   - 3 bougies baissières consécutives (Close < Open)
 *   - Delta volume négative sur la 3ème bougie
 */

SCSFExport scsf_MomentumAfterStrikeAlex(SCStudyInterfaceRef sc) {

    SCInputRef DeltaVolumeStudy = sc.Input[0];
    SCInputRef ConsecutiveCandles = sc.Input[1];

    SCSubgraphRef BuySignal = sc.Subgraph[0];
    SCSubgraphRef SellSignal = sc.Subgraph[1];
    SCSubgraphRef Signal = sc.Subgraph[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Momentum After Strike Alex";

        DeltaVolumeStudy.Name = "Delta Volume Study";
        DeltaVolumeStudy.SetStudyID(1);

        ConsecutiveCandles.Name = "Consecutive Candles Required";
        ConsecutiveCandles.SetInt(3);
        ConsecutiveCandles.SetIntLimits(2, 10);
        ConsecutiveCandles.SetDescription("Number of consecutive candles in same direction");

        BuySignal.Name = "Buy Signal";
        BuySignal.DrawStyle = DRAWSTYLE_ARROW_UP;
        BuySignal.PrimaryColor = RGB(0, 255, 0);
        BuySignal.LineWidth = 3;
        BuySignal.DrawZeros = false;

        SellSignal.Name = "Sell Signal";
        SellSignal.DrawStyle = DRAWSTYLE_ARROW_DOWN;
        SellSignal.PrimaryColor = RGB(255, 0, 0);
        SellSignal.LineWidth = 3;
        SellSignal.DrawZeros = false;

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

    const int requiredCandles = ConsecutiveCandles.GetInt();

    // Need at least N bars for consecutive check
    if (i < requiredCandles - 1) {
        return;
    }

    // Check for N consecutive up candles
    bool consecutiveUpCandles = true;
    for (int j = 0; j < requiredCandles; j++) {
        if (sc.Close[i-j] <= sc.Open[i-j]) {
            consecutiveUpCandles = false;
            break;
        }
    }

    // Check for N consecutive down candles
    bool consecutiveDownCandles = true;
    for (int j = 0; j < requiredCandles; j++) {
        if (sc.Close[i-j] >= sc.Open[i-j]) {
            consecutiveDownCandles = false;
            break;
        }
    }

    // Long signal: N consecutive up candles AND current delta volume positive
    const bool longCondition = consecutiveUpCandles && DeltaVolume[i] > 0;

    // Short signal: N consecutive down candles AND current delta volume negative
    const bool shortCondition = consecutiveDownCandles && DeltaVolume[i] < 0;

    if (shortCondition) {
        Signal[i] = -1;
        SellSignal[i] = sc.High[i] + 2 * sc.TickSize;
    }
    else if (longCondition) {
        Signal[i] = 1;
        BuySignal[i] = sc.Low[i] - 2 * sc.TickSize;
    }
    // else: signals already set to 0 above
}
