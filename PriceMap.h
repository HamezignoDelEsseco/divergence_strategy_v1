#ifndef TRADE_DIVERGENCE_MODE_1_PRICEMAP_H
#define TRADE_DIVERGENCE_MODE_1_PRICEMAP_H

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
    stats.NStaystick +=st;
}

#endif //TRADE_DIVERGENCE_MODE_1_PRICEMAP_H