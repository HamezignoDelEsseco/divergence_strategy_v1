#include "sierrachart.h"


struct PriceStats
{
    float TotalVol = 0;
    float TotalAskVol = 0;   // raw accumulated Ask volume at this price
    float TotalBidVol = 0;   // raw accumulated Bid volume at this price
    float NTrades = 0;
    int NUpticks = 0;
    int NDownticks = 0;
    int NStaystick = 0;
};

typedef std::map<int64_t, PriceStats> PriceMap;


inline PriceStats& GetPriceStats(PriceMap& priceMap, const int64_t price)
{
    return priceMap[price]; // default-constructed if missing
}

inline void UpdatePriceStats(
    PriceStats& stats,
    const float Vol,
    const float askVol,
    const float bidVol,
    const float nTrades,
    const int dt,
    const int ut,
    const int st
    )
{
    stats.TotalVol += Vol;
    stats.TotalAskVol += askVol;
    stats.TotalBidVol += bidVol;
    stats.NTrades += nTrades;
    stats.NUpticks += ut;
    stats.NDownticks += dt;
    stats.NStaystick +=st ;
}


SCSFExport scsf_UpDownTickCost(SCStudyInterfaceRef sc) {
    SCInputRef NTradesWindowFast = sc.Input[0];
    SCInputRef NTradesWindowSlow = sc.Input[1];
    SCInputRef VolumeByPriceStudy = sc.Input[2];

    SCSubgraphRef UPTCostSlow = sc.Subgraph[0];
    SCSubgraphRef UPTCostFast = sc.Subgraph[1];
    SCSubgraphRef TotalVolume = sc.Subgraph[2];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "UDT cost by price";

        NTradesWindowSlow.Name = "N trades in window fast";
        NTradesWindowSlow.SetIntLimits(1, 10000);
        NTradesWindowSlow.SetInt(200);

        NTradesWindowFast.Name = "N trades in window fast";
        NTradesWindowFast.SetIntLimits(NTradesWindowSlow.GetInt() + 1,10000);
        NTradesWindowFast.SetInt(400);


        VolumeByPriceStudy.Name = "Volume by price study";
        VolumeByPriceStudy.SetStudyID(0);

        UPTCostSlow.Name = "UDT cost slow";
        UPTCostFast.Name = "UDT cost fast";
        TotalVolume.Name = "Total volume";

    }

    const int i = sc.Index;

    auto priceMap = reinterpret_cast<PriceMap*>(sc.GetPersistentPointer(1));

    if (sc.IsFullRecalculation && i == 0) {
        SCString msg; msg.Format("Cleared array of size %f", priceMap->size());
        sc.AddMessageToLog(msg, 0);
        priceMap->clear();
    }

    if (priceMap == nullptr || sc.IsNewTradingDay(i)) {
        delete priceMap;
        priceMap = new PriceMap();
        sc.SetPersistentPointer(1, priceMap);
        SCString msg; msg.Format("Created PriceMap at %p", priceMap);
        sc.AddMessageToLog(msg, 0);
    }

    if (sc.IsNewTradingDay(i)) {
        TotalVolume[i] = 0;
        TotalVolume[i-1] = 0;
    }

    PriceStats& stats = GetPriceStats(*priceMap, sc.PriceValueToTicks(sc.Close[i]));
    const int uptick = i == 0 ? 0: sc.Close[i] > sc.Close[i - 1] ? 1:0;
    const int downtick = i == 0 ? 0 : sc.Close[i] < sc.Close[i - 1]? 1:0;
    const int staystick = i == 0 ? 0: sc.Close[i] == sc.Close[i - 1]? 1:0;

     // const int s = priceMap->size(); // Debugging

    UpdatePriceStats(stats, sc.Volume[i], sc.AskVolume[i], sc.BidVolume[i], sc.NumberOfTrades[i], uptick, downtick, staystick);

    TotalVolume[i] = TotalVolume[i-1] + sc.Volume[i];
}
