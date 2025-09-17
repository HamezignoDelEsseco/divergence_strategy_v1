#ifndef TRADEWRAPPER_H
#define TRADEWRAPPER_H

#include "sierrachart.h"

enum class TargetMode { Flat, Evolving };

enum class TradeStatus {Terminated, Active, Other, Expired};

class TradeWrapper {

public:
    TradeWrapper(int64_t parentId, int createdIndex, TargetMode mode, BuySellEnum dir, double constPlateauSize, int expirationBars = 10);

    // Setters
    int fetchAndUpdateOrders(SCStudyInterfaceRef sc);

    void updateAll(SCStudyInterfaceRef sc, int i, double price);

    void updateStopTargetPrice();

    void updatePlateau();

    // Getters
    [[nodiscard]] double getFilledPrice() const;

    [[nodiscard]] double getMaxFavorablePriceDifference() const;

    [[nodiscard]] TradeStatus getRealStatus(int index) const;

    [[nodiscard]] BuySellEnum getParentOrderDirection() const;

    [[nodiscard]] SCOrderStatusCodeEnum getStopOrderStatus() const;

    [[nodiscard]] SCOrderStatusCodeEnum getTargetOrderStatus() const;

    // Sierra Chart ops
    int flattenOrder(SCStudyInterfaceRef sc, double price) const;
    int modifyStopTargetOrders(SCStudyInterfaceRef sc, int i) const;

private:
    const int64_t parentOrderId;
    const int createdIndex;
    const int expirationBars;
    const BuySellEnum parentOrderDirection;
    TargetMode targetMode;
    s_SCTradeOrder parentOrder;
    s_SCTradeOrder stopOrder;
    s_SCTradeOrder targetOrder;
    double fillPrice;
    double maxFavorablePriceDifference;  // Price difference from fill price (starts at 0)
    double targetPrice;
    double stopPrice;
    double constPlateauSize;
    int currentPlateau;  // Current ATR plateau level
};
#endif //TRADEWRAPPER_H