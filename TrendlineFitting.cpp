/*
 * Trendline Fitting Study
 *
 * This study implements trendline fitting using optimization-based approach
 * to find support and resistance lines that fit high/low price data.
 *
 * Algorithm based on least-squares optimization with constraint checking
 * to ensure trendlines don't cross price data inappropriately.
 */

#include "sierrachart.h"
#include <cmath>

// Forward declarations
void linearRegression(const SCFloatArray& prices, int startIdx, int endIdx, double& slope, double& intercept);
double checkTrendLine(bool support, int pivot, double slope, const SCFloatArray& prices, int startIdx, int endIdx);
bool optimizeSlope(bool support, int pivot, double initSlope, const SCFloatArray& prices, int startIdx, int endIdx, double& outSlope, double& outIntercept);
void fitTrendlinesHighLow(const SCFloatArray& high, const SCFloatArray& low, const SCFloatArray& close,
                          int startIdx, int endIdx,
                          double& supportSlope, double& supportIntercept,
                          double& resistSlope, double& resistIntercept);

/**
 * Linear regression using least squares method
 * Calculates slope and intercept for a line of best fit
 */
void linearRegression(const SCFloatArray& prices, int startIdx, int endIdx, double& slope, double& intercept) {
    const int n = endIdx - startIdx + 1;

    if (n <= 0) {
        slope = 0;
        intercept = 0;
        return;
    }

    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;

    for (int i = 0; i < n; i++) {
        const double x = static_cast<double>(i);
        const double y = prices[startIdx + i];

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    const double denom = n * sumX2 - sumX * sumX;
    if (std::abs(denom) > 1e-10) {
        slope = (n * sumXY - sumX * sumY) / denom;
        intercept = (sumY - slope * sumX) / n;
    } else {
        slope = 0;
        intercept = sumY / n;
    }
}

/**
 * Check if a trendline is valid
 * Returns sum of squared errors if valid, -1.0 if invalid
 *
 * For support lines: line must not go above any prices
 * For resistance lines: line must not go below any prices
 */
double checkTrendLine(bool support, int pivot, double slope, const SCFloatArray& prices, int startIdx, int endIdx) {
    const int n = endIdx - startIdx + 1;
    const int pivotOffset = pivot - startIdx;

    // Find intercept of line through pivot point
    const double intercept = -slope * pivotOffset + prices[pivot];

    // Check validity and compute error
    double sumSquaredDiffs = 0.0;

    for (int i = 0; i < n; i++) {
        const double lineVal = slope * i + intercept;
        const double diff = lineVal - prices[startIdx + i];

        // Check validity
        if (support && diff > 1e-5) {
            return -1.0; // Support line above price - invalid
        }
        if (!support && diff < -1e-5) {
            return -1.0; // Resistance line below price - invalid
        }

        sumSquaredDiffs += diff * diff;
    }

    return sumSquaredDiffs;
}

/**
 * Optimize slope to minimize error while maintaining validity
 * Uses iterative gradient descent with adaptive step size
 */
bool optimizeSlope(bool support, int pivot, double initSlope, const SCFloatArray& prices,
                   int startIdx, int endIdx, double& outSlope, double& outIntercept) {
    const int n = endIdx - startIdx + 1;
    const int pivotOffset = pivot - startIdx;

    // Find price range to scale slope adjustments
    double minPrice = prices[startIdx];
    double maxPrice = prices[startIdx];
    for (int i = startIdx + 1; i <= endIdx; i++) {
        if (prices[i] < minPrice) minPrice = prices[i];
        if (prices[i] > maxPrice) maxPrice = prices[i];
    }

    const double slopeUnit = (maxPrice - minPrice) / n;

    // Optimization parameters
    const double optStep = 1.0;
    const double minStep = 0.0001;
    double currStep = optStep;

    // Initialize
    double bestSlope = initSlope;
    double bestErr = checkTrendLine(support, pivot, initSlope, prices, startIdx, endIdx);

    if (bestErr < 0.0) {
        return false; // Initial slope invalid
    }

    bool getDerivative = true;
    double derivative = 0.0;

    while (currStep > minStep) {
        if (getDerivative) {
            // Numerical differentiation - determine direction
            double slopeChange = bestSlope + slopeUnit * minStep;
            double testErr = checkTrendLine(support, pivot, slopeChange, prices, startIdx, endIdx);

            if (testErr >= 0.0) {
                derivative = testErr - bestErr;
            } else {
                // Try decreasing instead
                slopeChange = bestSlope - slopeUnit * minStep;
                testErr = checkTrendLine(support, pivot, slopeChange, prices, startIdx, endIdx);

                if (testErr < 0.0) {
                    // Cannot find derivative - stop optimization
                    break;
                }
                derivative = bestErr - testErr;
            }

            getDerivative = false;
        }

        // Adjust slope based on derivative
        double testSlope;
        if (derivative > 0.0) {
            testSlope = bestSlope - slopeUnit * currStep;
        } else {
            testSlope = bestSlope + slopeUnit * currStep;
        }

        const double testErr = checkTrendLine(support, pivot, testSlope, prices, startIdx, endIdx);

        if (testErr < 0.0 || testErr >= bestErr) {
            // Failed or didn't improve - reduce step size
            currStep *= 0.5;
        } else {
            // Improved - update best slope
            bestErr = testErr;
            bestSlope = testSlope;
            getDerivative = true; // Recompute derivative
        }
    }

    // Calculate final intercept
    outSlope = bestSlope;
    outIntercept = -bestSlope * pivotOffset + prices[pivot];

    return true;
}

/**
 * Fit support and resistance trendlines using high/low/close data
 * This is the main algorithm matching the Python implementation
 */
void fitTrendlinesHighLow(const SCFloatArray& high, const SCFloatArray& low, const SCFloatArray& close,
                          int startIdx, int endIdx,
                          double& supportSlope, double& supportIntercept,
                          double& resistSlope, double& resistIntercept) {

    // Step 1: Find line of best fit using close prices
    double initSlope, initIntercept;
    linearRegression(close, startIdx, endIdx, initSlope, initIntercept);

    const int n = endIdx - startIdx + 1;

    // Step 2: Find upper and lower pivot points
    int upperPivot = startIdx;
    int lowerPivot = startIdx;
    double maxDeviation = -1e10;
    double minDeviation = 1e10;

    for (int i = 0; i < n; i++) {
        const double lineVal = initSlope * i + initIntercept;

        // Check high prices for resistance pivot
        const double highDeviation = high[startIdx + i] - lineVal;
        if (highDeviation > maxDeviation) {
            maxDeviation = highDeviation;
            upperPivot = startIdx + i;
        }

        // Check low prices for support pivot
        const double lowDeviation = low[startIdx + i] - lineVal;
        if (lowDeviation < minDeviation) {
            minDeviation = lowDeviation;
            lowerPivot = startIdx + i;
        }
    }

    // Step 3: Optimize support line using lows
    if (!optimizeSlope(true, lowerPivot, initSlope, low, startIdx, endIdx, supportSlope, supportIntercept)) {
        // Fallback if optimization fails
        supportSlope = initSlope;
        supportIntercept = initIntercept;
    }

    // Step 4: Optimize resistance line using highs
    if (!optimizeSlope(false, upperPivot, initSlope, high, startIdx, endIdx, resistSlope, resistIntercept)) {
        // Fallback if optimization fails
        resistSlope = initSlope;
        resistIntercept = initIntercept;
    }
}

/**
 * Sierra Chart Study: Trendline Slopes
 *
 * Calculates support and resistance slopes using a rolling lookback window
 * Outputs the slopes over time for analysis
 */
SCSFExport scsf_TrendlineSlopes(SCStudyInterfaceRef sc) {
    // Inputs
    SCInputRef LookbackPeriod = sc.Input[0];
    SCInputRef UseLogPrices = sc.Input[1];

    // Outputs
    SCSubgraphRef SupportSlope = sc.Subgraph[0];
    SCSubgraphRef ResistSlope = sc.Subgraph[1];
    SCSubgraphRef SupportLine = sc.Subgraph[2];
    SCSubgraphRef ResistLine = sc.Subgraph[3];

    // Temporary arrays for log transformation (hidden)
    SCSubgraphRef TempHigh = sc.Subgraph[4];
    SCSubgraphRef TempLow = sc.Subgraph[5];
    SCSubgraphRef TempClose = sc.Subgraph[6];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trendline Slopes";
        sc.GraphRegion = 1; // Separate region

        LookbackPeriod.Name = "Lookback Period";
        LookbackPeriod.SetIntLimits(5, 200);
        LookbackPeriod.SetInt(30);

        UseLogPrices.Name = "Use Log Prices";
        UseLogPrices.SetYesNo(0);

        SupportSlope.Name = "Support Slope";
        SupportSlope.DrawStyle = DRAWSTYLE_LINE;
        SupportSlope.PrimaryColor = RGB(0, 255, 0); // Green
        SupportSlope.LineWidth = 2;

        ResistSlope.Name = "Resistance Slope";
        ResistSlope.DrawStyle = DRAWSTYLE_LINE;
        ResistSlope.PrimaryColor = RGB(255, 0, 0); // Red
        ResistSlope.LineWidth = 2;

        SupportLine.Name = "Support Trendline";
        SupportLine.DrawStyle = DRAWSTYLE_IGNORE;

        ResistLine.Name = "Resistance Trendline";
        ResistLine.DrawStyle = DRAWSTYLE_IGNORE;

        // Hide temporary arrays
        TempHigh.Name = "Temp High (Log)";
        TempHigh.DrawStyle = DRAWSTYLE_IGNORE;

        TempLow.Name = "Temp Low (Log)";
        TempLow.DrawStyle = DRAWSTYLE_IGNORE;

        TempClose.Name = "Temp Close (Log)";
        TempClose.DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int i = sc.Index;
    const int lookback = LookbackPeriod.GetInt();

    // Need enough bars for calculation
    if (i < lookback - 1) {
        return;
    }

    const int startIdx = i - lookback + 1;
    const int endIdx = i;

    // Calculate trendlines
    double supportSlope, supportIntercept;
    double resistSlope, resistIntercept;

    // Apply log transformation if requested
    if (UseLogPrices.GetYesNo()) {
        // Use subgraph arrays for temporary log-transformed storage
        for (int j = startIdx; j <= endIdx; j++) {
            TempHigh[j] = static_cast<float>(std::log(sc.High[j]));
            TempLow[j] = static_cast<float>(std::log(sc.Low[j]));
            TempClose[j] = static_cast<float>(std::log(sc.Close[j]));
        }

        fitTrendlinesHighLow(TempHigh, TempLow, TempClose, startIdx, endIdx,
                            supportSlope, supportIntercept,
                            resistSlope, resistIntercept);
    } else {
        // Use original prices directly
        fitTrendlinesHighLow(sc.High, sc.Low, sc.Close, startIdx, endIdx,
                            supportSlope, supportIntercept,
                            resistSlope, resistIntercept);
    }

    // Output slopes
    SupportSlope[i] = static_cast<float>(supportSlope);
    ResistSlope[i] = static_cast<float>(resistSlope);

    // Calculate current trendline values
    const int offset = lookback - 1;
    SupportLine[i] = static_cast<float>(supportSlope * offset + supportIntercept);
    ResistLine[i] = static_cast<float>(resistSlope * offset + resistIntercept);

    // If using log prices, convert back
    if (UseLogPrices.GetYesNo()) {
        SupportLine[i] = std::exp(SupportLine[i]);
        ResistLine[i] = std::exp(ResistLine[i]);
    }
}

/**
 * Sierra Chart Study: Trendline Display
 *
 * Displays actual support and resistance trendlines on the price chart
 */
SCSFExport scsf_TrendlineDisplay(SCStudyInterfaceRef sc) {
    // Inputs
    SCInputRef LookbackPeriod = sc.Input[0];
    SCInputRef UseLogPrices = sc.Input[1];

    // Outputs
    SCSubgraphRef SupportLine = sc.Subgraph[0];
    SCSubgraphRef ResistLine = sc.Subgraph[1];

    // Temporary arrays for log transformation (hidden)
    SCSubgraphRef TempHigh = sc.Subgraph[2];
    SCSubgraphRef TempLow = sc.Subgraph[3];
    SCSubgraphRef TempClose = sc.Subgraph[4];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trendline Display";
        sc.GraphRegion = 0; // Main price region

        LookbackPeriod.Name = "Lookback Period";
        LookbackPeriod.SetIntLimits(5, 200);
        LookbackPeriod.SetInt(30);

        UseLogPrices.Name = "Use Log Prices";
        UseLogPrices.SetYesNo(0);

        SupportLine.Name = "Support Line";
        SupportLine.DrawStyle = DRAWSTYLE_LINE;
        SupportLine.PrimaryColor = RGB(0, 200, 0); // Green
        SupportLine.LineWidth = 2;

        ResistLine.Name = "Resistance Line";
        ResistLine.DrawStyle = DRAWSTYLE_LINE;
        ResistLine.PrimaryColor = RGB(200, 0, 0); // Red
        ResistLine.LineWidth = 2;

        // Hide temporary arrays
        TempHigh.Name = "Temp High (Log)";
        TempHigh.DrawStyle = DRAWSTYLE_IGNORE;

        TempLow.Name = "Temp Low (Log)";
        TempLow.DrawStyle = DRAWSTYLE_IGNORE;

        TempClose.Name = "Temp Close (Log)";
        TempClose.DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int i = sc.Index;
    const int lookback = LookbackPeriod.GetInt();

    // Need enough bars for calculation
    if (i < lookback - 1) {
        return;
    }

    const int startIdx = i - lookback + 1;
    const int endIdx = i;

    // Calculate trendlines
    double supportSlope, supportIntercept;
    double resistSlope, resistIntercept;

    // Apply log transformation if requested
    if (UseLogPrices.GetYesNo()) {
        // Use subgraph arrays for temporary log-transformed storage
        for (int j = startIdx; j <= endIdx; j++) {
            TempHigh[j] = static_cast<float>(std::log(sc.High[j]));
            TempLow[j] = static_cast<float>(std::log(sc.Low[j]));
            TempClose[j] = static_cast<float>(std::log(sc.Close[j]));
        }

        fitTrendlinesHighLow(TempHigh, TempLow, TempClose, startIdx, endIdx,
                            supportSlope, supportIntercept,
                            resistSlope, resistIntercept);
    } else {
        // Use original prices directly
        fitTrendlinesHighLow(sc.High, sc.Low, sc.Close, startIdx, endIdx,
                            supportSlope, supportIntercept,
                            resistSlope, resistIntercept);
    }

    // Draw trendlines for the entire lookback period
    for (int j = 0; j < lookback; j++) {
        const int barIdx = startIdx + j;

        double supportVal = supportSlope * j + supportIntercept;
        double resistVal = resistSlope * j + resistIntercept;

        // Convert back from log if necessary
        if (UseLogPrices.GetYesNo()) {
            supportVal = std::exp(supportVal);
            resistVal = std::exp(resistVal);
        }

        SupportLine[barIdx] = static_cast<float>(supportVal);
        ResistLine[barIdx] = static_cast<float>(resistVal);
    }
}

/**
 * Sierra Chart Study: Trendline Breakout Signal
 *
 * Generates trading signals based on price breaking through trendlines
 *
 * Logic:
 * 1. Fits trendlines on PAST N bars (excluding current bar)
 * 2. Projects trendlines forward to current bar
 * 3. Generates signals:
 *    +1 = BUY  (Price breaks above resistance)
 *    -1 = SELL (Price breaks below support)
 *     0 = HOLD (Price between lines, maintain previous signal)
 */
SCSFExport scsf_TrendlineBreakoutSignal(SCStudyInterfaceRef sc) {
    // Inputs
    SCInputRef LookbackPeriod = sc.Input[0];
    SCInputRef UseLogPrices = sc.Input[1];

    // Outputs
    SCSubgraphRef TradeSignal = sc.Subgraph[0];
    SCSubgraphRef SupportValue = sc.Subgraph[1];
    SCSubgraphRef ResistValue = sc.Subgraph[2];

    // Temporary arrays for log transformation (hidden)
    SCSubgraphRef TempHigh = sc.Subgraph[3];
    SCSubgraphRef TempLow = sc.Subgraph[4];
    SCSubgraphRef TempClose = sc.Subgraph[5];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trendline Breakout Signal";
        sc.GraphRegion = 1; // Separate region

        LookbackPeriod.Name = "Lookback Period";
        LookbackPeriod.SetIntLimits(5, 200);
        LookbackPeriod.SetInt(72);

        UseLogPrices.Name = "Use Log Prices";
        UseLogPrices.SetYesNo(0);

        TradeSignal.Name = "Trade Signal";
        TradeSignal.DrawStyle = DRAWSTYLE_LINE;
        TradeSignal.PrimaryColor = RGB(255, 255, 0); // Yellow
        TradeSignal.LineWidth = 2;
        TradeSignal.DrawZeros = true;

        SupportValue.Name = "Support Value (Projected)";
        SupportValue.DrawStyle = DRAWSTYLE_IGNORE;

        ResistValue.Name = "Resistance Value (Projected)";
        ResistValue.DrawStyle = DRAWSTYLE_IGNORE;

        // Hide temporary arrays
        TempHigh.Name = "Temp High (Log)";
        TempHigh.DrawStyle = DRAWSTYLE_IGNORE;

        TempLow.Name = "Temp Low (Log)";
        TempLow.DrawStyle = DRAWSTYLE_IGNORE;

        TempClose.Name = "Temp Close (Log)";
        TempClose.DrawStyle = DRAWSTYLE_IGNORE;

        return;
    }

    const int i = sc.Index;
    const int lookback = LookbackPeriod.GetInt();

    // Need enough bars for calculation
    if (i < lookback) {
        return;
    }

    // IMPORTANT: Fit trendlines on PAST data only (NOT including current bar)
    // Window: [i - lookback, i - 1]
    const int startIdx = i - lookback;
    const int endIdx = i - 1;

    // Calculate trendlines on PAST data
    double supportSlope, supportIntercept;
    double resistSlope, resistIntercept;

    // Apply log transformation if requested
    if (UseLogPrices.GetYesNo()) {
        // Use subgraph arrays for temporary log-transformed storage
        for (int j = startIdx; j <= endIdx; j++) {
            TempHigh[j] = static_cast<float>(std::log(sc.High[j]));
            TempLow[j] = static_cast<float>(std::log(sc.Low[j]));
            TempClose[j] = static_cast<float>(std::log(sc.Close[j]));
        }

        fitTrendlinesHighLow(TempHigh, TempLow, TempClose, startIdx, endIdx,
                            supportSlope, supportIntercept,
                            resistSlope, resistIntercept);
    } else {
        // Use original prices directly
        fitTrendlinesHighLow(sc.High, sc.Low, sc.Close, startIdx, endIdx,
                            supportSlope, supportIntercept,
                            resistSlope, resistIntercept);
    }

    // PROJECT trendlines FORWARD to current bar
    // In the fitted window, current bar would be at position 'lookback'
    // (positions 0 to lookback-1 were used for fitting)
    double supportVal = supportSlope * lookback + supportIntercept;
    double resistVal = resistSlope * lookback + resistIntercept;

    // Convert back from log if necessary
    if (UseLogPrices.GetYesNo()) {
        supportVal = std::exp(supportVal);
        resistVal = std::exp(resistVal);
    }

    // Store projected values for reference
    SupportValue[i] = static_cast<float>(supportVal);
    ResistValue[i] = static_cast<float>(resistVal);

    // Get current close price
    const float currentClose = sc.Close[i];

    // GENERATE SIGNAL based on price vs trendlines
    if (currentClose > resistVal) {
        // Price broke above resistance = BREAKOUT = BUY
        TradeSignal[i] = 1.0;
    }
    else if (currentClose < supportVal) {
        // Price broke below support = BREAKDOWN = SELL
        TradeSignal[i] = -1.0;
    }
    else {
        // Price between lines = HOLD previous signal
        if (i > 0) {
            TradeSignal[i] = TradeSignal[i - 1];
        } else {
            TradeSignal[i] = 0.0;
        }
    }
}
