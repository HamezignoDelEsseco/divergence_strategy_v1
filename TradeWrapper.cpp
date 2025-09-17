#include "TradeWrapper.h"


TradeWrapper::TradeWrapper(
    const int64_t parentId,
    const int createdIndex,
    const TargetMode mode,
    const BuySellEnum dir,
    const double constPlateauSize,
    const int expirationBars
)
    : parentOrderId(parentId),
      createdIndex(createdIndex),
      expirationBars(expirationBars),
      parentOrderDirection(dir),
      targetMode(mode),
      fillPrice(0),
      maxFavorablePriceDifference(0.0),
      targetPrice(0.0),
      stopPrice(0.0),
      constPlateauSize(constPlateauSize),
      currentPlateau(0) {}

[[nodiscard]] TradeStatus TradeWrapper::getRealStatus(const int index) const {
    const bool priceCondition = parentOrder.Price1 != 0 && stopOrder.Price1 != 0 && targetOrder.Price1 != 0;
    const bool activeCondition = getStopOrderStatus() == SCT_OSC_OPEN && getTargetOrderStatus() == SCT_OSC_OPEN && priceCondition;
    const bool terminatedCondition = (getStopOrderStatus() == SCT_OSC_CANCELED || getTargetOrderStatus() == SCT_OSC_CANCELED) && priceCondition;
    if (activeCondition) {
        return TradeStatus::Active;
    } if (terminatedCondition) {
        return TradeStatus::Terminated;
    }
    return TradeStatus::Other;
}


void TradeWrapper::updateAll(SCStudyInterfaceRef sc, const int i, const double price) {
    fetchAndUpdateOrders(sc);
    if (getRealStatus(i) != TradeStatus::Active) {return;}
    
    // Initialize fill price once we have valid order data
    fillPrice = parentOrder.Price1;
    targetPrice = targetOrder.Price1;
    stopPrice = stopOrder.Price1;

    // Calculate price difference from fill price
    double currentPriceDifference = 0.0;
    switch (parentOrderDirection) {
        case BSE_BUY:
            currentPriceDifference = price - fillPrice;
            maxFavorablePriceDifference = std::max<double>(maxFavorablePriceDifference, currentPriceDifference);
            break;
        case BSE_SELL:
            currentPriceDifference = fillPrice - price;
            maxFavorablePriceDifference = std::max<double>(maxFavorablePriceDifference, currentPriceDifference);
            break;
        case BSE_UNDEFINED:
            break;
    }
    
    // Check for new plateau and update stops/targets accordingly
    updatePlateau();
}

void TradeWrapper::updateStopTargetPrice() {
    switch (parentOrderDirection) {
        case BSE_BUY:
            stopPrice = stopPrice + currentPlateau * constPlateauSize;
            targetPrice = targetPrice + currentPlateau * constPlateauSize;
            break;

        case BSE_SELL:
            stopPrice = stopPrice - currentPlateau * constPlateauSize;
            targetPrice = targetPrice - currentPlateau * constPlateauSize;
            break;

        case BSE_UNDEFINED:
            break;
    }
}


void TradeWrapper::updatePlateau() {
    if (const int newPlateau = static_cast<int>(maxFavorablePriceDifference / constPlateauSize); newPlateau > currentPlateau) {
        currentPlateau = newPlateau;
        updateStopTargetPrice();
    }
}

int TradeWrapper::flattenOrder(SCStudyInterfaceRef sc, const double price) const {
    if (targetMode == TargetMode::Flat) {
        bool flattenPosition = false;
        switch (getParentOrderDirection()) {
            case BSE_BUY:
                flattenPosition = price >= targetPrice; break;
            case BSE_SELL:
                flattenPosition = price <= targetPrice; break;
            case BSE_UNDEFINED:
                break;

        }
        if (flattenPosition && targetPrice != 0.0) { // Make sure it's not cancelled right after object creation
            return sc.CancelOrder(parentOrderId);
        }
    }
    return -1;
}

int TradeWrapper::modifyStopTargetOrders(SCStudyInterfaceRef sc, const int i) const {
    int success = 0;
    if (getRealStatus(i) == TradeStatus::Active) {
        s_SCNewOrder modifyStopOrder;
        s_SCNewOrder modifyTargetOrder;

        modifyTargetOrder.InternalOrderID = targetOrder.InternalOrderID;
        modifyTargetOrder.Price1 = targetPrice;
        success += sc.ModifyOrder(modifyTargetOrder);

        modifyStopOrder.InternalOrderID = stopOrder.InternalOrderID;
        modifyStopOrder.Price1 = stopPrice;
        success += sc.ModifyOrder(modifyStopOrder);
    }
    return success;
}

int TradeWrapper::fetchAndUpdateOrders(SCStudyInterfaceRef sc) {
    const int successParent = sc.GetOrderByOrderID(parentOrderId, parentOrder);
    const int successStop = sc.GetOrderByOrderID(parentOrder.StopChildInternalOrderID, stopOrder);
    const int successTarget = sc.GetOrderByOrderID(parentOrder.TargetChildInternalOrderID, targetOrder);
    return successParent + successStop + successTarget;
}

[[nodiscard]] double TradeWrapper::getFilledPrice() const {return fillPrice;}

[[nodiscard]] double TradeWrapper::getMaxFavorablePriceDifference() const {return maxFavorablePriceDifference;}

[[nodiscard]] BuySellEnum TradeWrapper::getParentOrderDirection() const {return parentOrder.BuySell;}


[[nodiscard]] SCOrderStatusCodeEnum TradeWrapper::getTargetOrderStatus() const {
    return targetOrder.OrderStatusCode;
}

[[nodiscard]] SCOrderStatusCodeEnum TradeWrapper::getStopOrderStatus() const {
    return stopOrder.OrderStatusCode;
}



