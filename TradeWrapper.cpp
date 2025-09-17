#include "TradeWrapper.h"


TradeWrapper::TradeWrapper(
    const int64_t parentId,
    const TargetMode mode,
    const double stopATRMult,
    const double targetATRMult,
    const BuySellEnum dir
    )
    : parentOrderId(parentId),
    parentOrderDirection(dir),
    targetMode(mode),
    fillPrice(0),
    maxFavorablePrice(0.0),
    stopAtrMultiplier(stopATRMult),
    targetAtrMultiplier(targetATRMult),
    targetPrice(0.0),
    stopPrice(0.0) {}

[[nodiscard]] TradeStatus TradeWrapper::getRealStatus() const {
    const bool activeCondition = getStopOrderStatus() == SCT_OSC_OPEN && getTargetOrderStatus() == SCT_OSC_OPEN;
    const bool terminatedCondition = getStopOrderStatus() == SCT_OSC_CANCELED || getTargetOrderStatus() == SCT_OSC_CANCELED;
    if (activeCondition) {
        return TradeStatus::Active;
    } if (terminatedCondition) {
        return TradeStatus::Terminated;
    }
    return TradeStatus::Other;
}


void TradeWrapper::updateAttributes(const double price, const double atr) {
    fillPrice = parentOrder.Price1;
    targetPrice = targetOrder.Price1;
    stopPrice = stopOrder.Price1;

    switch (parentOrderDirection) {
        case BSE_BUY:
            maxFavorablePrice = std::max<double>(maxFavorablePrice, price); break;
        case BSE_SELL:
            maxFavorablePrice = std::min<double>(maxFavorablePrice, price); break;
        case BSE_UNDEFINED:
            break;

    }
    updateStopPrice(atr);
    updateTargetPrice(price, atr);
}

void TradeWrapper::updateStopPrice(const double atr) {
    switch (parentOrderDirection) {
        case BSE_BUY:
            stopPrice = maxFavorablePrice - stopAtrMultiplier * atr; break;
        case BSE_SELL:
           stopPrice = maxFavorablePrice + stopAtrMultiplier * atr; break;
        case BSE_UNDEFINED:
            break;
    }
}

void TradeWrapper::updateTargetPrice(const double price, const double atr) {
    if (targetMode == TargetMode::Flat) return;

    double distance = 0.;

    switch (parentOrderDirection) {
        case BSE_BUY:
            distance = price - fillPrice; break;
        case BSE_SELL:
            distance = fillPrice - price; break;
        case BSE_UNDEFINED:
            break;
    }

    if (const int plateau = static_cast<int>(distance / (2.0 * atr)); plateau > 0) {
        switch (parentOrderDirection) {
            case BSE_BUY:
                targetPrice = fillPrice + (3.0 + plateau) * atr; break;
            case BSE_SELL:
                targetPrice = fillPrice - (3.0 + plateau) * atr; break;
            case BSE_UNDEFINED:
                break;
        }
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

int TradeWrapper::updateOrders(SCStudyInterfaceRef sc) {
    const int successParent = sc.GetOrderByOrderID(parentOrderId, parentOrder);
    const int successStop = sc.GetOrderByOrderID(parentOrder.StopChildInternalOrderID, stopOrder);
    const int successTarget = sc.GetOrderByOrderID(parentOrder.TargetChildInternalOrderID, targetOrder);
    return successParent + successStop + successTarget;
}

[[nodiscard]] double TradeWrapper::getFilledPrice() const {return fillPrice;}

[[nodiscard]] double TradeWrapper::getMaxFavorablePrice() const {return maxFavorablePrice;}

[[nodiscard]] BuySellEnum TradeWrapper::getParentOrderDirection() const {return parentOrder.BuySell;}


[[nodiscard]] SCOrderStatusCodeEnum TradeWrapper::getTargetOrderStatus() const {
    return stopOrder.OrderStatusCode;
}

[[nodiscard]] SCOrderStatusCodeEnum TradeWrapper::getStopOrderStatus() const {
    return targetOrder.OrderStatusCode;
}



