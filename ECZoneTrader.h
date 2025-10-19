#ifndef EXTENSIONCOMPRESSIONTRADER_H
#define EXTENSIONCOMPRESSIONTRADER_H
#include "sierrachart.h"


enum class OrderStatus {Idle, Active, Terminated};

class ECZoneTrader {

public:
    ECZoneTrader(float zoneID, BuySellEnum buySell, float targetTickOffset1, float stopTickOffset1, int retests, const SCFloatArray& entryPrice);

private:
    int64_t parentOrderId = 0;
    OrderStatus orderStatus = OrderStatus::Idle;
    float zoneID;
    int retests;
    const SCFloatArray& entryPrice;
    const float targetTickOffset1;
    const float stopTickOffset1;
};

#endif //EXTENSIONCOMPRESSIONTRADER_H