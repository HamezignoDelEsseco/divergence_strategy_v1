#include "sierrachart.h"

SCSFExport scsf_VALevelSignalAlex(SCStudyInterfaceRef sc) {
    /*
     * Value Area Level Signal
     *
     * Trades bounces and breakouts at VAH/VAL levels
     * Configurable max trades per type per day:
     *   - N bounce off VAH (short)
     *   - N bounce off VAL (long)
     *   - N breakout above VAH (long)
     *   - N breakout below VAL (short)
     *
     * Bounce: Price touches level, next candle closes back inside VA
     * Breakout: Price closes beyond level, next candle confirms
     */

    SCInputRef ValueAreaStudy = sc.Input[0];
    // Input[1] removed (was unused ConfirmationBars)
    SCInputRef LevelTolerance = sc.Input[2];
    SCInputRef MaxBounceVAH = sc.Input[3];
    SCInputRef MaxBounceVAL = sc.Input[4];
    SCInputRef MaxBreakoutVAH = sc.Input[5];
    SCInputRef MaxBreakoutVAL = sc.Input[6];
    SCInputRef StartTradingTime = sc.Input[7];
    SCInputRef EndTradingTime = sc.Input[8];
    SCInputRef MaxReentryVAH = sc.Input[9];
    SCInputRef MaxReentryVAL = sc.Input[10];
    SCInputRef ReentryLookbackBars = sc.Input[11];
    SCInputRef ReentryOutsideBars = sc.Input[12];
    SCInputRef ReentryConfirmBars = sc.Input[13];

    SCSubgraphRef Signal = sc.Subgraph[0];
    SCSubgraphRef BuySignal = sc.Subgraph[1];
    SCSubgraphRef SellSignal = sc.Subgraph[2];
    SCSubgraphRef TradeType = sc.Subgraph[3];  // 1=BounceVAL, 2=BounceVAH, 3=BreakoutVAH, 4=BreakoutVAL, 5=ReentryVAH, 6=ReentryVAL

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "VA Level Signal Alex";

        ValueAreaStudy.Name = "Value Area Study (VAH=SG1, VAL=SG2)";
        ValueAreaStudy.SetStudyID(1);

        LevelTolerance.Name = "Level Tolerance (ticks)";
        LevelTolerance.SetInt(2);
        LevelTolerance.SetIntLimits(0, 20);
        LevelTolerance.SetDescription("How close price needs to be to level to count as touch");

        MaxBounceVAH.Name = "Max Bounce VAH per day";
        MaxBounceVAH.SetInt(100);
        MaxBounceVAH.SetIntLimits(0, 100);
        MaxBounceVAH.SetDescription("Maximum short signals from VAH bounce per day (0 = disabled)");

        MaxBounceVAL.Name = "Max Bounce VAL per day";
        MaxBounceVAL.SetInt(100);
        MaxBounceVAL.SetIntLimits(0, 100);
        MaxBounceVAL.SetDescription("Maximum long signals from VAL bounce per day (0 = disabled)");

        MaxBreakoutVAH.Name = "Max Breakout VAH per day";
        MaxBreakoutVAH.SetInt(100);
        MaxBreakoutVAH.SetIntLimits(0, 100);
        MaxBreakoutVAH.SetDescription("Maximum long signals from VAH breakout per day (0 = disabled)");

        MaxBreakoutVAL.Name = "Max Breakout VAL per day";
        MaxBreakoutVAL.SetInt(100);
        MaxBreakoutVAL.SetIntLimits(0, 100);
        MaxBreakoutVAL.SetDescription("Maximum short signals from VAL breakout per day (0 = disabled)");

        StartTradingTime.Name = "Start Trading Time";
        StartTradingTime.SetTime(HMS_TIME(8, 0, 0));

        EndTradingTime.Name = "End Trading Time";
        EndTradingTime.SetTime(HMS_TIME(15, 45, 0));

        MaxReentryVAH.Name = "Max Reentry VAH per day";
        MaxReentryVAH.SetInt(100);
        MaxReentryVAH.SetIntLimits(0, 100);
        MaxReentryVAH.SetDescription("Maximum short signals from VAH reentry per day (0 = disabled)");

        MaxReentryVAL.Name = "Max Reentry VAL per day";
        MaxReentryVAL.SetInt(100);
        MaxReentryVAL.SetIntLimits(0, 100);
        MaxReentryVAL.SetDescription("Maximum long signals from VAL reentry per day (0 = disabled)");

        ReentryLookbackBars.Name = "Reentry Lookback Bars";
        ReentryLookbackBars.SetInt(15);
        ReentryLookbackBars.SetIntLimits(5, 50);
        ReentryLookbackBars.SetDescription("How far back to look for outside sequence");

        ReentryOutsideBars.Name = "Reentry Outside Bars Required";
        ReentryOutsideBars.SetInt(3);
        ReentryOutsideBars.SetIntLimits(2, 10);
        ReentryOutsideBars.SetDescription("Consecutive bars that must have been outside level");

        ReentryConfirmBars.Name = "Reentry Confirm Bars";
        ReentryConfirmBars.SetInt(2);
        ReentryConfirmBars.SetIntLimits(1, 5);
        ReentryConfirmBars.SetDescription("Consecutive bars inside level to confirm reentry");

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

        TradeType.Name = "Trade Type";
        TradeType.DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int i = sc.Index;

    // Skip the currently forming bar - only process closed bars
    if (i == sc.ArraySize - 1) {
        return;
    }

    // Check if we already processed this bar (avoid reprocessing on recalculation)
    int& lastProcessedBar = sc.GetPersistentInt(10);

    // Reset on full recalculation
    if (sc.IsFullRecalculation && i == 0) {
        lastProcessedBar = 0;
    }

    if (i < lastProcessedBar) {
        // Already processed this bar, don't reset signals
        return;
    }
    if (i == lastProcessedBar) {
        // Same bar, already processed - keep existing signal values
        return;
    }

    // New bar to process
    lastProcessedBar = i;

    // Initialize signals for this new bar
    Signal[i] = 0;
    BuySignal[i] = 0;
    SellSignal[i] = 0;
    TradeType[i] = 0;

    if (i < 3) {
        return;
    }

    // Get Value Area levels from study
    // Assuming: Subgraph 0 = POC, Subgraph 1 = VAH, Subgraph 2 = VAL
    SCFloatArray VAH;
    SCFloatArray VAL;
    sc.GetStudyArrayUsingID(ValueAreaStudy.GetStudyID(), 1, VAH);
    sc.GetStudyArrayUsingID(ValueAreaStudy.GetStudyID(), 2, VAL);

    if (VAH[i] == 0 || VAL[i] == 0) {
        return;
    }

    // Check if within trading hours
    SCDateTime currentTime = sc.BaseDateTimeIn[i].GetTime();
    const bool withinTradingHours = currentTime >= StartTradingTime.GetTime() && currentTime <= EndTradingTime.GetTime();

    const float tolerance = sc.TickSize * LevelTolerance.GetInt();

    // Persistent tracking for daily trade limits
    int& lastResetDate = sc.GetPersistentInt(1);
    int& bounceVALCount = sc.GetPersistentInt(2);   // Long bounce off VAL count
    int& bounceVAHCount = sc.GetPersistentInt(3);   // Short bounce off VAH count
    int& breakoutVAHCount = sc.GetPersistentInt(4); // Long breakout above VAH count
    int& breakoutVALCount = sc.GetPersistentInt(5); // Short breakout below VAL count
    int& reentryVAHCount = sc.GetPersistentInt(11); // Short reentry from above VAH count
    int& reentryVALCount = sc.GetPersistentInt(12); // Long reentry from below VAL count

    // State tracking for signal detection
    int& touchedVAH = sc.GetPersistentInt(6);       // Bar index when VAH was touched
    int& touchedVAL = sc.GetPersistentInt(7);       // Bar index when VAL was touched
    int& brokeAboveVAH = sc.GetPersistentInt(8);    // Bar index when broke above VAH
    int& brokeBelowVAL = sc.GetPersistentInt(9);    // Bar index when broke below VAL

    // Get current date
    int currentDate = sc.BaseDateTimeIn[i].GetDate();

    // Reset daily counters on new day
    if (currentDate != lastResetDate) {
        lastResetDate = currentDate;
        bounceVALCount = 0;
        bounceVAHCount = 0;
        breakoutVAHCount = 0;
        breakoutVALCount = 0;
        reentryVAHCount = 0;
        reentryVALCount = 0;
        touchedVAH = 0;
        touchedVAL = 0;
        brokeAboveVAH = 0;
        brokeBelowVAL = 0;
        lastProcessedBar = 0;
    }

    // Current bar info
    const float high = sc.High[i];
    const float low = sc.Low[i];
    const float close = sc.Close[i];

    // Only track touches and generate signals within trading hours
    if (!withinTradingHours) {
        return;
    }

    // === BREAKOUT BELOW VAL (Short Signal) ===
    // Bar i-5: Close >= VAL - tolerance (inside or at edge)
    // Bar i-4: Close >= VAL - tolerance (inside or at edge)
    // Bar i-3: Close >= VAL - tolerance (inside or at edge)
    // Bar i-2: Close >= VAL - tolerance (inside or at edge)
    // Bar i-1: Close <= VAL - tolerance (breakout)
    // Bar i (current): Close <= VAL - tolerance (confirmation) → Signal
    if (breakoutVALCount < MaxBreakoutVAL.GetInt() && i >= 6) {
        bool wasInside5 = sc.Close[i - 5] >= VAL[i - 5] - tolerance;
        bool wasInside4 = sc.Close[i - 4] >= VAL[i - 4] - tolerance;
        bool wasInside3 = sc.Close[i - 3] >= VAL[i - 3] - tolerance;
        bool wasInside2 = sc.Close[i - 2] >= VAL[i - 2] - tolerance;
        bool brokeOut = sc.Close[i - 1] <= VAL[i - 1] - tolerance;
        bool confirmed = close <= VAL[i] - tolerance;

        if (wasInside5 && wasInside4 && wasInside3 && wasInside2 && brokeOut && confirmed) {
            Signal[i] = -1;
            SellSignal[i] = high;
            TradeType[i] = 4;  // Breakout VAL
            breakoutVALCount++;
        }
    }

    // === BREAKOUT ABOVE VAH (Long Signal) ===
    // Bar i-5: Close <= VAH + tolerance (inside or at edge)
    // Bar i-4: Close <= VAH + tolerance (inside or at edge)
    // Bar i-3: Close <= VAH + tolerance (inside or at edge)
    // Bar i-2: Close <= VAH + tolerance (inside or at edge)
    // Bar i-1: Close >= VAH + tolerance (breakout)
    // Bar i (current): Close >= VAH + tolerance (confirmation) → Signal
    if (breakoutVAHCount < MaxBreakoutVAH.GetInt() && i >= 6) {
        bool wasInside5 = sc.Close[i - 5] <= VAH[i - 5] + tolerance;
        bool wasInside4 = sc.Close[i - 4] <= VAH[i - 4] + tolerance;
        bool wasInside3 = sc.Close[i - 3] <= VAH[i - 3] + tolerance;
        bool wasInside2 = sc.Close[i - 2] <= VAH[i - 2] + tolerance;
        bool brokeOut = sc.Close[i - 1] >= VAH[i - 1] + tolerance;
        bool confirmed = close >= VAH[i] + tolerance;

        if (wasInside5 && wasInside4 && wasInside3 && wasInside2 && brokeOut && confirmed) {
            Signal[i] = 1;
            BuySignal[i] = low;
            TradeType[i] = 3;  // Breakout VAH
            breakoutVAHCount++;
        }
    }

    // === BOUNCE OFF VAH (Short Signal) ===
    // Bar i-1: High touched VAH
    // Bar i (current): Close < VAH (bounced back)
    if (bounceVAHCount < MaxBounceVAH.GetInt()) {
        bool touchedLevel = sc.High[i - 1] >= VAH[i - 1] - tolerance;
        bool bouncedBack = close < VAH[i] - tolerance;
        bool didntBreakout = sc.Close[i - 1] <= VAH[i - 1] + tolerance;  // Didn't close above

        if (touchedLevel && bouncedBack && didntBreakout) {
            Signal[i] = -1;
            SellSignal[i] = high;
            TradeType[i] = 2;  // Bounce VAH
            bounceVAHCount++;
        }
    }

    // === BOUNCE OFF VAL (Long Signal) ===
    // Bar i-1: Low touched VAL
    // Bar i (current): Close > VAL (bounced back)
    if (bounceVALCount < MaxBounceVAL.GetInt()) {
        bool touchedLevel = sc.Low[i - 1] <= VAL[i - 1] + tolerance;
        bool bouncedBack = close > VAL[i] + tolerance;
        bool didntBreakout = sc.Close[i - 1] >= VAL[i - 1] - tolerance;  // Didn't close below

        if (touchedLevel && bouncedBack && didntBreakout) {
            Signal[i] = 1;
            BuySignal[i] = low;
            TradeType[i] = 1;  // Bounce VAL
            bounceVALCount++;
        }
    }

    // === REENTRY FROM ABOVE VAH (Short Signal) ===
    // Look back to find N consecutive bars outside (above VAH)
    // Then require M consecutive bars inside (below VAH) to confirm
    if (reentryVAHCount < MaxReentryVAH.GetInt()) {
        int lookback = ReentryLookbackBars.GetInt();
        int outsideRequired = ReentryOutsideBars.GetInt();
        int confirmRequired = ReentryConfirmBars.GetInt();

        if (i >= lookback + confirmRequired) {
            // Step 1: Check confirmation - last confirmRequired bars must be inside (at or below VAH)
            bool allConfirmed = true;
            for (int j = 0; j < confirmRequired; j++) {
                if (sc.Close[i - j] > VAH[i - j] + tolerance) {
                    allConfirmed = false;
                    break;
                }
            }

            if (allConfirmed) {
                // Step 2: Look back to find outsideRequired consecutive bars above VAH
                bool foundOutsideSequence = false;
                for (int start = confirmRequired; start <= lookback - outsideRequired; start++) {
                    bool allOutside = true;
                    for (int j = 0; j < outsideRequired; j++) {
                        if (sc.Close[i - start - j] < VAH[i - start - j] + tolerance) {
                            allOutside = false;
                            break;
                        }
                    }
                    if (allOutside) {
                        foundOutsideSequence = true;
                        break;
                    }
                }

                if (foundOutsideSequence) {
                    Signal[i] = -1;
                    SellSignal[i] = high;
                    TradeType[i] = 5;  // Reentry VAH
                    reentryVAHCount++;
                }
            }
        }
    }

    // === REENTRY FROM BELOW VAL (Long Signal) ===
    // Look back to find N consecutive bars outside (below VAL)
    // Then require M consecutive bars inside (above VAL) to confirm
    if (reentryVALCount < MaxReentryVAL.GetInt()) {
        int lookback = ReentryLookbackBars.GetInt();
        int outsideRequired = ReentryOutsideBars.GetInt();
        int confirmRequired = ReentryConfirmBars.GetInt();

        if (i >= lookback + confirmRequired) {
            // Step 1: Check confirmation - last confirmRequired bars must be inside (at or above VAL)
            bool allConfirmed = true;
            for (int j = 0; j < confirmRequired; j++) {
                if (sc.Close[i - j] < VAL[i - j] - tolerance) {
                    allConfirmed = false;
                    break;
                }
            }

            if (allConfirmed) {
                // Step 2: Look back to find outsideRequired consecutive bars below VAL
                bool foundOutsideSequence = false;
                for (int start = confirmRequired; start <= lookback - outsideRequired; start++) {
                    bool allOutside = true;
                    for (int j = 0; j < outsideRequired; j++) {
                        if (sc.Close[i - start - j] > VAL[i - start - j] - tolerance) {
                            allOutside = false;
                            break;
                        }
                    }
                    if (allOutside) {
                        foundOutsideSequence = true;
                        break;
                    }
                }

                if (foundOutsideSequence) {
                    Signal[i] = 1;
                    BuySignal[i] = low;
                    TradeType[i] = 6;  // Reentry VAL
                    reentryVALCount++;
                }
            }
        }
    }
}
