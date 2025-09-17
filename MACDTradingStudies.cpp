#include "sierrachart.h"
#include "TradeWrapper.h"
#include "helpers.h"

SCSFExport scsf_StrategyMACDShort(SCStudyInterfaceRef sc) {
    /*
     Shorting Red MACD when:
    - The last xover is RED AND MACD goes negative
    - OR there is green xover and red xover in next bar
    - Trading either up to N Target ticks OR to green MACD
    - Can choose whether to only short when the price is below the EWA in the chart
    - Make sure the MACD is negative enough for the strategy. Having a MACD bouncing around 0 is a recipe for failure
    - The number of ticks that we take should be a function of the MACD value: if VERY negative then we can aim for higher ticks (long)
    - If very positive: we can aim for more ticks (Short)
    - Exit rules: make sure that if at some point in the trade the max P&L has down-ticked by 10, get out to at least get some snacks
    */
    SCInputRef PriceEMWAStudy = sc.Input[0];
    SCInputRef MACDXStudy = sc.Input[1];
    SCInputRef ATRStudy = sc.Input[2];

    // SCInputRef ADXStudy = sc.Input[1];

    // SCInputRef RangeTrendADXThresh = sc.Input[3];
    SCInputRef MaxMACDDiff = sc.Input[3];
    SCInputRef PriceEMWAMinOffset = sc.Input[4];
    SCInputRef UseRangeOnly = sc.Input[5];
    SCInputRef OneTradePerPeriod = sc.Input[6]; // One the trade ended in the "red period", don't perform any other trade in this period
    SCInputRef UseEWAThresh = sc.Input[7];
    SCInputRef GiveBackTicks = sc.Input[8];
    SCInputRef MaxTicksEntryFromCrossOVer = sc.Input[9];
    SCInputRef AllowTradingAlways = sc.Input[10];

    SCSubgraphRef TradeId = sc.Subgraph[0];
    SCSubgraphRef CumMaxOpenPnL = sc.Subgraph[1];
    SCSubgraphRef CurrentOpenPnL = sc.Subgraph[2];
    SCSubgraphRef posAveragePrice = sc.Subgraph[3];
    SCSubgraphRef posLowPrice = sc.Subgraph[4];
    SCSubgraphRef posHighPrice = sc.Subgraph[5];
    SCSubgraphRef lastTradeIndex = sc.Subgraph[6];
    SCSubgraphRef lastXOverIndex = sc.Subgraph[7];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trading MACD Short Exec";

        // The Study
        PriceEMWAStudy.Name = "PriceEMWA";
        PriceEMWAStudy.SetStudyID(6);

        //ADXStudy.Name = "ADX";
        //ADXStudy.SetStudyID(0);

        MACDXStudy.Name = "MACD CrossOver";
        MACDXStudy.SetStudyID(7);

        ATRStudy.Name = "ATR";
        ATRStudy.SetStudyID(2);

        MaxMACDDiff.Name = "Max MACDDiff";
        MaxMACDDiff.SetFloatLimits(-1., 0.0);
        MaxMACDDiff.SetFloat(-0.2);

        PriceEMWAMinOffset.Name = "Price EMWA Tick Offset";
        PriceEMWAMinOffset.SetIntLimits(0, 20);
        PriceEMWAMinOffset.SetInt(5);

        MaxTicksEntryFromCrossOVer.Name = "Maximum number of ticks entry after cross over";
        MaxTicksEntryFromCrossOVer.SetIntLimits(1, 20);
        MaxTicksEntryFromCrossOVer.SetInt(6);


        UseRangeOnly.Name = "Trade in range only";
        UseRangeOnly.SetYesNo(0);

        UseEWAThresh.Name = "Trade above/below EWA only";
        UseEWAThresh.SetYesNo(0);

        OneTradePerPeriod.Name = "Trade above/below EWA only";
        OneTradePerPeriod.SetYesNo(0);

        GiveBackTicks.Name = "Give back in ticks";
        GiveBackTicks.SetIntLimits(0, 20);
        GiveBackTicks.SetInt(10);

        AllowTradingAlways.Name = "Allow trading always";
        AllowTradingAlways.SetYesNo(0);

        TradeId.Name = "Trade ID";
        CumMaxOpenPnL.Name = "Cumulative maximum open PnL";
        CurrentOpenPnL.Name = "Current open PnL";

        posAveragePrice.Name = "Pos Avg Price";
        posAveragePrice.DrawStyle = DRAWSTYLE_IGNORE;

        posLowPrice.Name = "Pos Low Price";
        posLowPrice.DrawStyle = DRAWSTYLE_IGNORE;

        posHighPrice.Name = "Pos High Price";
        posHighPrice.DrawStyle = DRAWSTYLE_IGNORE;

        lastTradeIndex.Name = "Last trade index";
        lastTradeIndex.DrawStyle = DRAWSTYLE_LINE;

        lastXOverIndex.Name = "Last cross over index";
        lastXOverIndex.DrawStyle = DRAWSTYLE_LINE;

        // sc.MaximumPositionAllowed = 1;

    }
    const int i = sc.Index;

    // Common study specs
    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = 1;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.TimeInForce = SCT_TIF_DAY;

    // Retrieving ID
    int64_t &InternalOrderID = sc.GetPersistentInt64(1);
    int &LastCrossOverSellIndex = sc.GetPersistentInt(2);
    int &LastSellTradeIndex = sc.GetPersistentInt(3);
    double &FillPrice = sc.GetPersistentDouble(1);

    if (sc.IsFullRecalculation) {
        LastSellTradeIndex = 0;
    }

    // Trading allowed bool
    bool TradingAllowed = AllowTradingAlways.GetInt() == 1 ? true : tradingAllowedCash(sc);

    // Retrieving Studies
    SCFloatArray PriceEMWA;
    SCFloatArray MACD;
    SCFloatArray MACDMA;
    SCFloatArray MACDDiff;
    SCFloatArray ATR;

    sc.GetStudyArrayUsingID(PriceEMWAStudy.GetStudyID(), 0, PriceEMWA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 0, MACD);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 1, MACDMA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 2, MACDDiff);
    sc.GetStudyArrayUsingID(ATRStudy.GetStudyID(), 0, ATR);

    const int Xover = sc.CrossOver(MACD, MACDMA, i);
    if (Xover==CROSS_FROM_TOP) {
        LastCrossOverSellIndex = i;
    }

    const bool EWACond = UseEWAThresh.GetYesNo() == 1
    ? sc.Close[i] < PriceEMWA[i] && sc.Close[LastCrossOverSellIndex] < PriceEMWA[LastCrossOverSellIndex]
    : true;

    const bool sellCondition = EWACond
            && TradingAllowed
            && LastSellTradeIndex < LastCrossOverSellIndex
            && MACD[i] <= 0
            && MACDDiff[i] <= MaxMACDDiff.GetFloat() // && MACDDiff[i-1] <= MaxMACDDiff.GetFloat() // To avoid entry periods of 1 bar only...
            && i - LastCrossOverSellIndex <= MaxTicksEntryFromCrossOVer.GetInt();

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    if (sellCondition && PositionData.PositionQuantity == 0) {
        int orderSubmitted = 0;
        NewOrder.Target1Offset = 2 * ATR[i];
        NewOrder.Stop1Offset = 2 * ATR[i];
        // NewOrder.Stop1Price = sc.High[LastCrossOverSellIndex] + sc.TickSize * 3;

        orderSubmitted = static_cast<int>(sc.SellOrder(NewOrder));
        LastSellTradeIndex = LastCrossOverSellIndex;
        if (orderSubmitted > 0) {
            FillPrice = NewOrder.Price1;
            InternalOrderID = NewOrder.InternalOrderID;
            // TradeId[i] = static_cast<float>(InternalOrderID);
            SCString Buffer;
            Buffer.Format("ADDED ORDER WITH ID %d", InternalOrderID);
            sc.AddMessageToLog(Buffer, 1);
        }
    }

    int& MaxPnLForTradeInTicks = sc.GetPersistentInt(1);
    if (int CurrentPnLTicks; PositionData.PositionQuantity != 0) {
        CurrentPnLTicks = static_cast<int>(PositionData.OpenProfitLoss  / sc.CurrencyValuePerTick);
        CurrentOpenPnL[i] = static_cast<float>(CurrentPnLTicks);
        if (CurrentPnLTicks > MaxPnLForTradeInTicks) {
            MaxPnLForTradeInTicks = CurrentPnLTicks;
            // const double NewStop = PositionData.PriceLowDuringPosition + (GiveBackTicks.GetFloat() * sc.TickSize);
            //SCString Buffer;
            //Buffer.Format("Need to change the stop to %f", static_cast<float>(NewStop));
            //sc.AddMessageToLog(Buffer, 1);
            //ModifyAttachedStop(InternalOrderID, NewStop, sc);
            //LogAttachedStop(sc.GetPersistentInt64(1), sc);

            // if (s_SCTradeOrder ParentOrder, StopOrder; sc.GetOrderByOrderID(sc.GetPersistentInt64(1), ParentOrder) == 1) {
            //     const int stopOrderSuccess = sc.GetOrderByOrderID(ParentOrder.StopChildInternalOrderID, StopOrder);
            //     if (stopOrderSuccess == 1) {
            //         s_SCNewOrder ModifyOrder;
            //         ModifyOrder.InternalOrderID = StopOrder.InternalOrderID;
            //         ModifyOrder.Price1 = ParentOrder.Price1 - (GiveBackTicks.GetInt() - MaxPnLForTradeInTicks) * sc.TickSize;
//
            //         SCString Buffer;
            //         Buffer.Format("Modified stop limit order price (%d) to %d", StopOrder.InternalOrderID, ModifyOrder.Price1);
            //         sc.AddMessageToLog(Buffer, 1);
//p
            //         sc.ModifyOrder(ModifyOrder);
            //     }
            // }
        }
    } else {
        MaxPnLForTradeInTicks = 0;
        CurrentOpenPnL[i] = 0;
        CumMaxOpenPnL[i] = 0;
        InternalOrderID = 0;
        FillPrice = 0;
    }
    posAveragePrice[i] = PositionData.AveragePrice;
    posLowPrice[i] = PositionData.PriceLowDuringPosition;
    posAveragePrice[i] = PositionData.PriceHighDuringPosition;
    lastTradeIndex[i] = LastSellTradeIndex;
    lastXOverIndex[i] = LastCrossOverSellIndex;

    CumMaxOpenPnL[i] = MaxPnLForTradeInTicks;
    TradeId[i] = static_cast<float>(InternalOrderID);

    flattenAllAfterCash(sc);
}

SCSFExport scsf_StrategyMACDShortFromManager(SCStudyInterfaceRef sc) {
    /*
     Shorting Red MACD when:
    - The last xover is RED AND MACD goes negative
    - OR there is green xover and red xover in next bar
    - Trading either up to N Target ticks OR to green MACD
    - Can choose whether to only short when the price is below the EWA in the chart
    - Make sure the MACD is negative enough for the strategy. Having a MACD bouncing around 0 is a recipe for failure
    - The number of ticks that we take should be a function of the MACD value: if VERY negative then we can aim for higher ticks (long)
    - If very positive: we can aim for more ticks (Short)
    - Exit rules: make sure that if at some point in the trade the max P&L has down-ticked by 10, get out to at least get some snacks
    */
    SCInputRef PriceEMWAStudy = sc.Input[0];
    SCInputRef MACDXStudy = sc.Input[1];
    SCInputRef ATRStudy = sc.Input[2];

    // SCInputRef ADXStudy = sc.Input[1];

    // SCInputRef RangeTrendADXThresh = sc.Input[3];
    SCInputRef MaxMACDDiff = sc.Input[3];
    SCInputRef PriceEMWAMinOffset = sc.Input[4];
    SCInputRef UseRangeOnly = sc.Input[5];
    SCInputRef OneTradePerPeriod = sc.Input[6]; // One the trade ended in the "red period", don't perform any other trade in this period
    SCInputRef UseEWAThresh = sc.Input[7];
    SCInputRef GiveBackTicks = sc.Input[8];
    SCInputRef MaxTicksEntryFromCrossOVer = sc.Input[9];
    SCInputRef AllowTradingAlways = sc.Input[10];

    SCSubgraphRef TradeId = sc.Subgraph[0];
    SCSubgraphRef CumMaxOpenPnL = sc.Subgraph[1];
    SCSubgraphRef CurrentOpenPnL = sc.Subgraph[2];
    SCSubgraphRef lastTradeIndex = sc.Subgraph[3];
    SCSubgraphRef lastXOverIndex = sc.Subgraph[4];
    SCSubgraphRef tradeFilledPrice = sc.Subgraph[5];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;
        sc.GraphName = "Trading MACD Short Exec - MANAGER";

        // The Study
        PriceEMWAStudy.Name = "PriceEMWA";
        PriceEMWAStudy.SetStudyID(6);

        //ADXStudy.Name = "ADX";
        //ADXStudy.SetStudyID(0);

        MACDXStudy.Name = "MACD CrossOver";
        MACDXStudy.SetStudyID(7);

        ATRStudy.Name = "ATR";
        ATRStudy.SetStudyID(2);

        MaxMACDDiff.Name = "Max MACDDiff";
        MaxMACDDiff.SetFloatLimits(-1., 0.0);
        MaxMACDDiff.SetFloat(-0.2);

        PriceEMWAMinOffset.Name = "Price EMWA Tick Offset";
        PriceEMWAMinOffset.SetIntLimits(0, 20);
        PriceEMWAMinOffset.SetInt(5);

        MaxTicksEntryFromCrossOVer.Name = "Maximum number of ticks entry after cross over";
        MaxTicksEntryFromCrossOVer.SetIntLimits(1, 20);
        MaxTicksEntryFromCrossOVer.SetInt(30);


        UseRangeOnly.Name = "Trade in range only";
        UseRangeOnly.SetYesNo(0);

        UseEWAThresh.Name = "Trade above/below EWA only";
        UseEWAThresh.SetYesNo(0);

        OneTradePerPeriod.Name = "Trade above/below EWA only";
        OneTradePerPeriod.SetYesNo(0);

        GiveBackTicks.Name = "Give back in ticks";
        GiveBackTicks.SetIntLimits(0, 20);
        GiveBackTicks.SetInt(10);

        AllowTradingAlways.Name = "Allow trading always";
        AllowTradingAlways.SetYesNo(0);

        TradeId.Name = "Trade ID";
        TradeId.DrawStyle = DRAWSTYLE_IGNORE;


        CumMaxOpenPnL.Name = "Cumulative maximum open PnL";
        CumMaxOpenPnL.DrawStyle = DRAWSTYLE_IGNORE;

        CurrentOpenPnL.Name = "Current open PnL";
        CurrentOpenPnL.DrawStyle = DRAWSTYLE_IGNORE;

        lastTradeIndex.Name = "Last trade index";
        lastTradeIndex.DrawStyle = DRAWSTYLE_IGNORE;

        lastXOverIndex.Name = "Last cross over index";
        lastXOverIndex.DrawStyle = DRAWSTYLE_IGNORE;

        tradeFilledPrice.Name = "Trade filled price";
        tradeFilledPrice.DrawStyle = DRAWSTYLE_LINE;

    }
    const int i = sc.Index;

    // Get the trade wrapper from persistent storage
    auto* trade = static_cast<TradeWrapper*>(sc.GetPersistentPointer(1));

    if (sc.IsFullRecalculation) {
        // Clean up existing trade if any
        if (trade != nullptr) {
            delete trade;
            trade = nullptr;
        }
        sc.SetPersistentPointer(1, nullptr);
    }

    // Common study specs
    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = 1;
    NewOrder.OrderType = SCT_ORDERTYPE_MARKET;
    NewOrder.TimeInForce = SCT_TIF_DAY;

    // Retrieving ID
    int64_t &InternalOrderID = sc.GetPersistentInt64(1);
    int &LastCrossOverSellIndex = sc.GetPersistentInt(2);
    int &LastSellTradeIndex = sc.GetPersistentInt(3);
    double &FillPrice = sc.GetPersistentDouble(1);

    if (sc.IsFullRecalculation) {
        LastSellTradeIndex = 0;
    }

    // Trading allowed bool
    bool TradingAllowed = AllowTradingAlways.GetInt() == 1 ? true : tradingAllowedCash(sc);

    // Retrieving Studies
    SCFloatArray PriceEMWA;
    SCFloatArray MACD;
    SCFloatArray MACDMA;
    SCFloatArray MACDDiff;
    SCFloatArray ATR;

    sc.GetStudyArrayUsingID(PriceEMWAStudy.GetStudyID(), 0, PriceEMWA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 0, MACD);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 1, MACDMA);
    sc.GetStudyArrayUsingID(MACDXStudy.GetStudyID(), 2, MACDDiff);
    sc.GetStudyArrayUsingID(ATRStudy.GetStudyID(), 0, ATR);

    const int Xover = sc.CrossOver(MACD, MACDMA, i);
    if (Xover == CROSS_FROM_TOP) {
        LastCrossOverSellIndex = i;
    }

    const bool EWACond = UseEWAThresh.GetYesNo() == 1
        ? sc.Close[i] < PriceEMWA[i] && sc.Close[LastCrossOverSellIndex] < PriceEMWA[LastCrossOverSellIndex]
        : true;

    const bool sellCondition = EWACond
        && TradingAllowed
        && LastSellTradeIndex < LastCrossOverSellIndex
        && MACD[i] <= 0
        && MACDDiff[i] <= MaxMACDDiff.GetFloat()
        && i - LastCrossOverSellIndex <= MaxTicksEntryFromCrossOVer.GetInt();

    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);

    // Update existing trade
    if (trade != nullptr) {
        try {
            switch (trade->getRealStatus(i)) {
                case TradeStatus::Terminated:
                    delete trade;
                    trade = nullptr;
                    sc.SetPersistentPointer(1, nullptr);
                    break;
                default:
                    trade->updateAll(sc, i);
                    if (const int success = trade->modifyStopTargetOrders(sc, i); success == 2) {
                        SCString Buffer;
                        sc.AddMessageToLog(Buffer.Format("Successfully changed the order"), 1);
                    }

                    tradeFilledPrice[i] = static_cast<float>(trade->getMaxFavorablePriceDifference());
                    break;
            }
        } catch (const std::exception& e) {
            SCString Buffer;
            Buffer.Format("ERROR in trade update: %s", e.what());
            sc.AddMessageToLog(Buffer, 1);

            // Clean up on error
            delete trade;
            trade = nullptr;
            sc.SetPersistentPointer(1, nullptr);
        }
    }

    if (sellCondition && trade == nullptr) {
        int orderSubmitted = 0;
        NewOrder.Target1Offset = 3 * sc.TickSize;
        NewOrder.Stop1Offset = 3 * sc.TickSize;

        orderSubmitted = static_cast<int>(sc.SellOrder(NewOrder));

        if (orderSubmitted > 0) {
            LastSellTradeIndex = LastCrossOverSellIndex;
            FillPrice = NewOrder.Price1;
            InternalOrderID = NewOrder.InternalOrderID;

            // Create new trade wrapper with proper error handling
            try {
                trade = new TradeWrapper(InternalOrderID, i, TargetMode::Evolving,  BSE_SELL, 2*sc.TickSize);
                sc.SetPersistentPointer(1, trade);
                TradeId[i] = static_cast<float>(InternalOrderID);

                SCString Buffer;
                Buffer.Format("ADDED ORDER WITH ID %d (Evolving mode)", InternalOrderID);
                sc.AddMessageToLog(Buffer, 1);
            } catch (const std::exception& e) {
                SCString Buffer;
                Buffer.Format("ERROR: Failed to create TradeWrapper: %s", e.what());
                sc.AddMessageToLog(Buffer, 1);
                trade = nullptr;
            }
        }
    }


    lastTradeIndex[i] = static_cast<float>(LastSellTradeIndex);
    lastXOverIndex[i] = static_cast<float>(LastCrossOverSellIndex);
    TradeId[i] = static_cast<float>(InternalOrderID);
}

