#include "sierrachart.h"

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