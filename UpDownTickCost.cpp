#include "sierrachart.h"

#include "PriceMap.h"

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


    if (priceMap == nullptr) { //|| sc.IsNewTradingDay(i)) {
        delete priceMap;
        priceMap = new PriceMap();
        sc.SetPersistentPointer(1, priceMap);
        SCString msg; msg.Format("Created PriceMap at %p", priceMap);
        sc.AddMessageToLog(msg, 0);
    }

    if (sc.IsNewTradingDay(i) || i == 0) {
        priceMap->clear();
        TotalVolume[i] = 0;
        TotalVolume[i-1] = 0;
    } else {
        PriceStats& stats = GetPriceStats(*priceMap, sc.PriceValueToTicks(sc.Close[i-1]));
        const int uptick = i == 1 ? 0: sc.Close[i] > sc.Close[i - 1] ? 1:0;
        const int downtick = i == 1 ? 0 : sc.Close[i] < sc.Close[i - 1]? 1:0;
        const int staystick = i == 1 ? 0: sc.Close[i] == sc.Close[i - 1]? 1:0;
        UpdatePriceStats(stats, sc.Volume[i-1], sc.AskVolume[i-1], sc.BidVolume[i-1], sc.NumberOfTrades[i-1], uptick, downtick, staystick);

    }

    // We look at the upticks and down ticks FROM the latest price, not TO the latest price !! (we always lag one trade)
    //if (i > 0 && !sc.IsNewTradingDay(i)) {
//
    //}

    TotalVolume[i] = TotalVolume[i-1] + sc.Volume[i];
}
