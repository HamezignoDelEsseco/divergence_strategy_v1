#include "TradeManager.h"


Trade::Trade(const int64_t parentId,
          const BuySellEnum dir,
          const double entryPrice,
          const double atr,
          const double stopAtrMult,
          const double targetAtrMult,
          const TargetMode mode)
    : parentOrderId(parentId),
    direction(dir),
    entryPrice(entryPrice),
    maxFavorablePrice(entryPrice),
    stopAtrMultiplier(stopAtrMult),
    targetAtrMultiplier(targetAtrMult),
    targetMode(mode),
    stopPrice(0),
    active(true)
{
    recalcStop(atr);

    if (targetMode == TargetMode::Flat) {
        if (direction == BSE_BUY)
            targetPrice = entryPrice + targetAtrMultiplier * atr;
        else
            targetPrice = entryPrice - targetAtrMultiplier * atr;
    } else {
        // evolving mode starts at 3x ATR
        if (direction == BSE_BUY)
            targetPrice = entryPrice + 3.0 * atr;
        else
            targetPrice = entryPrice - 3.0 * atr;
    }
}

Trade::Trade(const s_SCNewOrder &newOrder, const double atr, const double stopAtrMult, const double targetAtrMult, const BuySellEnum dir, const TargetMode mode):
    parentOrderId(newOrder.InternalOrderID),
    direction(dir),
    entryPrice(newOrder.Price1),
    maxFavorablePrice(newOrder.Price1),
    stopAtrMultiplier(stopAtrMult),
    targetAtrMultiplier(targetAtrMult),
    targetMode(mode),
    stopPrice(0),
    active(true) {

    recalcStop(atr);

    if (targetMode == TargetMode::Flat) {
        if (direction == BSE_BUY)
            targetPrice = entryPrice + targetAtrMultiplier * atr;
        else
            targetPrice = entryPrice - targetAtrMultiplier * atr;
    } else {
        // evolving mode starts at 3x ATR
        if (direction == BSE_BUY)
            targetPrice = entryPrice + 3.0 * atr;
        else
            targetPrice = entryPrice - 3.0 * atr;
    }
}



void Trade::update(const double currentPrice, const double atr)
{
    if (!active) return;

    if (direction == BSE_BUY)
        maxFavorablePrice = std::max(maxFavorablePrice, currentPrice);
    else
        maxFavorablePrice = std::min(maxFavorablePrice, currentPrice);

    recalcStop(atr);
    recalcTarget(atr, currentPrice);
}

void Trade::recalcStop(const double atr)
{
    if (direction == BSE_BUY)
        stopPrice = maxFavorablePrice - stopAtrMultiplier * atr;
    else
        stopPrice = maxFavorablePrice + stopAtrMultiplier * atr;
}

void Trade::recalcTarget(const double atr, const double currentPrice)
{
    if (targetMode == TargetMode::Flat) return;

    double distance = (direction == BSE_BUY)
                      ? currentPrice - entryPrice
                      : entryPrice - currentPrice;

    int plateau = static_cast<int>(distance / (2.0 * atr));

    if (plateau > 0) {
        if (direction == BSE_BUY)
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
                            BuySellEnum dir,
                            double entryPrice,
                            double atr,
                            double stopAtrMult,
                            double targetAtrMult,
                            TargetMode mode)
{
    if (tradeExists(parentId)) return; // prevent duplicates
    trades.emplace_back(parentId, dir, entryPrice, atr, stopAtrMult, targetAtrMult, mode);
}

void TradeManager::addTrade(const Trade& trade)
{
    if (tradeExists(trade.getParentOrderId())) return; // prevent duplicates
    trades.push_back(trade);
}

void TradeManager::addTrade(Trade&& trade)
{
    if (tradeExists(trade.getParentOrderId())) return; // prevent duplicates
    //trades.emplace_back(std::move(trade));
    trades.emplace_back(trade);
}

void TradeManager::updateAll(double currentPrice, double atr, SCStudyInterfaceRef sc)
{
    for (auto& trade : trades)
        trade.update(currentPrice, atr);

    // auto-cleanup: erase trades whose parent orders are gone
    trades.erase(
        std::remove_if(trades.begin(), trades.end(),
                       [&](const Trade& t) {
                           return !isOrderActiveInSierra(t.getParentOrderId(), sc);
                       }),
        trades.end());
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

bool TradeManager::tradeExists(int64_t parentId) const
{
    for (const auto& t : trades)
        if (t.getParentOrderId() == parentId)
            return true;
    return false;
}

bool TradeManager::isOrderActiveInSierra(int64_t parentId, SCStudyInterfaceRef sc) const
{
    s_SCTradeOrder order;
    if (sc.GetOrderByOrderID(parentId, order) != 1)
        return false; // no such order

    // Only keep if still open
    return order.OrderStatusCode == SCT_OSC_OPEN;
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