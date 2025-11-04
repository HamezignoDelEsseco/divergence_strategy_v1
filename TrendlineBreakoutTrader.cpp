/*
 * Trendline Breakout Trading Strategy
 *
 * This strategy trades breakouts through trendlines fitted on recent price data.
 *
 * Long Strategy:
 * - Enters long when price breaks above resistance trendline
 * - Stop loss placed at support trendline or fixed distance
 * - Multiple targets with breakeven management
 *
 * Short Strategy:
 * - Enters short when price breaks below support trendline
 * - Stop loss placed at resistance trendline or fixed distance
 * - Multiple targets with breakeven management
 */

#include "helpers.h"
#include "sierrachart.h"

/**
 * Trendline Breakout Trader - Long Only
 * Enters when price breaks above resistance trendline
 */
SCSFExport scsf_TrendlineBreakoutTraderLong(SCStudyInterfaceRef sc) {

    SCInputRef TrendlineSignal = sc.Input[0];
    SCInputRef MaxRiskPerContractInTicks = sc.Input[1];
    SCInputRef MaxTarget = sc.Input[2];
    SCInputRef BEQuantity = sc.Input[3];
    SCInputRef LongQuantity = sc.Input[4];
    SCInputRef EndTradingTime = sc.Input[5];
    SCInputRef MaxDailyPLInTicks = sc.Input[6];
    SCInputRef MinDailyPLInTicks = sc.Input[7];
    SCInputRef UseFixedStop = sc.Input[8];
    SCInputRef FixedStopTicks = sc.Input[9];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Trendline Breakout Trader (Long)";

        // Inputs
        TrendlineSignal.Name = "Trendline Breakout Signal Study";
        TrendlineSignal.SetStudyID(1);

        MaxRiskPerContractInTicks.Name = "Max risk per contract (in ticks)";
        MaxRiskPerContractInTicks.SetIntLimits(3, 100);
        MaxRiskPerContractInTicks.SetInt(15);

        MaxTarget.Name = "Max. target (in ticks)";
        MaxTarget.SetIntLimits(15, 200);
        MaxTarget.SetInt(40);

        BEQuantity.Name = "BE qty (breakeven target)";
        BEQuantity.SetIntLimits(1, 5);
        BEQuantity.SetInt(2);

        LongQuantity.Name = "Long qty (runner)";
        LongQuantity.SetIntLimits(1, 5);
        LongQuantity.SetInt(1);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15, 45, 0));

        MaxDailyPLInTicks.Name = "Maximum daily profit (in ticks)";
        MaxDailyPLInTicks.SetIntLimits(10, 1000);
        MaxDailyPLInTicks.SetInt(1000);

        MinDailyPLInTicks.Name = "Maximum daily loss (in ticks)";
        MinDailyPLInTicks.SetIntLimits(-1000, -10);
        MinDailyPLInTicks.SetInt(-1000);

        UseFixedStop.Name = "Use fixed stop instead of support line";
        UseFixedStop.SetYesNo(0);

        FixedStopTicks.Name = "Fixed stop distance (in ticks)";
        FixedStopTicks.SetIntLimits(5, 50);
        FixedStopTicks.SetInt(12);

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 5;
        sc.MaintainTradeStatisticsAndTradesData = true;

        return;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize - 1)
        return;

    const int i = sc.Index;

    int& barOrderPlaced = sc.GetPersistentInt(1);
    int& messagePrinted = sc.GetPersistentInt(2);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    // Stop trading after end time
    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    // Check daily P&L limits
    if ((PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
        || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("Daily P&L limit reached", 1);
        }
        messagePrinted++;
        return;
    }

    // Get signal arrays from Trendline Breakout Signal study
    SCFloatArray TradeSignal;
    SCFloatArray SupportValue;
    SCFloatArray ResistValue;
    sc.GetStudyArrayUsingID(TrendlineSignal.GetStudyID(), 0, TradeSignal);
    sc.GetStudyArrayUsingID(TrendlineSignal.GetStudyID(), 1, SupportValue);
    sc.GetStudyArrayUsingID(TrendlineSignal.GetStudyID(), 2, ResistValue);

    // Check for long breakout signal (TradeSignal == 1)
    if (TradeSignal[i] == 1.0 && PositionData.PositionQuantity == 0) {

        s_SCNewOrder newOrder;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.TimeInForce = SCT_TIF_DAY;
        newOrder.OrderQuantity = BEQuantity.GetInt() + LongQuantity.GetInt();
        newOrder.OCOGroup1Quantity = BEQuantity.GetInt();
        newOrder.OCOGroup2Quantity = LongQuantity.GetInt();

        // Calculate stop loss
        float stopPrice;
        if (UseFixedStop.GetYesNo()) {
            // Fixed stop distance from entry
            stopPrice = sc.Close[i] - (FixedStopTicks.GetInt() * sc.TickSize);
        } else {
            // Use support trendline as stop
            stopPrice = SupportValue[i];

            // Safety check: ensure stop is not too far
            float riskInTicks = (sc.Close[i] - stopPrice) / sc.TickSize;
            if (riskInTicks > MaxRiskPerContractInTicks.GetInt()) {
                // Risk too high, skip this trade
                sc.AddMessageToLog("Long trade skipped: risk exceeds maximum", 0);
                return;
            }
        }

        newOrder.Stop1Price = stopPrice;
        newOrder.Stop2Price = stopPrice;

        // Calculate first target (BE target)
        float riskInTicks = (sc.Close[i] - stopPrice) / sc.TickSize;
        float firstTargetTicks = std::min<float>(riskInTicks * 1.5f, MaxTarget.GetFloat() / 2.0f);
        firstTargetTicks = std::max<float>(firstTargetTicks, 8.0f); // Minimum 8 ticks

        newOrder.Target1Offset = firstTargetTicks * sc.TickSize;
        newOrder.Target2Offset = MaxTarget.GetFloat() * sc.TickSize;

        newOrder.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP_LIMIT;
        newOrder.AttachedOrderStop2Type = SCT_ORDERTYPE_STOP_LIMIT;

        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_LIMIT;
        newOrder.AttachedOrderTarget2Type = SCT_ORDERTYPE_LIMIT;

        // Move to breakeven after first target is close to being hit
        newOrder.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        newOrder.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 3;
        newOrder.MoveToBreakEven.TriggerOffsetInTicks = static_cast<int>(firstTargetTicks * 0.7f);

        if (const double orderPlaced = sc.BuyEntry(newOrder); orderPlaced > 0) {
            barOrderPlaced = i;
            SCString message;
            message.Format("Long breakout entry at %.2f, stop at %.2f, risk %.1f ticks",
                          sc.Close[i], stopPrice, riskInTicks);
            sc.AddMessageToLog(message, 0);
        }
    }
}

/**
 * Trendline Breakout Trader - Short Only
 * Enters when price breaks below support trendline
 */
SCSFExport scsf_TrendlineBreakoutTraderShort(SCStudyInterfaceRef sc) {

    SCInputRef TrendlineSignal = sc.Input[0];
    SCInputRef MaxRiskPerContractInTicks = sc.Input[1];
    SCInputRef MaxTarget = sc.Input[2];
    SCInputRef BEQuantity = sc.Input[3];
    SCInputRef ShortQuantity = sc.Input[4];
    SCInputRef EndTradingTime = sc.Input[5];
    SCInputRef MaxDailyPLInTicks = sc.Input[6];
    SCInputRef MinDailyPLInTicks = sc.Input[7];
    SCInputRef UseFixedStop = sc.Input[8];
    SCInputRef FixedStopTicks = sc.Input[9];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Trendline Breakout Trader (Short)";

        // Inputs
        TrendlineSignal.Name = "Trendline Breakout Signal Study";
        TrendlineSignal.SetStudyID(1);

        MaxRiskPerContractInTicks.Name = "Max risk per contract (in ticks)";
        MaxRiskPerContractInTicks.SetIntLimits(3, 100);
        MaxRiskPerContractInTicks.SetInt(15);

        MaxTarget.Name = "Max. target (in ticks)";
        MaxTarget.SetIntLimits(15, 200);
        MaxTarget.SetInt(40);

        BEQuantity.Name = "BE qty (breakeven target)";
        BEQuantity.SetIntLimits(1, 5);
        BEQuantity.SetInt(2);

        ShortQuantity.Name = "Short qty (runner)";
        ShortQuantity.SetIntLimits(1, 5);
        ShortQuantity.SetInt(1);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15, 45, 0));

        MaxDailyPLInTicks.Name = "Maximum daily profit (in ticks)";
        MaxDailyPLInTicks.SetIntLimits(10, 1000);
        MaxDailyPLInTicks.SetInt(1000);

        MinDailyPLInTicks.Name = "Maximum daily loss (in ticks)";
        MinDailyPLInTicks.SetIntLimits(-1000, -10);
        MinDailyPLInTicks.SetInt(-1000);

        UseFixedStop.Name = "Use fixed stop instead of resistance line";
        UseFixedStop.SetYesNo(0);

        FixedStopTicks.Name = "Fixed stop distance (in ticks)";
        FixedStopTicks.SetIntLimits(5, 50);
        FixedStopTicks.SetInt(12);

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 5;
        sc.MaintainTradeStatisticsAndTradesData = true;

        return;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize - 1)
        return;

    const int i = sc.Index;

    int& barOrderPlaced = sc.GetPersistentInt(1);
    int& messagePrinted = sc.GetPersistentInt(2);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    // Stop trading after end time
    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    // Check daily P&L limits
    if ((PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
        || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("Daily P&L limit reached", 1);
        }
        messagePrinted++;
        return;
    }

    // Get signal arrays from Trendline Breakout Signal study
    SCFloatArray TradeSignal;
    SCFloatArray SupportValue;
    SCFloatArray ResistValue;
    sc.GetStudyArrayUsingID(TrendlineSignal.GetStudyID(), 0, TradeSignal);
    sc.GetStudyArrayUsingID(TrendlineSignal.GetStudyID(), 1, SupportValue);
    sc.GetStudyArrayUsingID(TrendlineSignal.GetStudyID(), 2, ResistValue);

    // Check for short breakdown signal (TradeSignal == -1)
    if (TradeSignal[i] == -1.0 && PositionData.PositionQuantity == 0) {

        s_SCNewOrder newOrder;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.TimeInForce = SCT_TIF_DAY;
        newOrder.OrderQuantity = BEQuantity.GetInt() + ShortQuantity.GetInt();
        newOrder.OCOGroup1Quantity = BEQuantity.GetInt();
        newOrder.OCOGroup2Quantity = ShortQuantity.GetInt();

        // Calculate stop loss
        float stopPrice;
        if (UseFixedStop.GetYesNo()) {
            // Fixed stop distance from entry
            stopPrice = sc.Close[i] + (FixedStopTicks.GetInt() * sc.TickSize);
        } else {
            // Use resistance trendline as stop
            stopPrice = ResistValue[i];

            // Safety check: ensure stop is not too far
            float riskInTicks = (stopPrice - sc.Close[i]) / sc.TickSize;
            if (riskInTicks > MaxRiskPerContractInTicks.GetInt()) {
                // Risk too high, skip this trade
                sc.AddMessageToLog("Short trade skipped: risk exceeds maximum", 0);
                return;
            }
        }

        newOrder.Stop1Price = stopPrice;
        newOrder.Stop2Price = stopPrice;

        // Calculate first target (BE target)
        float riskInTicks = (stopPrice - sc.Close[i]) / sc.TickSize;
        float firstTargetTicks = std::min<float>(riskInTicks * 1.5f, MaxTarget.GetFloat() / 2.0f);
        firstTargetTicks = std::max<float>(firstTargetTicks, 8.0f); // Minimum 8 ticks

        newOrder.Target1Offset = firstTargetTicks * sc.TickSize;
        newOrder.Target2Offset = MaxTarget.GetFloat() * sc.TickSize;

        newOrder.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP_LIMIT;
        newOrder.AttachedOrderStop2Type = SCT_ORDERTYPE_STOP_LIMIT;

        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_LIMIT;
        newOrder.AttachedOrderTarget2Type = SCT_ORDERTYPE_LIMIT;

        // Move to breakeven after first target is close to being hit
        newOrder.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        newOrder.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 3;
        newOrder.MoveToBreakEven.TriggerOffsetInTicks = static_cast<int>(firstTargetTicks * 0.7f);

        if (const double orderPlaced = sc.SellEntry(newOrder); orderPlaced > 0) {
            barOrderPlaced = i;
            SCString message;
            message.Format("Short breakdown entry at %.2f, stop at %.2f, risk %.1f ticks",
                          sc.Close[i], stopPrice, riskInTicks);
            sc.AddMessageToLog(message, 0);
        }
    }
}




