#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_VALevelSignalAlexTrading(SCStudyInterfaceRef sc) {

    SCInputRef SignalStudy = sc.Input[0];
    SCInputRef StopLossTicks = sc.Input[1];
    SCInputRef TakeProfitTicks = sc.Input[2];
    SCInputRef StartTradingTime = sc.Input[3];
    SCInputRef EndTradingTime = sc.Input[4];
    SCInputRef MaxDailyPLInTicks = sc.Input[5];
    SCInputRef MinDailyPLInTicks = sc.Input[6];
    SCInputRef BreakevenTriggerTicks = sc.Input[7];
    SCInputRef TrailingStepTicks = sc.Input[8];
    SCInputRef UseTrailingStop = sc.Input[9];

    SCSubgraphRef EntryPriceGraph = sc.Subgraph[0];
    SCSubgraphRef StopPriceGraph = sc.Subgraph[1];
    SCSubgraphRef MaxProfitGraph = sc.Subgraph[2];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "VA Level Signal Alex Trading";

        SignalStudy.Name = "VA Level Signal Study";
        SignalStudy.SetStudyID(1);

        StopLossTicks.Name = "Initial Stop Loss (ticks)";
        StopLossTicks.SetInt(40);
        StopLossTicks.SetIntLimits(1, 500);

        TakeProfitTicks.Name = "Take Profit (ticks) - 0 for no TP";
        TakeProfitTicks.SetInt(0);
        TakeProfitTicks.SetIntLimits(0, 500);

        StartTradingTime.Name = "Start Trading Time";
        StartTradingTime.SetTime(HMS_TIME(8, 0, 0));

        EndTradingTime.Name = "End Trading Time";
        EndTradingTime.SetTime(HMS_TIME(15, 45, 0));

        MaxDailyPLInTicks.Name = "Max Daily P&L (ticks)";
        MaxDailyPLInTicks.SetInt(300);
        MaxDailyPLInTicks.SetIntLimits(10, 1000);

        MinDailyPLInTicks.Name = "Min Daily P&L (ticks)";
        MinDailyPLInTicks.SetInt(-100);
        MinDailyPLInTicks.SetIntLimits(-1000, -10);

        BreakevenTriggerTicks.Name = "Breakeven Trigger (ticks)";
        BreakevenTriggerTicks.SetInt(20);
        BreakevenTriggerTicks.SetIntLimits(1, 200);
        BreakevenTriggerTicks.SetDescription("Move SL to entry when profit reaches this");

        TrailingStepTicks.Name = "Trailing Step (ticks)";
        TrailingStepTicks.SetInt(10);
        TrailingStepTicks.SetIntLimits(1, 100);
        TrailingStepTicks.SetDescription("Trail SL by this amount after breakeven");

        UseTrailingStop.Name = "Use Trailing Stop";
        UseTrailingStop.SetYesNo(1);

        EntryPriceGraph.Name = "Entry Price";
        EntryPriceGraph.DrawStyle = DRAWSTYLE_LINE;
        EntryPriceGraph.PrimaryColor = RGB(255, 255, 0);

        StopPriceGraph.Name = "Current Stop";
        StopPriceGraph.DrawStyle = DRAWSTYLE_LINE;
        StopPriceGraph.PrimaryColor = RGB(255, 0, 0);

        MaxProfitGraph.Name = "Max Profit Ticks";
        MaxProfitGraph.DrawStyle = DRAWSTYLE_IGNORE;

        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 1;
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = false;
        sc.AllowEntryWithWorkingOrders = false;
        sc.SupportReversals = true;

        return;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize - 1)
        return;

    const int i = sc.Index;

    // Persistent variables
    float& entryPrice = sc.GetPersistentFloat(1);
    float& currentStopPrice = sc.GetPersistentFloat(2);
    float& maxFavorablePrice = sc.GetPersistentFloat(3);
    int& tradeDirection = sc.GetPersistentInt(1);  // 1 = long, -1 = short, 0 = flat
    int& dailyPLMessagePrinted = sc.GetPersistentInt(2);
    int& lastTradeDate = sc.GetPersistentInt(3);

    // Reset daily tracking on new day
    int currentDate = sc.BaseDateTimeIn[i].GetDate();
    if (currentDate != lastTradeDate) {
        lastTradeDate = currentDate;
        dailyPLMessagePrinted = 0;
    }

    // Get position data
    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    // Check if within trading hours
    SCDateTime currentTime = sc.BaseDateTimeIn[i].GetTime();

    if (currentTime < StartTradingTime.GetTime()) {
        return;
    }

    if (currentTime > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
            entryPrice = 0;
            currentStopPrice = 0;
            maxFavorablePrice = 0;
            tradeDirection = 0;
        }
        return;
    }

    // Check daily P&L limits
    float dailyPLTicks = PositionData.DailyProfitLoss / sc.CurrencyValuePerTick;
    if ((dailyPLTicks >= MaxDailyPLInTicks.GetInt() || dailyPLTicks <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0) {
        if (dailyPLMessagePrinted == 0) {
            sc.AddMessageToLog("Daily P&L limit reached - no more trading today", 1);
            dailyPLMessagePrinted = 1;
        }
        return;
    }

    // Get the signal from previous bar (closed bar, not forming bar)
    SCFloatArray Signal;
    sc.GetStudyArrayUsingID(SignalStudy.GetStudyID(), 0, Signal);

    float currentSignal = Signal[i - 1];

    // Update graphs
    EntryPriceGraph[i] = entryPrice;
    StopPriceGraph[i] = currentStopPrice;

    // === POSITION MANAGEMENT (Trailing Stop) ===
    if (PositionData.PositionQuantity != 0 && UseTrailingStop.GetYesNo()) {
        float currentPrice = sc.Close[i];

        // Update max favorable price
        if (tradeDirection == 1) {  // Long
            if (currentPrice > maxFavorablePrice || maxFavorablePrice == 0) {
                maxFavorablePrice = currentPrice;
            }

            float maxProfitTicks = (maxFavorablePrice - entryPrice) / sc.TickSize;
            MaxProfitGraph[i] = maxProfitTicks;

            // Calculate new stop level
            float newStopPrice = currentStopPrice;

            if (maxProfitTicks >= BreakevenTriggerTicks.GetInt()) {
                // At least breakeven
                float trailingStops = floor((maxProfitTicks - BreakevenTriggerTicks.GetInt()) / TrailingStepTicks.GetInt());
                newStopPrice = entryPrice + (trailingStops * TrailingStepTicks.GetInt() * sc.TickSize);

                // Make sure new stop is higher than current stop
                if (newStopPrice > currentStopPrice) {
                    currentStopPrice = newStopPrice;

                    SCString msg;
                    msg.Format("Trailing Stop Updated - Long: New SL = %.2f (Profit: %.0f ticks)", currentStopPrice, maxProfitTicks);
                    sc.AddMessageToLog(msg, 0);
                }
            }

            // Check if price hit our trailing stop
            if (currentPrice <= currentStopPrice && currentStopPrice > 0) {
                sc.FlattenAndCancelAllOrders();
                entryPrice = 0;
                currentStopPrice = 0;
                maxFavorablePrice = 0;
                tradeDirection = 0;
                return;
            }

        } else if (tradeDirection == -1) {  // Short
            if (currentPrice < maxFavorablePrice || maxFavorablePrice == 0) {
                maxFavorablePrice = currentPrice;
            }

            float maxProfitTicks = (entryPrice - maxFavorablePrice) / sc.TickSize;
            MaxProfitGraph[i] = maxProfitTicks;

            // Calculate new stop level
            float newStopPrice = currentStopPrice;

            if (maxProfitTicks >= BreakevenTriggerTicks.GetInt()) {
                // At least breakeven
                float trailingStops = floor((maxProfitTicks - BreakevenTriggerTicks.GetInt()) / TrailingStepTicks.GetInt());
                newStopPrice = entryPrice - (trailingStops * TrailingStepTicks.GetInt() * sc.TickSize);

                // Make sure new stop is lower than current stop (for shorts)
                if (newStopPrice < currentStopPrice || currentStopPrice == 0) {
                    currentStopPrice = newStopPrice;

                    SCString msg;
                    msg.Format("Trailing Stop Updated - Short: New SL = %.2f (Profit: %.0f ticks)", currentStopPrice, maxProfitTicks);
                    sc.AddMessageToLog(msg, 0);
                }
            }

            // Check if price hit our trailing stop
            if (currentPrice >= currentStopPrice && currentStopPrice > 0) {
                sc.FlattenAndCancelAllOrders();
                entryPrice = 0;
                currentStopPrice = 0;
                maxFavorablePrice = 0;
                tradeDirection = 0;
                return;
            }
        }
    }

    // Reset tracking when flat
    if (PositionData.PositionQuantity == 0) {
        entryPrice = 0;
        currentStopPrice = 0;
        maxFavorablePrice = 0;
        tradeDirection = 0;
    }

    // === ENTRY LOGIC ===
    s_SCNewOrder newOrder;
    newOrder.OrderQuantity = 1;
    newOrder.TimeInForce = SCT_TIF_DAY;
    newOrder.OrderType = SCT_ORDERTYPE_MARKET;

    // Set initial stop and optional take profit
    newOrder.Stop1Offset = StopLossTicks.GetInt() * sc.TickSize;
    if (TakeProfitTicks.GetInt() > 0) {
        newOrder.Target1Offset = TakeProfitTicks.GetInt() * sc.TickSize;
    }

    // Signal = 0: No action
    if (currentSignal == 0) {
        return;
    }

    // Signal = 1: Go Long
    if (currentSignal == 1) {
        if (PositionData.PositionQuantity < 0) {
            sc.FlattenAndCancelAllOrders();
        }
        if (PositionData.PositionQuantity <= 0) {
            int result = sc.BuyOrder(newOrder);
            if (result > 0) {
                entryPrice = sc.Close[i];
                currentStopPrice = entryPrice - (StopLossTicks.GetInt() * sc.TickSize);
                maxFavorablePrice = entryPrice;
                tradeDirection = 1;

                SCString msg;
                msg.Format("LONG Entry at %.2f, Initial SL at %.2f", entryPrice, currentStopPrice);
                sc.AddMessageToLog(msg, 0);
            }
        }
        return;
    }

    // Signal = -1: Go Short
    if (currentSignal == -1) {
        if (PositionData.PositionQuantity > 0) {
            sc.FlattenAndCancelAllOrders();
        }
        if (PositionData.PositionQuantity >= 0) {
            int result = sc.SellOrder(newOrder);
            if (result > 0) {
                entryPrice = sc.Close[i];
                currentStopPrice = entryPrice + (StopLossTicks.GetInt() * sc.TickSize);
                maxFavorablePrice = entryPrice;
                tradeDirection = -1;

                SCString msg;
                msg.Format("SHORT Entry at %.2f, Initial SL at %.2f", entryPrice, currentStopPrice);
                sc.AddMessageToLog(msg, 0);
            }
        }
        return;
    }
}
