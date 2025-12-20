#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_DeltaVolumeMomentumAlexTrading(SCStudyInterfaceRef sc) {

    SCInputRef DeltaVolumeMomentumSignalStudy = sc.Input[0];
    SCInputRef StopLoss = sc.Input[1];
    SCInputRef TakeProfit = sc.Input[2];
    SCInputRef StartTradingTime = sc.Input[3];
    SCInputRef EndTradingTime = sc.Input[4];
    SCInputRef MaxDailyPLInTicks = sc.Input[5];
    SCInputRef MinDailyPLInTicks = sc.Input[6];
    SCInputRef FlattenOnZeroSignal = sc.Input[7];
    SCInputRef StopAfterWinTicks = sc.Input[8];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Delta Volume Momentum Alex Trading";

        // Inputs
        DeltaVolumeMomentumSignalStudy.Name = "Delta Volume Momentum Signal Study";
        DeltaVolumeMomentumSignalStudy.SetStudyID(1);

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
        FlattenOnZeroSignal.SetDescription("Yes = Exit when signal goes to 0, No = Exit only on SL/TP");

        StopAfterWinTicks.Name = "Stop Trading After Single Win (ticks)";
        StopAfterWinTicks.SetInt(0);
        StopAfterWinTicks.SetIntLimits(0, 1000);
        StopAfterWinTicks.SetDescription("Stop trading for the day after one trade hits this profit (0 = disabled)");

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 1; // No limit on scaling
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = true; // Allow scaling in
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
    int& messagePrinted = sc.GetPersistentInt(1);
    int& bigWinHitToday = sc.GetPersistentInt(2);
    int& lastTradeDate = sc.GetPersistentInt(3);
    int& previousPositionQty = sc.GetPersistentInt(4);
    double& lastCheckedTradePL = sc.GetPersistentDouble(0);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    // Check if within trading hours
    SCDateTime currentTime = sc.BaseDateTimeIn[i].GetTime();
    int currentDate = sc.BaseDateTimeIn[i].GetDate();

    // Reset big win flag at start of new day
    if (currentDate != lastTradeDate) {
        bigWinHitToday = 0;
        lastTradeDate = currentDate;
        messagePrinted = 0;
        // Reset tracking - mark current LastTradeProfitLoss as "already seen" so we don't trigger on yesterday's trade
        lastCheckedTradePL = PositionData.LastTradeProfitLoss;
        previousPositionQty = PositionData.PositionQuantity;
    }

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

    // Check for big win - stop trading after single trade hits threshold
    int stopAfterWinThreshold = StopAfterWinTicks.GetInt();
    if (stopAfterWinThreshold > 0) {
        // Detect trade close: transition from having position to flat AND LastTradeProfitLoss changed
        bool tradeJustClosed = (previousPositionQty != 0 && PositionData.PositionQuantity == 0 &&
                                PositionData.LastTradeProfitLoss != lastCheckedTradePL);

        if (tradeJustClosed) {
            double lastTradePLInTicks = PositionData.LastTradeProfitLoss / sc.CurrencyValuePerTick;
            lastCheckedTradePL = PositionData.LastTradeProfitLoss;

            if (lastTradePLInTicks >= stopAfterWinThreshold && bigWinHitToday == 0) {
                bigWinHitToday = 1;
                SCString msg;
                msg.Format("Big win achieved: %.1f ticks >= %d threshold. Stopping trading for today.",
                          lastTradePLInTicks, stopAfterWinThreshold);
                sc.AddMessageToLog(msg, 0);
            }
        }

        // Update previous position for next check
        previousPositionQty = PositionData.PositionQuantity;

        // If big win already hit today, stop trading
        if (bigWinHitToday == 1 && PositionData.PositionQuantity == 0) {
            return;
        }
    }

    // Get the signal from Delta Volume Momentum Signal study
    SCFloatArray Signal;
    sc.GetStudyArrayUsingID(DeltaVolumeMomentumSignalStudy.GetStudyID(), 2, Signal);

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
        // Otherwise: Keep position, exit only on SL/TP
        return;
    }

    // Signal = 1 (Long): Go long or add to long position
    if (Signal[i-1] == 1) {
        // If short, flatten first
        if (PositionData.PositionQuantity < 0) {
            sc.FlattenAndCancelAllOrders();
        }
        // Always enter long (either new position or adding to existing)
        sc.BuyOrder(newOrder);
        return;
    }

    // Signal = -1 (Short): Go short or add to short position
    if (Signal[i-1] == -1) {
        // If long, flatten first
        if (PositionData.PositionQuantity > 0) {
            sc.FlattenAndCancelAllOrders();
        }
        // Always enter short (either new position or adding to existing)
        sc.SellOrder(newOrder);
        return;
    }
}
