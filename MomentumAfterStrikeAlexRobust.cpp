#include "helpers.h"
#include "sierrachart.h"

/*
 * MOMENTUM AFTER STRIKE ALEX ROBUST
 *
 * Amélioration de MomentumAfterStrike avec:
 * 1. Breakout Confirmation - Prix casse high/low récent
 * 2. Price Momentum - Chaque bougie progresse (close > close précédent)
 * 3. Daily Range Filter - Arrête signaux quand range jour >= seuil
 *
 * Concept Daily Range Filter:
 * - Si range jour < 30 points → Prendre tous les signaux (marché calme)
 * - Si range jour >= 30 points → STOP signaux (extension en cours, ne pas chaser)
 *
 * Range jour = High du jour - Low du jour
 *
 * LONG:
 *   - N bougies haussières consécutives (Close > Open)
 *   - Chaque close > close précédent (progression)
 *   - Delta volume positive
 *   - Prix casse high des X dernières barres (breakout)
 *   - Range du jour < seuil
 *
 * SHORT: (inverse)
 */

SCSFExport scsf_MomentumAfterStrikeAlexRobust(SCStudyInterfaceRef sc) {

    // Inputs
    SCInputRef DeltaVolumeStudy = sc.Input[0];
    SCInputRef ConsecutiveCandles = sc.Input[1];
    SCInputRef BreakoutLookback = sc.Input[2];
    SCInputRef RequireBreakout = sc.Input[3];
    SCInputRef RequirePriceMomentum = sc.Input[4];
    SCInputRef DailyRangeThreshold = sc.Input[5];
    SCInputRef UseDailyRangeFilter = sc.Input[6];
    SCInputRef MinBarsBetweenSignals = sc.Input[7];

    // Subgraphs
    SCSubgraphRef BuySignal = sc.Subgraph[0];
    SCSubgraphRef SellSignal = sc.Subgraph[1];
    SCSubgraphRef Signal = sc.Subgraph[2];
    SCSubgraphRef BreakoutLevelHigh = sc.Subgraph[3];
    SCSubgraphRef BreakoutLevelLow = sc.Subgraph[4];
    SCSubgraphRef DailyRange = sc.Subgraph[5];

    // Persistent variables
    int& LastResetDate = sc.GetPersistentInt(0);
    int& LastLongSignalBar = sc.GetPersistentInt(1);
    int& LastShortSignalBar = sc.GetPersistentInt(2);
    float& DailyHigh = sc.GetPersistentFloat(0);
    float& DailyLow = sc.GetPersistentFloat(1);

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Momentum After Strike Alex Robust";
        sc.GraphRegion = 0;

        // Inputs
        DeltaVolumeStudy.Name = "Delta Volume Study";
        DeltaVolumeStudy.SetStudyID(1);

        ConsecutiveCandles.Name = "Consecutive Candles Required";
        ConsecutiveCandles.SetInt(3);
        ConsecutiveCandles.SetIntLimits(2, 10);
        ConsecutiveCandles.SetDescription("Number of consecutive candles in same direction");

        BreakoutLookback.Name = "Breakout Lookback Bars";
        BreakoutLookback.SetInt(10);
        BreakoutLookback.SetIntLimits(5, 50);
        BreakoutLookback.SetDescription("Lookback period for high/low breakout detection");

        RequireBreakout.Name = "Require Breakout Confirmation";
        RequireBreakout.SetYesNo(1);
        RequireBreakout.SetDescription("Yes = Only signal if price breaks recent high/low");

        RequirePriceMomentum.Name = "Require Price Momentum";
        RequirePriceMomentum.SetYesNo(1);
        RequirePriceMomentum.SetDescription("Yes = Each candle close must be higher/lower than previous");

        DailyRangeThreshold.Name = "Daily Range Threshold (points)";
        DailyRangeThreshold.SetFloat(30.0);
        DailyRangeThreshold.SetFloatLimits(5.0, 200.0);
        DailyRangeThreshold.SetDescription("Stop signals when daily range >= this (marché a déjà bougé)");

        UseDailyRangeFilter.Name = "Use Daily Range Filter";
        UseDailyRangeFilter.SetYesNo(1);
        UseDailyRangeFilter.SetDescription("Yes = Stop signals when range >= threshold");

        MinBarsBetweenSignals.Name = "Min Bars Between Same Direction Signals";
        MinBarsBetweenSignals.SetInt(10);
        MinBarsBetweenSignals.SetIntLimits(0, 50);
        MinBarsBetweenSignals.SetDescription("Minimum bars to wait before re-entering same direction (0 = disabled)");

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

        BreakoutLevelHigh.Name = "Breakout Level High";
        BreakoutLevelHigh.DrawStyle = DRAWSTYLE_DASH;
        BreakoutLevelHigh.PrimaryColor = RGB(0, 200, 0);
        BreakoutLevelHigh.LineWidth = 1;
        BreakoutLevelHigh.DrawZeros = false;

        BreakoutLevelLow.Name = "Breakout Level Low";
        BreakoutLevelLow.DrawStyle = DRAWSTYLE_DASH;
        BreakoutLevelLow.PrimaryColor = RGB(200, 0, 0);
        BreakoutLevelLow.LineWidth = 1;
        BreakoutLevelLow.DrawZeros = false;

        DailyRange.Name = "Daily Range";
        DailyRange.DrawStyle = DRAWSTYLE_IGNORE;
        DailyRange.PrimaryColor = RGB(255, 255, 255);

        return;
    }

    const int i = sc.Index;

    // Initialize persistent variables if not initialized yet
    if (LastLongSignalBar == 0 && LastShortSignalBar == 0 && LastResetDate == 0) {
        LastLongSignalBar = -10000;
        LastShortSignalBar = -10000;
        DailyHigh = 0;
        DailyLow = 0;
        LastResetDate = 0;
    }

    // Get delta volume from the specified study
    SCFloatArray DeltaVolume;
    sc.GetStudyArrayUsingID(DeltaVolumeStudy.GetStudyID(), 0, DeltaVolume);

    const int requiredCandles = ConsecutiveCandles.GetInt();
    const int lookback = BreakoutLookback.GetInt();

    // Need at least max(N, lookback) bars
    if (i < max(requiredCandles - 1, lookback)) {
        // Initialize signals for early bars
        Signal[i] = 0;
        BuySignal[i] = 0;
        SellSignal[i] = 0;
        return;
    }

    // Only calculate signals on the last tick of the bar (when bar closes)
    // This prevents the signal from being set and then immediately reset on same bar
    if (sc.GetBarHasClosedStatus() == BHCS_BAR_HAS_NOT_CLOSED) {
        return;
    }

    // Initialize all signals to 0 first (only reaches here when bar closes)
    Signal[i] = 0;
    BuySignal[i] = 0;
    SellSignal[i] = 0;

    // ========================================
    // DAILY RANGE TRACKING
    // ========================================
    SCDateTime currentDate = sc.BaseDateTimeIn[i].GetDate();
    int currentDateInt = currentDate.GetDate();

    // Reset daily high/low at start of new day
    if (currentDateInt != LastResetDate) {
        DailyHigh = sc.High[i];
        DailyLow = sc.Low[i];
        LastResetDate = currentDateInt;
        // Reset signal bars at new day
        LastLongSignalBar = -1000;
        LastShortSignalBar = -1000;
    }

    // Update daily high/low
    if (sc.High[i] > DailyHigh) {
        DailyHigh = sc.High[i];
    }
    if (sc.Low[i] < DailyLow) {
        DailyLow = sc.Low[i];
    }

    // Calculate daily range
    float currentDailyRange = DailyHigh - DailyLow;
    DailyRange[i] = currentDailyRange;

    // Check if daily range filter is active
    bool rangeFilterActive = false;
    if (UseDailyRangeFilter.GetYesNo() && currentDailyRange >= DailyRangeThreshold.GetFloat()) {
        rangeFilterActive = true;

        // Log once per bar when filter activates
        static int lastLoggedBar = -1;
        static int lastLoggedDate = -1;
        if (i != lastLoggedBar || currentDateInt != lastLoggedDate) {
            SCString msg;
            msg.Format("Daily Range Filter Active: %.2f points >= %.2f points. No new signals (extension en cours).",
                      currentDailyRange, DailyRangeThreshold.GetFloat());
            sc.AddMessageToLog(msg, 0);
            lastLoggedBar = i;
            lastLoggedDate = currentDateInt;
        }

        return; // Stop taking new signals
    }

    // ========================================
    // BREAKOUT LEVEL CALCULATION
    // ========================================
    float breakoutHigh = sc.High[i - lookback];
    float breakoutLow = sc.Low[i - lookback];

    for (int j = i - lookback + 1; j < i; j++) {
        if (sc.High[j] > breakoutHigh) {
            breakoutHigh = sc.High[j];
        }
        if (sc.Low[j] < breakoutLow) {
            breakoutLow = sc.Low[j];
        }
    }

    BreakoutLevelHigh[i] = breakoutHigh;
    BreakoutLevelLow[i] = breakoutLow;

    // ========================================
    // CONSECUTIVE CANDLES CHECK
    // ========================================
    bool consecutiveUpCandles = true;
    bool consecutiveDownCandles = true;

    for (int j = 0; j < requiredCandles; j++) {
        if (sc.Close[i-j] <= sc.Open[i-j]) {
            consecutiveUpCandles = false;
        }
        if (sc.Close[i-j] >= sc.Open[i-j]) {
            consecutiveDownCandles = false;
        }
    }

    // ========================================
    // PRICE MOMENTUM CHECK
    // ========================================
    bool priceMomentumUp = true;
    bool priceMomentumDown = true;

    if (RequirePriceMomentum.GetYesNo()) {
        for (int j = 0; j < requiredCandles - 1; j++) {
            // For uptrend: each close should be higher than previous
            if (sc.Close[i-j] <= sc.Close[i-j-1]) {
                priceMomentumUp = false;
            }
            // For downtrend: each close should be lower than previous
            if (sc.Close[i-j] >= sc.Close[i-j-1]) {
                priceMomentumDown = false;
            }
        }
    }

    // ========================================
    // BREAKOUT CONFIRMATION
    // ========================================
    bool breaksHigh = sc.Close[i] > breakoutHigh;
    bool breaksLow = sc.Close[i] < breakoutLow;

    // ========================================
    // SIGNAL SPACING CHECK
    // ========================================
    int minBarsBetween = MinBarsBetweenSignals.GetInt();
    int barsSinceLastLong = (LastLongSignalBar >= 0) ? (i - LastLongSignalBar) : 1000;
    int barsSinceLastShort = (LastShortSignalBar >= 0) ? (i - LastShortSignalBar) : 1000;

    bool canTakeLong = (minBarsBetween == 0) || (barsSinceLastLong >= minBarsBetween);
    bool canTakeShort = (minBarsBetween == 0) || (barsSinceLastShort >= minBarsBetween);

    // ========================================
    // SIGNAL LOGIC
    // ========================================

    // Check if signal conditions are present (without spacing check)
    bool longConditionsPresent =
        consecutiveUpCandles &&
        DeltaVolume[i] > 0 &&
        (!RequirePriceMomentum.GetYesNo() || priceMomentumUp) &&
        (!RequireBreakout.GetYesNo() || breaksHigh);

    bool shortConditionsPresent =
        consecutiveDownCandles &&
        DeltaVolume[i] < 0 &&
        (!RequirePriceMomentum.GetYesNo() || priceMomentumDown) &&
        (!RequireBreakout.GetYesNo() || breaksLow);

    // Process SHORT signal
    if (shortConditionsPresent) {
        if (canTakeShort) {
            // Accept signal - conditions met AND properly spaced
            Signal[i] = -1;
            SellSignal[i] = sc.High[i] + 2 * sc.TickSize;

            SCString msg;
            msg.Format("SHORT TAKEN: %d bars, momentum=%s, breakout=%s, range=%.2f pts, last_short=%d bars ago",
                      requiredCandles,
                      priceMomentumDown ? "Yes" : "No",
                      breaksLow ? "Yes" : "No",
                      currentDailyRange,
                      barsSinceLastShort);
            sc.AddMessageToLog(msg, 0);

            // Update last signal bar ONLY when taking signal
            LastShortSignalBar = i;
        } else {
            // Block signal - too soon after last one
            // DO NOT update LastShortSignalBar - let the counter keep running from last TAKEN signal
            SCString msg;
            msg.Format("SHORT BLOCKED: Only %d bars since last signal (need %d)",
                      barsSinceLastShort, minBarsBetween);
            sc.AddMessageToLog(msg, 0);
        }
    }
    // Process LONG signal
    else if (longConditionsPresent) {
        if (canTakeLong) {
            // Accept signal - conditions met AND properly spaced
            Signal[i] = 1;
            BuySignal[i] = sc.Low[i] - 2 * sc.TickSize;

            SCString msg;
            msg.Format("LONG TAKEN: %d bars, momentum=%s, breakout=%s, range=%.2f pts, last_long=%d bars ago",
                      requiredCandles,
                      priceMomentumUp ? "Yes" : "No",
                      breaksHigh ? "Yes" : "No",
                      currentDailyRange,
                      barsSinceLastLong);
            sc.AddMessageToLog(msg, 0);

            // Update last signal bar ONLY when taking signal
            LastLongSignalBar = i;
        } else {
            // Block signal - too soon after last one
            // DO NOT update LastLongSignalBar - let the counter keep running from last TAKEN signal
            SCString msg;
            msg.Format("LONG BLOCKED: Only %d bars since last signal (need %d)",
                      barsSinceLastLong, minBarsBetween);
            sc.AddMessageToLog(msg, 0);
        }
    }
}
