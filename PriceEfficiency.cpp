#include "sierrachart.h"


struct PriceStats
{
    float TotalAskVol;   // raw accumulated Ask volume at this price
    float TotalBidVol;   // raw accumulated Bid volume at this price
    float EWMAAsk;       // EWMA of Ask volume
    float EWMABid;       // EWMA of Bid volume
};

typedef std::map<int, PriceStats> PriceMap;


// Compute alpha from half-life (in updates)
inline float AlphaFromHalfLife(float halfLife)
{
    return 1.0f - powf(0.5f, 1.0f / halfLife);
}

// Generic EWMA update
inline float UpdateEWMA(float oldVal, float newObs, float alpha)
{
    return (1.0f - alpha) * oldVal + alpha * newObs;
}

inline PriceStats& GetPriceStats(PriceMap& priceMap, int price)
{
    return priceMap[price]; // default-constructed if missing
}

// Update stats with new trade
inline void UpdatePriceStats(PriceStats& stats, float askVol, float bidVol, float alpha)
{
    // Update totals
    stats.TotalAskVol += askVol;
    stats.TotalBidVol += bidVol;

    // Update EWMAs
    stats.EWMAAsk = UpdateEWMA(stats.EWMAAsk, askVol, alpha);
    stats.EWMABid = UpdateEWMA(stats.EWMABid, bidVol, alpha);
}

SCSFExport scsf_PerPriceAskBidEMWA(SCStudyInterfaceRef sc)
{
    // ---- Inputs ----
    SCInputRef Input_HalfLife = sc.Input[0];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Per-Price Ask/Bid with EWMA (T&S)";
        sc.AutoLoop = 0; // manual
        Input_HalfLife.Name = "EWMA Half-Life (trades)";
        Input_HalfLife.SetInt(20);
        return;
    }

    // ---- Persistent map ----
    PriceMap* priceMap = reinterpret_cast<PriceMap*>(sc.GetPersistentPointer(1));
    if (priceMap == nullptr)
    {
        priceMap = new PriceMap();
        sc.SetPersistentPointer(1, priceMap);
    }

    // ---- Compute alpha ----
    float alpha = AlphaFromHalfLife(static_cast<float>(Input_HalfLife.GetInt()));

    int64_t& LastProcessedSequence = sc.GetPersistentInt64(1);

    //reset the sequence number on a full recalculation so we start fresh for each full recalculation.
    if (sc.IsFullRecalculation && sc.UpdateStartIndex == 0)
        LastProcessedSequence = 0;


    // ---- Get Time & Sales ----
    c_SCTimeAndSalesArray TnS;
    sc.GetTimeAndSales(TnS);

    // Track last sequence number so we don’t reprocess old trades
    int& lastSeq = sc.GetPersistentInt(2);

    for (int i = 0; i < TnS.Size(); i++)
    {
        const s_TimeAndSales& ts = TnS[i];

        // Skip already processed
        if (ts.Sequence <= lastSeq)
            continue;

        // Work in integer ticks for map key
        int priceTicks = static_cast<int>(sc.PriceValueToTicks(ts.Price));

        // Ask vs Bid trade
        float askVol = 0.0f, bidVol = 0.0f;
        if (ts.Type == SC_TS_ASK)
            askVol = static_cast<float>(ts.Volume);
        else if (ts.Type == SC_TS_BID)
            bidVol = static_cast<float>(ts.Volume);
        else
            continue;

        // Update stats
        PriceStats& stats = GetPriceStats(*priceMap, priceTicks);

        stats.TotalAskVol += askVol;
        stats.TotalBidVol += bidVol;
        stats.EWMAAsk = UpdateEWMA(stats.EWMAAsk, askVol, alpha);
        stats.EWMABid = UpdateEWMA(stats.EWMABid, bidVol, alpha);

        // Example: log one stat
        SCString msg;
        msg.Format("Price=%d  Ask=%.1f Bid=%.1f EWMAAsk=%.2f EWMABid=%.2f",
                   priceTicks, stats.TotalAskVol, stats.TotalBidVol,
                   stats.EWMAAsk, stats.EWMABid);
        sc.AddMessageToLog(msg, 0);

        // Update last sequence
        if (ts.Sequence > lastSeq)
            lastSeq = ts.Sequence;
    }
}


SCSFExport scsf_TimeAndSalesIterationExample(SCStudyInterfaceRef sc)
{
    if (sc.SetDefaults)
    {
        // Set the configuration and defaults

        sc.GraphName = "Time and Sales Iteration Example";

        sc.StudyDescription = "This is an example of iterating through the time and sales records which have been added since the last study function call.";

        sc.AutoLoop = 0;//Use manual looping



        sc.GraphRegion = 0;

        return;
    }


    int64_t& LastProcessedSequence = sc.GetPersistentInt64(1);

    //reset the sequence number on a full recalculation so we start fresh for each full recalculation.
    if (sc.IsFullRecalculation && sc.UpdateStartIndex == 0)
        LastProcessedSequence = 0;

    // Get the Time and Sales
    c_SCTimeAndSalesArray TimeSales;
    sc.GetTimeAndSales(TimeSales);
    if (TimeSales.Size() == 0)
        return;  // No Time and Sales data available for the symbol

    //Set the initial sequence number
    if (LastProcessedSequence == 0)
        LastProcessedSequence = TimeSales[0].Sequence;

    PriceMap* priceMap = reinterpret_cast<PriceMap*>(sc.GetPersistentPointer(1));
    if (priceMap == nullptr)
    {
        priceMap = new PriceMap();
        sc.SetPersistentPointer(1, priceMap);
    }

    // Loop through the Time and Sales.
    for (int TSIndex = 0; TSIndex < TimeSales.Size(); ++TSIndex)
    {
        //do not reprocess previously processed sequence numbers.
        if (TimeSales[TSIndex].Sequence <= LastProcessedSequence)
            continue;

        LastProcessedSequence = TimeSales[TSIndex].Sequence;

        //only interested in trade records
        if (TimeSales[TSIndex].Type == SC_TS_BID || TimeSales[TSIndex].Type == SC_TS_ASK)
        {

            int priceTicks = static_cast<int>(sc.PriceValueToTicks(TimeSales[TSIndex].Price));

            // Ask vs Bid trade
            float askVol = 0.0f, bidVol = 0.0f;
            if (TimeSales[TSIndex].Type == SC_TS_ASK)
                askVol = static_cast<float>(TimeSales[TSIndex].Volume);
            else if (TimeSales[TSIndex].Type == SC_TS_BID)
                bidVol = static_cast<float>(TimeSales[TSIndex].Volume);
            else
                continue;

            // Update stats
            PriceStats& stats = GetPriceStats(*priceMap, priceTicks);

            stats.TotalAskVol += askVol;
            stats.TotalBidVol += bidVol;

        }
    }

}

SCSFExport scsf_CupCdown(SCStudyInterfaceRef sc)
{
    SCSubgraphRef Cup    = sc.Subgraph[0];
    SCSubgraphRef Cdown  = sc.Subgraph[1];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Cup Cdown Example";
        sc.StudyDescription = "Toy study computing Cup and Cdown using 1-trade bars";

        sc.AutoLoop = 1;  // iterate over each bar automatically
        sc.GraphRegion = 1;

        Cup.Name = "Cup";
        Cup.DrawStyle = DRAWSTYLE_BAR;
        Cup.PrimaryColor = RGB(0, 200, 0);

        Cdown.Name = "Cdown";
        Cdown.DrawStyle = DRAWSTYLE_BAR;
        Cdown.PrimaryColor = RGB(200, 0, 0);

        return;
    }

    // Persistent state
    int& lastPrice = sc.GetPersistentInt(1);
    float& volAccumulator = sc.GetPersistentFloat(1);

    // Current trade data
    float price = sc.Close[sc.Index];      // on a 1-trade chart, Close == trade price
    float volume = sc.Volume[sc.Index];    // trade size

    if (sc.Index == 0)
    {
        lastPrice = (int)(price * sc.RealTimePriceMultiplier);
        volAccumulator = 0.0f;
        Cup[sc.Index] = 0;
        Cdown[sc.Index] = 0;
        return;
    }

    int currentPrice = (int)(price * sc.RealTimePriceMultiplier);
    int prevPrice    = lastPrice;

    if (currentPrice == prevPrice)
    {
        // Same price → keep accumulating volume
        volAccumulator += volume;
        Cup[sc.Index] = 0;
        Cdown[sc.Index] = 0;
    }
    else if (currentPrice > prevPrice)
    {
        // Uptick event completed
        volAccumulator += volume;
        Cup[sc.Index] = volAccumulator;
        Cdown[sc.Index] = 0;

        // Reset for next sequence
        volAccumulator = 0;
        lastPrice = currentPrice;
    }
    else if (currentPrice < prevPrice)
    {
        // Downtick event completed
        volAccumulator += volume;
        Cdown[sc.Index] = volAccumulator;
        Cup[sc.Index] = 0;

        // Reset for next sequence
        volAccumulator = 0;
        lastPrice = currentPrice;
    }
}