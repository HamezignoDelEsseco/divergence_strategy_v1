#ifndef TRADE_MANAGER_H
#define TRADE_MANAGER_H

#include <vector>
#include <cstdint>
#include "sierrachart.h"  // Required for SCStudyInterfaceRef etc.

enum class TargetMode { Flat, Evolving };

class Trade {
public:
    Trade(int64_t parentId,
          BuySellEnum dir,
          double entryPrice,
          double atr,
          double stopAtrMult,
          double targetAtrMult,
          TargetMode mode);

    Trade(const s_SCNewOrder &newOrder, double atr, double stopAtrMult, double targetAtrMult, BuySellEnum dir, TargetMode mode);

    void update(double currentPrice, double atr);

    [[nodiscard]] double getStopPrice() const;
    [[nodiscard]] double getTargetPrice() const;
    [[nodiscard]] int64_t getParentOrderId() const;
    [[nodiscard]] bool isActive() const;
    void close();

private:
    int64_t parentOrderId;
    BuySellEnum direction;
    double entryPrice;
    double maxFavorablePrice;
    double stopAtrMultiplier;
    double targetAtrMultiplier;
    TargetMode targetMode;
    double targetPrice;
    double stopPrice;
    bool active;

    void recalcStop(double atr);
    void recalcTarget(double atr, double currentPrice);
};

class TradeManager {
public:
    void addTrade(int64_t parentId,
                  BuySellEnum dir,
                  double entryPrice,
                  double atr,
                  double stopAtrMult,
                  double targetAtrMult,
                  TargetMode mode);

    void addTrade(const Trade& trade);
    void addTrade(Trade&& trade);

    void updateAll(double currentPrice, double atr, SCStudyInterfaceRef sc);
    void syncWithSierra(SCStudyInterfaceRef sc);

    Trade* getTradeById(int64_t parentId);
    std::vector<Trade>& getAllTrades();

private:
    std::vector<Trade> trades;

    void modifyAttachedStop(int64_t parentKey, double newStop, SCStudyInterfaceRef sc);
    void modifyAttachedTarget(int64_t parentKey, double newTarget, SCStudyInterfaceRef sc);

    bool tradeExists(int64_t parentId) const;
    bool isOrderActiveInSierra(int64_t parentId, SCStudyInterfaceRef sc) const;
};

#endif // TRADE_MANAGER_H
