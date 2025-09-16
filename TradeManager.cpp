#include "TradeManager.h"
#include <algorithm>

// ---------------- Trade ----------------
Trade::Trade(int64_t parentId,
             TradeDirection dir,
             double entry,
             double atr,
             double stopAtrMult,
             double targetAtrMult,
             TargetMode mode)
    : parentOrderId(parentId),
      direction(dir),
      entryPrice(entry),
      maxFavorablePrice(entry),
      stopAtrMultiplier(stopAtrMult),
      targetAtrMultiplier(targetAtrMult),
      targetMode(mode),
      active(true)
{
    recalcStop(atr);

    if (targetMode == TargetMode::Flat) {
        if (direction == TradeDirection::Long)
            targetPrice = entryPrice + targetAtrMultiplier * atr;
        else
            targetPrice = entryPrice - targetAtrMultiplier * atr;
    } else {
        // evolving mode starts at 3x ATR
        if (direction == TradeDirection::Long)
            targetPrice = entryPrice + 3.0 * atr;
        else
            targetPrice = entryPrice - 3.0 * atr;
    }
}

void Trade::update(double currentPrice, double atr)
{
    if (!active) return;

    if (direction == TradeDirection::Long)
        maxFavorablePrice = std::max(maxFavorablePrice, currentPrice);
    else
        maxFavorablePrice = std::min(maxFavorablePrice, currentPrice);

    recalcStop(atr);
    recalcTarget(atr, currentPrice);
}

void Trade::recalcStop(double atr)
{
    if (direction == TradeDirection::Long)
        stopPrice = maxFavorablePrice - stopAtrMultiplier * atr;
    else
        stopPrice = maxFavorablePrice + stopAtrMultiplier * atr;
}

void Trade::recalcTarget(double atr, double currentPrice)
{
    if (targetMode == TargetMode::Flat) return;

    double distance = (direction == TradeDirection::Long)
                      ? currentPrice - entryPrice
                      : entryPrice - currentPrice;

    int plateau = static_cast<int>(distance / (2.0 * atr));

    if (plateau > 0) {
        if (direction == TradeDirection::Long)
            targetPrice = entryPrice + (3.0 + plateau) * atr;
        else
            targetPrice = entryPrice - (3.0 + plateau) * atr;
    }
}

[[nodiscard]] double Trade::getStopPrice() const { return stopPrice; }
[[nodiscard]] double Trade::getTargetPrice() const { return targetPrice; }
[[nodiscard]] int64_t Trade::getParentOrderId() const { return parentOrderId; }
[[nodiscard]] bool Trade::isActive() const { return active; }
void Trade::close() { active = false; }

// ---------------- TradeManager ----------------
void TradeManager::addTrade(int64_t parentId,
                            TradeDirection dir,
                            double entryPrice,
                            double atr,
                            double stopAtrMult,
                            double targetAtrMult,
                            TargetMode mode)
{
    trades.emplace_back(parentId, dir, entryPrice, atr, stopAtrMult, targetAtrMult, mode);
}

void TradeManager::updateAll(double currentPrice, double atr)
{
    for (auto& trade : trades)
        trade.update(currentPrice, atr);
}

void TradeManager::syncWithSierra(SCStudyInterfaceRef sc)
{
    for (auto& trade : trades)
    {
        if (!trade.isActive()) continue;

        modifyAttachedStop(trade.getParentOrderId(), trade.getStopPrice(), sc);
        modifyAttachedTarget(trade.getParentOrderId(), trade.getTargetPrice(), sc);
    }
}

void TradeManager::modifyAttachedStop(int64_t parentKey, double newStop, SCStudyInterfaceRef sc)
{
    s_SCTradeOrder parent, stopOrder;
    if (sc.GetOrderByOrderID(parentKey, parent) == 1)
    {
        if (sc.GetOrderByOrderID(parent.StopChildInternalOrderID, stopOrder) == 1)
        {
            s_SCNewOrder modify;
            modify.InternalOrderID = stopOrder.InternalOrderID;
            modify.Price1 = newStop;
            sc.ModifyOrder(modify);
        }
    }
}

void TradeManager::modifyAttachedTarget(int64_t parentKey, double newTarget, SCStudyInterfaceRef sc)
{
    s_SCTradeOrder parent, targetOrder;
    if (sc.GetOrderByOrderID(parentKey, parent) == 1)
    {
        if (sc.GetOrderByOrderID(parent.TargetChildInternalOrderID, targetOrder) == 1)
        {
            s_SCNewOrder modify;
            modify.InternalOrderID = targetOrder.InternalOrderID;
            modify.Price1 = newTarget;
            sc.ModifyOrder(modify);
        }
    }
}

Trade* TradeManager::getTradeById(int64_t parentId)
{
    for (auto& trade : trades)
    {
        if (trade.getParentOrderId() == parentId)
            return &trade;
    }
    return nullptr;
}

std::vector<Trade>& TradeManager::getAllTrades() { return trades; }
