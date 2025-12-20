#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_MomentumAfterStrikeAlexRobustTrading(SCStudyInterfaceRef sc) {

    SCInputRef SignalStudy = sc.Input[0];
    SCInputRef StopLoss = sc.Input[1];
    SCInputRef TakeProfit = sc.Input[2];
    SCInputRef StartTradingTime = sc.Input[3];
    SCInputRef EndTradingTime = sc.Input[4];
    SCInputRef MaxDailyPLInTicks = sc.Input[5];
    SCInputRef MinDailyPLInTicks = sc.Input[6];
    SCInputRef FlattenOnZeroSignal = sc.Input[7];
    SCInputRef ConsecutiveOppositeBars = sc.Input[8];

    // Persistent variable to track consecutive bars
    int& ConsecutiveRedBars = sc.GetPersistentInt(1);
    int& ConsecutiveGreenBars = sc.GetPersistentInt(2);

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Momentum After Strike Alex Robust Trading";

        // Inputs
        SignalStudy.Name = "Signal Study (MomentumAfterStrikeAlexRobust)";
        SignalStudy.SetStudyID(1);

        StopLoss.Name = "Stop Loss (ticks)";
        StopLoss.SetIntLimits(1, 500);
        StopLoss.SetInt(100);

        TakeProfit.Name = "Take Profit (ticks)";
        TakeProfit.SetIntLimits(1, 500);
        TakeProfit.SetInt(100);

        StartTradingTime.Name = "Start trading time";
        StartTradingTime.SetTime(HMS_TIME(8, 0, 0));

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15, 45, 0));

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day";
        MaxDailyPLInTicks.SetIntLimits(10, 1000);
        MaxDailyPLInTicks.SetInt(300);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day";
        MinDailyPLInTicks.SetIntLimits(-1000, -10);
        MinDailyPLInTicks.SetInt(-100);

        FlattenOnZeroSignal.Name = "Flatten Position When Signal = 0";
        FlattenOnZeroSignal.SetYesNo(0);
        FlattenOnZeroSignal.SetDescription("Yes = Exit when signal goes to 0, No = Exit only on SL/TP or opposite bars");

        ConsecutiveOppositeBars.Name = "Consecutive Opposite Bars to Flatten";
        ConsecutiveOppositeBars.SetInt(3);
        ConsecutiveOppositeBars.SetIntLimits(0, 10);
        ConsecutiveOppositeBars.SetDescription("Number of consecutive opposite candles before flattening (0 = disabled)");

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 1;
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = false;
        sc.AllowEntryWithWorkingOrders = true;

        return;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize - 1)
        return;

    const int i = sc.Index;
    int& messagePrinted = sc.GetPersistentInt(0);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    // Check if within trading hours
    SCDateTime currentTime = sc.BaseDateTimeIn[i].GetTime();

    // Don't trade before start time
    if (currentTime < StartTradingTime.GetTime()) {
        return;
    }

    // Stop trading after end time and flatten any open positions
    if (currentTime > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    // Check daily P/L limits
    if (
        (PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
            || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0
        ) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("You have exceeded your daily loss / profit", 1);
        }
        messagePrinted++;
        return;
    }

    // ========================================
    // CONSECUTIVE OPPOSITE BARS TRACKING
    // ========================================
    int oppositeBarThreshold = ConsecutiveOppositeBars.GetInt();

    if (oppositeBarThreshold > 0 && i > 0) {
        // Check if current bar is red (bearish)
        bool currentBarRed = sc.Close[i] < sc.Open[i];
        // Check if current bar is green (bullish)
        bool currentBarGreen = sc.Close[i] > sc.Open[i];

        // ONLY track consecutive bars if we're in a position
        if (PositionData.PositionQuantity > 0) {
            // In LONG position - track red bars only
            if (currentBarRed) {
                ConsecutiveRedBars++;
            } else {
                ConsecutiveRedBars = 0;  // Reset if not red
            }

            // Check if we should flatten
            if (ConsecutiveRedBars >= oppositeBarThreshold) {
                sc.FlattenAndCancelAllOrders();
                SCString msg;
                msg.Format("LONG FLATTENED: %d consecutive red bars detected", ConsecutiveRedBars);
                sc.AddMessageToLog(msg, 0);
                ConsecutiveRedBars = 0;
                ConsecutiveGreenBars = 0;
                return;
            }
        }
        else if (PositionData.PositionQuantity < 0) {
            // In SHORT position - track green bars only
            if (currentBarGreen) {
                ConsecutiveGreenBars++;
            } else {
                ConsecutiveGreenBars = 0;  // Reset if not green
            }

            // Check if we should flatten
            if (ConsecutiveGreenBars >= oppositeBarThreshold) {
                sc.FlattenAndCancelAllOrders();
                SCString msg;
                msg.Format("SHORT FLATTENED: %d consecutive green bars detected", ConsecutiveGreenBars);
                sc.AddMessageToLog(msg, 0);
                ConsecutiveRedBars = 0;
                ConsecutiveGreenBars = 0;
                return;
            }
        }
        else {
            // No position - reset both counters
            ConsecutiveRedBars = 0;
            ConsecutiveGreenBars = 0;
        }
    }

    // ========================================
    // GET SIGNAL FROM ROBUST STUDY
    // ========================================
    // Get the signal from MomentumAfterStrikeAlexRobust study (Subgraph[2])
    SCFloatArray Signal;
    sc.GetStudyArrayUsingID(SignalStudy.GetStudyID(), 2, Signal);

    // Setup new order with SL and TP
    s_SCNewOrder newOrder;
    newOrder.OrderQuantity = 1;
    newOrder.TimeInForce = SCT_TIF_DAY;
    newOrder.OrderType = SCT_ORDERTYPE_MARKET;
    newOrder.Stop1Offset = sc.TickSize * StopLoss.GetInt();
    newOrder.Target1Offset = sc.TickSize * TakeProfit.GetInt();
    newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_STOP_LIMIT;
    newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_LIMIT;

    // Signal = 0 (Neutral): Behavior based on FlattenOnZeroSignal parameter
    if (Signal[i-1] == 0) {
        if (FlattenOnZeroSignal.GetYesNo() && PositionData.PositionQuantity != 0) {
            // Option: Flatten all positions when signal goes to zero
            sc.FlattenAndCancelAllOrders();
        }
        // Otherwise: Keep position, exit only on SL/TP or opposite bars
        return;
    }

    // Signal = 1 (Long): Go long
    if (Signal[i-1] == 1) {
        // If short, flatten first
        if (PositionData.PositionQuantity < 0) {
            sc.FlattenAndCancelAllOrders();
        }
        // Enter long if not already in position
        if (PositionData.PositionQuantity == 0) {
            // Reset consecutive bar counters on new position entry
            ConsecutiveRedBars = 0;
            ConsecutiveGreenBars = 0;
            sc.BuyOrder(newOrder);
        }
        return;
    }

    // Signal = -1 (Short): Go short
    if (Signal[i-1] == -1) {
        // If long, flatten first
        if (PositionData.PositionQuantity > 0) {
            sc.FlattenAndCancelAllOrders();
        }
        // Enter short if not already in position
        if (PositionData.PositionQuantity == 0) {
            // Reset consecutive bar counters on new position entry
            ConsecutiveRedBars = 0;
            ConsecutiveGreenBars = 0;
            sc.SellOrder(newOrder);
        }
        return;
    }
}
