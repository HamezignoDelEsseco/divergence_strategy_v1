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
    parentOrderStatus(SCT_OSC_UNSPECIFIED),
    targetMode(mode),
    fillPrice(0),
    maxFavorablePrice(0.0),
    stopAtrMultiplier(stopATRMult),
    targetAtrMultiplier(targetATRMult),
    targetPrice(0.0),
    stopPrice(0.0) {}

void TradeWrapper::updateAttributes(const double price, const double atr) {
    parentOrderStatus = parentOrder.OrderStatusCode;

    if (parentOrderStatus == SCT_OSC_FILLED) {
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
        switch (parentOrderDirection) {
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

[[nodiscard]] SCOrderStatusCodeEnum TradeWrapper::getParentOrderStatus() const {return parentOrderStatus;}



