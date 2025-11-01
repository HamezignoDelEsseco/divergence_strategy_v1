#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_EndOfStreakTraderLong(SCStudyInterfaceRef sc) {

    SCInputRef EOSSignal = sc.Input[0];
    SCInputRef MaxRiskPerContractInTicks = sc.Input[1];
    SCInputRef MaxTarget = sc.Input[2];
    SCInputRef BEQuantity = sc.Input[3];
    SCInputRef LongQuantity = sc.Input[4];
    SCInputRef EndTradingTime = sc.Input[5];
    SCInputRef MaxDailyPLInTicks = sc.Input[6];
    SCInputRef MinDailyPLInTicks = sc.Input[7];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "End of streak trader (long)";

        // Inputs
        EOSSignal.Name = "End of streak signal";
        EOSSignal.SetStudyID(1);

        MaxRiskPerContractInTicks.Name = "Max risk per contract (in ticks)";
        MaxRiskPerContractInTicks.SetIntLimits(3, 50);
        MaxRiskPerContractInTicks.SetInt(8);

        MaxTarget.Name = "Max. target";
        MaxTarget.SetIntLimits(15, 200);
        MaxTarget.SetInt(30);

        BEQuantity.Name = "BE qty";
        BEQuantity.SetIntLimits(1, 2);
        BEQuantity.SetInt(2);

        LongQuantity.Name = "Long qty";
        LongQuantity.SetIntLimits(1, 2);
        LongQuantity.SetInt(1);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15,45,0));

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day"; // Total quantity that breaks even
        MaxDailyPLInTicks.SetIntLimits(10, 100);
        MaxDailyPLInTicks.SetInt(1000);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day"; // Total quantity that breaks even
        MinDailyPLInTicks.SetIntLimits(-100, -10);
        MinDailyPLInTicks.SetInt(-1000);

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 3;
        sc.MaintainTradeStatisticsAndTradesData = true;

    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

    const int i = sc.Index;

    int& barOderPlaced = sc.GetPersistentInt(1);
    int& messagePrinted = sc.GetPersistentInt(2);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    if (
        (PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
            || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0
        ) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("You have exceeded your daily loss", 1);
        }
        messagePrinted ++;

        return;
        }

    SCFloatArray TradeSignal;
    SCFloatArray TakeFirstTarget;
    SCFloatArray StopLoss;
    sc.GetStudyArrayUsingID(EOSSignal.GetStudyID(), 0, TradeSignal);
    sc.GetStudyArrayUsingID(EOSSignal.GetStudyID(), 1, StopLoss);
    sc.GetStudyArrayUsingID(EOSSignal.GetStudyID(), 2, TakeFirstTarget);

    if (TradeSignal[i] == 1 && TakeFirstTarget[i] <= MaxRiskPerContractInTicks.GetFloat()) { // We are long
        s_SCNewOrder newOrder;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.OrderQuantity = BEQuantity.GetInt() + LongQuantity.GetInt();
        newOrder.OCOGroup1Quantity = BEQuantity.GetInt();
        newOrder.OCOGroup2Quantity = LongQuantity.GetInt();

        newOrder.Stop1Price = StopLoss[i];
        newOrder.Stop2Price = StopLoss[i];

        newOrder.Target1Offset = std::max<float>(TakeFirstTarget[i] * sc.TickSize, 8 * sc.TickSize);
        newOrder.Target2Offset = MaxTarget.GetFloat() * sc.TickSize;

        newOrder.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP_LIMIT;
        newOrder.AttachedOrderStop2Type = SCT_ORDERTYPE_STOP_LIMIT;

        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_LIMIT;
        newOrder.AttachedOrderTarget2Type = SCT_ORDERTYPE_LIMIT;

        newOrder.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        newOrder.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 5;
        newOrder.MoveToBreakEven.TriggerOffsetInTicks = 20;

        if (const double orderPlaced = sc.BuyEntry(newOrder); orderPlaced > 0) {
            barOderPlaced = i;
        }
    }
}


SCSFExport scsf_EndOfStreakTraderLagLong(SCStudyInterfaceRef sc) {

    SCInputRef EOSSignal = sc.Input[0];
    SCInputRef MaxRiskPerContractInTicks = sc.Input[1];
    SCInputRef MaxTarget = sc.Input[2];
    SCInputRef BEQuantity = sc.Input[3];
    SCInputRef LongQuantity = sc.Input[4];
    SCInputRef EndTradingTime = sc.Input[5];
    SCInputRef MaxDailyPLInTicks = sc.Input[6];
    SCInputRef MinDailyPLInTicks = sc.Input[7];
    SCInputRef MarketIfTouched = sc.Input[8];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "End of streak trader lag (long)";

        // Inputs
        EOSSignal.Name = "End of streak signal";
        EOSSignal.SetStudyID(1);

        MaxRiskPerContractInTicks.Name = "Max risk per contract (in ticks)";
        MaxRiskPerContractInTicks.SetIntLimits(3, 50);
        MaxRiskPerContractInTicks.SetInt(8);

        MaxTarget.Name = "Max. target";
        MaxTarget.SetIntLimits(15, 200);
        MaxTarget.SetInt(30);

        BEQuantity.Name = "BE qty";
        BEQuantity.SetIntLimits(1, 2);
        BEQuantity.SetInt(2);

        LongQuantity.Name = "Long qty";
        LongQuantity.SetIntLimits(1, 2);
        LongQuantity.SetInt(1);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15,45,0));

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day"; // Total quantity that breaks even
        MaxDailyPLInTicks.SetIntLimits(10, 100);
        MaxDailyPLInTicks.SetInt(1000);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day"; // Total quantity that breaks even
        MinDailyPLInTicks.SetIntLimits(-100, -10);
        MinDailyPLInTicks.SetInt(-1000);

        MarketIfTouched.Name = "Yes for MITO, 0 for MO"; // Total quantity that breaks even
        MarketIfTouched.SetYesNo(1);

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;
        sc.MaximumPositionAllowed = 3;
        sc.MaintainTradeStatisticsAndTradesData = true;

    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

    const int i = sc.Index;

    int& barOderPlaced = sc.GetPersistentInt(1);
    int& messagePrinted = sc.GetPersistentInt(2);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    if (
        (PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
            || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0
        ) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("You have exceeded your daily loss", 1);
        }
        messagePrinted ++;

        return;
        }

    SCFloatArray TradeSignal;
    SCFloatArray StopLoss;
    sc.GetStudyArrayUsingID(EOSSignal.GetStudyID(), 0, TradeSignal);
    sc.GetStudyArrayUsingID(EOSSignal.GetStudyID(), 1, StopLoss);

    if (TradeSignal[i-1] == 1 && sc.High[i] > sc.High[i-1]) { // We are long
        const double entryPrice = sc.High[i-1] - sc.TickSize * 2;

        const double fixedRisk = MaxRiskPerContractInTicks.GetDouble() * sc.TickSize;
        const double riskOffsetPrice = std::min<double>(
            fixedRisk,
            entryPrice - std::min<double>(sc.Low[i-1], sc.Low[i-2]) + sc.TickSize
        );

        s_SCNewOrder newOrder;
        newOrder.TimeInForce = SCT_TIF_DAY;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;

        if (MarketIfTouched.GetYesNo() == 1) {
            newOrder.OrderType = SCT_ORDERTYPE_MARKET_IF_TOUCHED;
            newOrder.Price1 = entryPrice;
        }
        newOrder.OrderQuantity = BEQuantity.GetInt() + LongQuantity.GetInt();
        newOrder.OCOGroup1Quantity = BEQuantity.GetInt();
        newOrder.OCOGroup2Quantity = LongQuantity.GetInt();

        newOrder.Stop1Offset = riskOffsetPrice;
        newOrder.Stop2Offset = riskOffsetPrice;

        newOrder.Target1Offset = riskOffsetPrice / 2 + sc.TickSize;
        newOrder.Target2Offset = MaxTarget.GetFloat() * sc.TickSize;

        newOrder.AttachedOrderStop1Type = SCT_ORDERTYPE_STOP_LIMIT;
        newOrder.AttachedOrderStop2Type = SCT_ORDERTYPE_STOP_LIMIT;

        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_LIMIT;
        newOrder.AttachedOrderTarget2Type = SCT_ORDERTYPE_LIMIT;

        newOrder.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        newOrder.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 1;
        newOrder.MoveToBreakEven.TriggerOffsetInTicks = 20;

        if (const double orderPlaced = sc.BuyEntry(newOrder); orderPlaced > 0) {
            barOderPlaced = i;
        }
    }
}

SCSFExport scsf_EndOfStreakTraderLagLongShort(SCStudyInterfaceRef sc) {

    SCInputRef EOSSignal = sc.Input[0];
    SCInputRef VVAStudy = sc.Input[1];
    SCInputRef MaxTarget = sc.Input[2];
    SCInputRef LongQuantity = sc.Input[4];
    SCInputRef EndTradingTime = sc.Input[5];
    SCInputRef MaxDailyPLInTicks = sc.Input[6];
    SCInputRef MinDailyPLInTicks = sc.Input[7];
    SCInputRef MarketIfTouched = sc.Input[8];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "End of streak trader lag (long+short)";

        // Inputs
        EOSSignal.Name = "End of streak signal";
        EOSSignal.SetStudyID(1);

        VVAStudy.Name = "VVA Study";
        VVAStudy.SetStudyID(1);

        MaxTarget.Name = "Max. target";
        MaxTarget.SetIntLimits(15, 200);
        MaxTarget.SetInt(30);

        LongQuantity.Name = "Long qty";
        LongQuantity.SetIntLimits(1, 2);
        LongQuantity.SetInt(1);

        EndTradingTime.Name = "End trading time";
        EndTradingTime.SetTime(HMS_TIME(15,45,0));

        MaxDailyPLInTicks.Name = "Maximum PL in ticks per day"; // Total quantity that breaks even
        MaxDailyPLInTicks.SetIntLimits(10, 1000);
        MaxDailyPLInTicks.SetInt(300);

        MinDailyPLInTicks.Name = "Minimum PL in ticks per day"; // Total quantity that breaks even
        MinDailyPLInTicks.SetIntLimits(-1000, -10);
        MinDailyPLInTicks.SetInt(-100);

        MarketIfTouched.Name = "Yes for MITO, 0 for MO";
        MarketIfTouched.SetYesNo(1);

        // Flags
        sc.HideStudy = true;
        sc.SupportReversals = false;
        sc.AllowOnlyOneTradePerBar = true;

        sc.MaximumPositionAllowed = 3;

        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = true;
        sc.AllowEntryWithWorkingOrders = true;
    }

    if (sc.LastCallToFunction)
        return;

    if (sc.IsFullRecalculation)
        return;

    if (sc.Index < sc.ArraySize-1)
        return;

    const int i = sc.Index;
    double estimatedEntryPrice;
    int& messagePrinted = sc.GetPersistentInt(2);

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sc.BaseDateTimeIn[i].GetTime() > EndTradingTime.GetTime()) {
        if (PositionData.PositionQuantity != 0) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    if (
        (PositionData.DailyProfitLoss / sc.CurrencyValuePerTick >= MaxDailyPLInTicks.GetInt()
            || PositionData.DailyProfitLoss / sc.CurrencyValuePerTick <= MinDailyPLInTicks.GetInt())
        && PositionData.PositionQuantity == 0
        ) {
        if (messagePrinted == 0) {
            sc.AddMessageToLog("You have exceeded your daily loss / profit", 1);
        }
        messagePrinted ++;

        return;
        }

    SCFloatArray VVAH;
    SCFloatArray VVAL;
    sc.GetStudyArrayUsingID(VVAStudy.GetStudyID(), 1, VVAH);
    sc.GetStudyArrayUsingID(VVAStudy.GetStudyID(), 2, VVAL);

    SCFloatArray TradeSignal;
    SCFloatArray StopLoss;
    sc.GetStudyArrayUsingID(EOSSignal.GetStudyID(), 0, TradeSignal);
    sc.GetStudyArrayUsingID(EOSSignal.GetStudyID(), 1, StopLoss);

    if (TradeSignal[i] == 1 && PositionData.PositionQuantity >= 0) {
        estimatedEntryPrice = sc.Close[i] + sc.TickSize;
        s_SCNewOrder newOrder;
        newOrder.OrderQuantity = 1;
        newOrder.TimeInForce = SCT_TIF_DAY;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.Stop1Price = StopLoss[i];
        newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_STOP_LIMIT;
        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_STOP_LIMIT;


        if (estimatedEntryPrice >= VVAH[i]) {
            newOrder.Target1Offset = MaxTarget.GetFloat() * sc.TickSize;
        } else {
            newOrder.Target1Price = std::min<double>(
                estimatedEntryPrice > VVAH[i] ? VVAH[i] : VVAL[i],
                estimatedEntryPrice + sc.TickSize * 70
                );
        }

        newOrder.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        newOrder.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 1;
        newOrder.MoveToBreakEven.TriggerOffsetInTicks = 20;

        sc.BuyOrder(newOrder);
    }

    if (TradeSignal[i] == -1 && PositionData.PositionQuantity <= 0) {
        estimatedEntryPrice = sc.Close[i] - sc.TickSize;
        s_SCNewOrder newOrder;
        newOrder.OrderQuantity = 1;

        newOrder.TimeInForce = SCT_TIF_DAY;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.Stop1Price = StopLoss[i];
        newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_STOP_LIMIT;
        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_STOP_LIMIT;


        if (estimatedEntryPrice <= VVAL[i]) {
            newOrder.Target1Offset = MaxTarget.GetFloat() * sc.TickSize;
        } else {
            newOrder.Target1Price = std::max<double>(
                estimatedEntryPrice > VVAH[i] ? VVAH[i] : VVAL[i],
                estimatedEntryPrice - sc.TickSize * 70
                );
        }

        newOrder.MoveToBreakEven.Type = MOVETO_BE_ACTION_TYPE_OFFSET_TRIGGERED;
        newOrder.MoveToBreakEven.BreakEvenLevelOffsetInTicks = 1;
        newOrder.MoveToBreakEven.TriggerOffsetInTicks = 20;

        sc.SellOrder(newOrder);

    }

    if (PositionData.OpenProfitLoss / sc.CurrencyValuePerTick >= 105) {
        sc.FlattenAndCancelAllOrders();
    }

    if (PositionData.OpenProfitLoss / sc.CurrencyValuePerTick + PositionData.DailyProfitLoss / sc.CurrencyValuePerTick>= 105) {
        sc.FlattenAndCancelAllOrders();
    }
}