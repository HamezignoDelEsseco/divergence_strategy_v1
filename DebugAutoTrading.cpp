#include "sierrachart.h"
#include "helpers.h"

SCSFExport scsf_OrderStatusCodeDebug(SCStudyInterfaceRef sc) {
    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "AT - Order Status Debug";

        // Allows to compute the daily P&L
        sc.MaintainTradeStatisticsAndTradesData = true;
    }

    s_SCTradeOrder tradeOrder;

    int64_t& internalOrderID = sc.GetPersistentInt64(1);
    // int64_t& stopInternalID = sc.GetPersistentInt64(2);
    // int64_t& targetInternalID = sc.GetPersistentInt64(2);


    double& limitOrderPrice = sc.GetPersistentDouble(1);
    const int retrieved_order = sc.GetOrderByOrderID(internalOrderID, tradeOrder);
    const SCOrderStatusCodeEnum orderStatus = tradeOrder.OrderStatusCode;

    const bool isWorking = IsWorkingOrderStatus(orderStatus);
    const bool isFilled = orderStatus == SCT_OSC_FILLED;
    const bool isCancelled = orderStatus == SCT_OSC_CANCELED;


    const int i = sc.Index;

    if (i==0) {internalOrderID = 0;}

    if (limitOrderPrice == 0.0 || sc.Close[i] < limitOrderPrice) {
        limitOrderPrice = sc.Close[i] - 5;
    }

    if (retrieved_order == 1) {
        // Even if no new orders are created, this will end up not being triggered
        // This is because in SC the non-working orders are cleared at some points and cannot be accessed anymore using the
        // GetOrderByOrderID function !!
        const bool foundYou = true;
    }

    if (retrieved_order != 1) { // The order was found
        s_SCNewOrder newOrder;
        newOrder.OrderQuantity = 1;
        newOrder.OrderType = SCT_ORDERTYPE_LIMIT;
        newOrder.TimeInForce = SCT_TIF_DAY;

        newOrder.Price1 = limitOrderPrice;
        newOrder.Target1Offset = 10 * sc.TickSize;
        newOrder.Stop1Offset = 5 * sc.TickSize;

        const double orderSubmitted = sc.SellOrder(newOrder);
        if (orderSubmitted > 0) {
            internalOrderID = newOrder.InternalOrderID;
        }
    }

}

SCSFExport scsf_OrderStatusCodeDebugTwoQuantity(SCStudyInterfaceRef sc) {
    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "AT - Order Status Debug 2 qty";

        // Allows to compute the daily P&L
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.MaximumPositionAllowed = 2;
        sc.AllowOnlyOneTradePerBar = true;
    }

    s_SCTradeOrder tradeOrder;

    int64_t& internalOrderID = sc.GetPersistentInt64(1);
    // int64_t& stopInternalID = sc.GetPersistentInt64(2);
    // int64_t& targetInternalID = sc.GetPersistentInt64(2);


    double& limitOrderPrice = sc.GetPersistentDouble(1);
    const int retrieved_order = sc.GetOrderByOrderID(internalOrderID, tradeOrder);

    const int i = sc.Index;

    if (i==0) {internalOrderID = 0;}

    if (limitOrderPrice == 0.0 || sc.Close[i] < limitOrderPrice) {
        limitOrderPrice = sc.Close[i] - 5;
    }

    if (retrieved_order == 1) {
        // Even if no new orders are created, this will end up not being triggered
        // This is because in SC the non-working orders are cleared at some points and cannot be accessed anymore using the
        // GetOrderByOrderID function !!
        const bool foundYou = true;
    }

    if (retrieved_order != 1) { // The order was found
        s_SCNewOrder newOrder;
        newOrder.OrderQuantity = 2;
        newOrder.OrderType = SCT_ORDERTYPE_LIMIT;
        newOrder.TimeInForce = SCT_TIF_DAY;

        newOrder.Price1 = limitOrderPrice;
        newOrder.Price2 = limitOrderPrice;

        newOrder.Target1Offset = 10 * sc.TickSize;
        newOrder.Stop1Offset = 5 * sc.TickSize;
        newOrder.Target2Offset = 30 * sc.TickSize;
        newOrder.Stop2Offset = 15 * sc.TickSize;

        newOrder.OCOGroup1Quantity = 1;
        newOrder.OCOGroup2Quantity = 1;

        const double orderSubmitted = sc.SellOrder(newOrder);
        if (orderSubmitted > 0) {
            internalOrderID = newOrder.InternalOrderID;
        }
    }
}


SCSFExport scsf_OCOWrapDebug(SCStudyInterfaceRef sc) {
    SCInputRef Enabled = sc.Input[0];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "AT - OCO Wrap debug";

        // Allows to compute the daily P&L
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.MaximumPositionAllowed = 2;
        sc.AllowOnlyOneTradePerBar = true;

        Enabled.Name = "Enabled";
        Enabled.SetYesNo(0);
    }

    if (Enabled.GetInt() == 0) {
        return ;
    }

    s_SCTradeOrder tradeOrder;

    int64_t& internalOrderID = sc.GetPersistentInt64(1);


    double& limitOrderPrice = sc.GetPersistentDouble(1);
    const int retrieved_order = sc.GetOrderByOrderID(internalOrderID, tradeOrder);
    const SCOrderStatusCodeEnum orderStatus = tradeOrder.OrderStatusCode;

    const bool isWorking = IsWorkingOrderStatus(orderStatus);
    const bool isFilled = orderStatus == SCT_OSC_FILLED;
    const bool isCancelled = orderStatus == SCT_OSC_CANCELED;


    const int i = sc.Index;

    if (i==0) {internalOrderID = 0;}

    if (limitOrderPrice == 0.0 || sc.Close[i] < limitOrderPrice) {
        limitOrderPrice = sc.Close[i] + 5;
    }

    if (retrieved_order == 1) {
        // Even if no new orders are created, this will end up not being triggered
        // This is because in SC the non-working orders are cleared at some points and cannot be accessed anymore using the
        // GetOrderByOrderID function !!
        const bool foundYou = true;
    }

    if (retrieved_order != 1) { // The order was found
        s_SCNewOrder newOrder;
        newOrder.OrderQuantity = 2;
        newOrder.OrderType = SCT_ORDERTYPE_LIMIT;
        newOrder.TimeInForce = SCT_TIF_DAY;

        newOrder.Price1 = limitOrderPrice;
        newOrder.Price2 = limitOrderPrice + 2;

        newOrder.Target1Offset = 10 * sc.TickSize;
        newOrder.Stop1Offset = 5 * sc.TickSize;
        newOrder.Target2Offset = 30 * sc.TickSize;
        newOrder.Stop2Offset = 15 * sc.TickSize;

        newOrder.OCOGroup1Quantity = 1;
        newOrder.OCOGroup2Quantity = 1;

        const double orderSubmitted = sc.SellOrder(newOrder);
        if (orderSubmitted > 0) {
            internalOrderID = newOrder.InternalOrderID;
        }

    }

    s_SCOrderFillData orderFillData;
    int filledByIndex = sc.GetOrderFillEntry(0, orderFillData);
    if (filledByIndex) {
        bool filled = true;
    }

}

SCSFExport scsf_TradingStatus(SCStudyInterfaceRef sc) {
    SCInputRef Enabled = sc.Input[0];

    SCSubgraphRef NumWorkingAttachedOrders = sc.Subgraph[0];
    SCSubgraphRef NumWorkingParentOrders = sc.Subgraph[1];
    SCSubgraphRef NumFilledParentOrders = sc.Subgraph[2];
    SCSubgraphRef NumFilledAttachedOrders = sc.Subgraph[3];
    SCSubgraphRef ActivityFlag = sc.Subgraph[4];
    SCSubgraphRef TotalTradesLoop = sc.Subgraph[5];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "AT - Activity Status";

        // Allows to compute the daily P&L
        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.MaximumPositionAllowed = 2;
        sc.AllowOnlyOneTradePerBar = true;

        Enabled.Name = "Enabled";
        Enabled.SetYesNo(0);

        NumWorkingAttachedOrders.Name = "Number of working attached";
        NumWorkingParentOrders.Name = "Number of working parents";
        NumFilledParentOrders.Name = "Number of filled parents";
        NumFilledAttachedOrders.Name = "Number of filled attached";
        ActivityFlag.Name = "Trading activity status";
        TotalTradesLoop.Name = "Total Trades";

    }

    const int i = sc.Index;

    if (Enabled.GetInt() == 0) {
        return ;
    }
    int totalLoopSize = 0;
    int workingAttachedOrdersCounter = 0;
    int workingParentOrdersCounter = 0;
    int filledParentOrdersCounter = 0;
    int filledAttachedOrdersCounter = 0;


    const int activeStatus = tradeActivityStatus(sc, workingParentOrdersCounter, workingAttachedOrdersCounter, filledParentOrdersCounter, filledAttachedOrdersCounter, totalLoopSize);

    NumWorkingAttachedOrders[i] = workingAttachedOrdersCounter;
    NumWorkingParentOrders[i] = workingParentOrdersCounter;
    NumFilledAttachedOrders[i] = filledAttachedOrdersCounter;
    NumFilledParentOrders[i] = filledParentOrdersCounter;
    TotalTradesLoop[i] = totalLoopSize;

    ActivityFlag[i] = activeStatus;
}


SCSFExport scsf_WorkingOrderId(SCStudyInterfaceRef sc) {
    SCInputRef Enabled = sc.Input[0];;
    SCSubgraphRef ActivityFlag = sc.Subgraph[0];


    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "AT - Working Parent ID";

        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.MaximumPositionAllowed = 2;
        sc.AllowOnlyOneTradePerBar = true;

        Enabled.Name = "Enabled";
        Enabled.SetYesNo(0);

        ActivityFlag.Name = "Trading activity status";

    }

    const int i = sc.Index;

    if (Enabled.GetInt() == 0) {
        return ;
    }
    uint32_t workingOrderId = 0;
    const int activeStatus = workingParentOrder(sc, workingOrderId);
    ActivityFlag[i] = activeStatus;
}

