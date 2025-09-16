#ifndef TRADEWRAPPER_H
#define TRADEWRAPPER_H

#include "sierrachart.h"

enum class TargetMode { Flat, Evolving };

class TradeWrapper {
public:
    TradeWrapper(int64_t parentId, TargetMode mode, double stopATRMult, double targetATRMult, BuySellEnum dir);

    int updateOrders(SCStudyInterfaceRef sc);

    void updateAttributes(double price, double atr);

    void updateStopPrice(double atr);
    void updateTargetPrice(double price, double atr);

    int flattenOrder(SCStudyInterfaceRef sc, double price) const;

    // Getters
    [[nodiscard]] double getFilledPrice() const;
    [[nodiscard]] double getMaxFavorablePrice() const;
    [[nodiscard]] SCOrderStatusCodeEnum getParentOrderStatus() const;


private:
    const int64_t parentOrderId;
    const BuySellEnum parentOrderDirection;
    SCOrderStatusCodeEnum parentOrderStatus;
    TargetMode targetMode;
    s_SCTradeOrder parentOrder;
    s_SCTradeOrder stopOrder;
    s_SCTradeOrder targetOrder;
    double fillPrice;
    double maxFavorablePrice;
    double stopAtrMultiplier;
    double targetAtrMultiplier;
    double targetPrice;
    double stopPrice;
};
#endif //TRADEWRAPPER_H