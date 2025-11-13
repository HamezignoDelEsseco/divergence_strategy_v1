#include "sierrachart.h"

SCSFExport scsf_ClusterImbalanceTrader(SCStudyInterfaceRef sc) {

    SCInputRef ImbalanceAlert = sc.Input[0];
    SCInputRef ShortLong = sc.Input[1];


    SCSubgraphRef StopLoss = sc.Subgraph[0];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Cluster imbalance trader";

        ImbalanceAlert.Name = "Alert ask";
        ImbalanceAlert.SetStudyID(1);

        ShortLong.Name = "Trader direction (0 for short)";
        ShortLong.SetYesNo(0);

        // Outputs
        StopLoss.Name = "Stop loss";

        sc.GraphRegion = 0;
        sc.ScaleRangeType = SCALE_SAMEASREGION;

        // Flags
        sc.SupportReversals = true;
        sc.AllowOnlyOneTradePerBar = true;

        sc.MaximumPositionAllowed = 1;

        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.AllowMultipleEntriesInSameDirection = true;
        sc.AllowEntryWithWorkingOrders = false;
    }

    const int currIdx = sc.Index;
    float& stopPrice = sc.GetPersistentFloat(1);
    StopLoss[currIdx] = stopPrice;

    // Preload adjacent zones
    std::vector<SCFloatArray> ImbalanceZones(10);
    std::vector<SCFloatArray> AskAdjacentZones(10);

    for (size_t i = 0; i < 10; ++i) {
        sc.GetStudyArrayUsingID(ImbalanceAlert.GetStudyID(), 48 + i, ImbalanceZones[i]);
    }

    SCFloatArrayRef firstBot = ImbalanceZones[0];
    SCFloatArrayRef firstTop = ImbalanceZones[1];

    if (firstBot[currIdx] != 0 && firstTop[currIdx] != 0) {
    if (ShortLong.GetYesNo() == 0) {
        stopPrice = firstBot[currIdx];
    } else {
        stopPrice = firstTop[currIdx];
    }

        if (sc.LastCallToFunction)
            return;

        if (sc.IsFullRecalculation)
            return;

        if (sc.Index < sc.ArraySize-1)
            return;

        s_SCNewOrder newOrder;
        newOrder.OrderQuantity = 1;
        newOrder.TimeInForce = SCT_TIF_DAY;
        newOrder.OrderType = SCT_ORDERTYPE_MARKET;
        newOrder.Stop1Price = stopPrice;
        newOrder.Target1Offset = 5 * sc.TickSize;
        newOrder.AttachedOrderStopAllType = SCT_ORDERTYPE_STOP;
        newOrder.AttachedOrderTarget1Type = SCT_ORDERTYPE_STOP_LIMIT;
        if (ShortLong.GetYesNo() == 0) {
            sc.BuyOrder(newOrder);
        } else {
            sc.SellOrder(newOrder);
        }
    }


}