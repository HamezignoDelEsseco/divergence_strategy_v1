#include "helpers.h"
#include "sierrachart.h"
#include <cmath>

/*
 * HAWKES VOLATILITY ALEX
 *
 * Based on: https://github.com/neurotrader888/VolatilityHawkes
 *
 * Concept:
 * 1. Normalize range using ATR
 * 2. Apply Hawkes process (exponential smoothing with memory)
 * 3. Detect volatility expansion after contraction
 * 4. Signal direction based on price change during contraction
 *
 * Signal Logic:
 * - Wait for volatility < Q05 (contraction/accumulation)
 * - When volatility > Q95 (expansion/breakout)
 * - Direction = price change since contraction
 *   → Price up = LONG
 *   → Price down = SHORT
 */

SCSFExport scsf_HawkesVolatilityAlex(SCStudyInterfaceRef sc) {

    // Inputs
    SCInputRef ATRStudy = sc.Input[0];
    SCInputRef HawkesKappa = sc.Input[1];
    SCInputRef QuantileLookback = sc.Input[2];
    SCInputRef LowerQuantile = sc.Input[3];
    SCInputRef UpperQuantile = sc.Input[4];

    // Subgraphs
    SCSubgraphRef BuySignal = sc.Subgraph[0];
    SCSubgraphRef SellSignal = sc.Subgraph[1];
    SCSubgraphRef Signal = sc.Subgraph[2];  // Standard position for trading system
    SCSubgraphRef HawkesVol = sc.Subgraph[3];
    SCSubgraphRef Q05 = sc.Subgraph[4];
    SCSubgraphRef Q95 = sc.Subgraph[5];
    SCSubgraphRef NormalizedRange = sc.Subgraph[6];

    // Persistent variables
    int& LastBelowQ05Index = sc.GetPersistentInt(0);
    float& LastBelowQ05Price = sc.GetPersistentFloat(0);
    int& CurrentSignal = sc.GetPersistentInt(1);

    if (sc.SetDefaults) {
        sc.GraphName = "Hawkes Volatility Alex";
        sc.GraphRegion = 1;
        sc.AutoLoop = 1;

        // Inputs
        ATRStudy.Name = "ATR Study";
        ATRStudy.SetStudyID(0);
        ATRStudy.SetDescription("Reference to ATR study (add 'Average True Range' study first)");

        HawkesKappa.Name = "Hawkes Kappa (Decay)";
        HawkesKappa.SetFloat(0.1);
        HawkesKappa.SetFloatLimits(0.01, 1.0);
        HawkesKappa.SetDescription("Decay factor: lower = more memory (Python: 0.1)");

        QuantileLookback.Name = "Quantile Lookback";
        QuantileLookback.SetInt(168);
        QuantileLookback.SetIntLimits(20, 1000);
        QuantileLookback.SetDescription("Lookback for Q05/Q95 calculation (Python: 168)");

        LowerQuantile.Name = "Lower Quantile (Contraction)";
        LowerQuantile.SetFloat(0.05);
        LowerQuantile.SetFloatLimits(0.01, 0.25);
        LowerQuantile.SetDescription("Lower threshold (Python: 0.05 = 5th percentile)");

        UpperQuantile.Name = "Upper Quantile (Expansion)";
        UpperQuantile.SetFloat(0.95);
        UpperQuantile.SetFloatLimits(0.75, 0.99);
        UpperQuantile.SetDescription("Upper threshold (Python: 0.95 = 95th percentile)");

        // Subgraphs
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
        Signal.DrawZeros = true;

        HawkesVol.Name = "Hawkes Volatility";
        HawkesVol.DrawStyle = DRAWSTYLE_LINE;
        HawkesVol.PrimaryColor = RGB(255, 255, 0);
        HawkesVol.LineWidth = 2;

        Q05.Name = "Q05 (Lower Quantile)";
        Q05.DrawStyle = DRAWSTYLE_LINE;
        Q05.PrimaryColor = RGB(0, 255, 0);
        Q05.LineWidth = 1;

        Q95.Name = "Q95 (Upper Quantile)";
        Q95.DrawStyle = DRAWSTYLE_LINE;
        Q95.PrimaryColor = RGB(255, 0, 0);
        Q95.LineWidth = 1;

        NormalizedRange.Name = "Normalized Range";
        NormalizedRange.DrawStyle = DRAWSTYLE_IGNORE;
        NormalizedRange.PrimaryColor = RGB(100, 100, 100);

        return;
    }

    const int i = sc.Index;

    // Initialize
    Signal[i] = 0;
    BuySignal[i] = 0;
    SellSignal[i] = 0;

    // ========================================
    // STEP 1: Get ATR from study
    // ========================================
    SCFloatArray ATRArray;
    sc.GetStudyArrayUsingID(ATRStudy.GetStudyID(), 0, ATRArray);

    // Check if ATR study is valid
    if (ATRArray.GetArraySize() == 0) {
        return;
    }

    // ========================================
    // STEP 2: Normalized Range
    // ========================================
    // norm_range = (high - low) / ATR
    float range = sc.High[i] - sc.Low[i];
    if (ATRArray[i] > 0) {
        NormalizedRange[i] = range / ATRArray[i];
    } else {
        NormalizedRange[i] = 0;
    }

    // ========================================
    // STEP 3: Hawkes Process
    // ========================================
    // output[i] = output[i-1] * alpha + input[i]
    // then multiply by kappa
    float kappa = HawkesKappa.GetFloat();
    float alpha = exp(-kappa);

    if (i == 0 || sc.BaseDateTimeIn[i].GetDate() != sc.BaseDateTimeIn[i-1].GetDate()) {
        // First bar or new day: initialize
        HawkesVol[i] = NormalizedRange[i] * kappa;
    } else {
        HawkesVol[i] = (HawkesVol[i-1] * alpha + NormalizedRange[i]) * kappa;
    }

    // Need enough bars for quantile calculation
    if (i < QuantileLookback.GetInt()) {
        return;
    }

    // ========================================
    // STEP 4: Calculate Quantiles (Q05, Q95)
    // ========================================
    int lookback = QuantileLookback.GetInt();

    // Collect HawkesVol values for quantile calculation
    std::vector<float> volValues;
    volValues.reserve(lookback);
    for (int j = i - lookback + 1; j <= i; j++) {
        if (j >= 0) {
            volValues.push_back(HawkesVol[j]);
        }
    }

    // Sort for quantile calculation
    std::sort(volValues.begin(), volValues.end());

    // Calculate quantiles using user parameters
    float lowerQ = LowerQuantile.GetFloat();
    float upperQ = UpperQuantile.GetFloat();

    int q05Index = static_cast<int>(volValues.size() * lowerQ);
    int q95Index = static_cast<int>(volValues.size() * upperQ);

    // Ensure indices are within bounds
    if (q05Index < 0) q05Index = 0;
    if (q95Index >= static_cast<int>(volValues.size())) q95Index = volValues.size() - 1;

    Q05[i] = volValues[q05Index];
    Q95[i] = volValues[q95Index];

    // ========================================
    // STEP 5: Signal Generation
    // ========================================

    // Check if volatility is below Q05 (contraction)
    if (HawkesVol[i] < Q05[i]) {
        LastBelowQ05Index = i;
        LastBelowQ05Price = sc.Close[i];
        CurrentSignal = 0; // Reset signal during contraction
    }

    // Check if volatility breaks above Q95 (expansion)
    bool volBreaksQ95 = HawkesVol[i] > Q95[i];
    bool volWasBelowQ95 = (i > 0) && (HawkesVol[i-1] <= Q95[i-1]);
    bool hadContraction = LastBelowQ05Index >= 0;

    if (volBreaksQ95 && volWasBelowQ95 && hadContraction) {
        // Volatility expansion detected!
        // Determine direction based on price change since contraction
        float priceChange = sc.Close[i] - LastBelowQ05Price;

        if (priceChange > 0) {
            // Price moved up during contraction → LONG
            CurrentSignal = 1;
            BuySignal[i] = sc.Low[i] - 2 * sc.TickSize;

            SCString msg;
            msg.Format("LONG: Vol expansion after contraction. Price change: %.2f", priceChange);
            sc.AddMessageToLog(msg, 0);
        }
        else if (priceChange < 0) {
            // Price moved down during contraction → SHORT
            CurrentSignal = -1;
            SellSignal[i] = sc.High[i] + 2 * sc.TickSize;

            SCString msg;
            msg.Format("SHORT: Vol expansion after contraction. Price change: %.2f", priceChange);
            sc.AddMessageToLog(msg, 0);
        }
    }

    // Maintain current signal
    Signal[i] = CurrentSignal;
}
