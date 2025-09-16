#ifndef TRADE_MANAGER_H
#define TRADE_MANAGER_H

#include <vector>
#include <cstdint>
#include "sierrachart.h"  // Required for SCStudyInterfaceRef etc.

enum class TradeDirection { Long, Short };
enum class TargetMode { Flat, Evolving };

class Trade {
public:
    Trade(int64_t parentId,
          TradeDirection dir,
          double entryPrice,
          double atr,
          double stopAtrMult,
          double targetAtrMult,
          TargetMode mode);

    void update(double currentPrice, double atr);

    [[nodiscard]] double getStopPrice() const;
    [[nodiscard]] double getTargetPrice() const;
    [[nodiscard]] int64_t getParentOrderId() const;
    [[nodiscard]] bool isActive() const;
    void close();

private:
    int64_t parentOrderId;
    TradeDirection direction;
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
                  TradeDirection dir,
                  double entryPrice,
                  double atr,
                  double stopAtrMult,
                  double targetAtrMult,
                  TargetMode mode);

    void updateAll(double currentPrice, double atr);
    void syncWithSierra(SCStudyInterfaceRef sc);  // apply stops/targets to SC

    Trade* getTradeById(int64_t parentId);
    std::vector<Trade>& getAllTrades();

private:
    std::vector<Trade> trades;

    void modifyAttachedStop(int64_t parentKey, double newStop, SCStudyInterfaceRef sc);
    void modifyAttachedTarget(int64_t parentKey, double newTarget, SCStudyInterfaceRef sc);
};

#endif // TRADE_MANAGER_H
