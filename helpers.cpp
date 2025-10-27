#include "helpers.h"
#include "sierrachart.h"


bool IsCleanTick(const float priceOfInterest, SCStudyInterfaceRef sc, const int minVolume, const int offset) {
    const int priceOfInterestInTicks = sc.PriceValueToTicks(priceOfInterest);
    const s_VolumeAtPriceV2 elem = sc.VolumeAtPriceForBars->GetVAPElementAtPrice(sc.Index - offset, priceOfInterestInTicks);

    const bool res = elem.BidVolume > minVolume & elem.AskVolume > minVolume;
    return res;
}

bool tradingAllowedCash(SCStudyInterfaceRef sc) {
    const int BarTime = sc.BaseDateTimeIn[sc.Index].GetTime();
    const bool TradingAllowed = BarTime >= HMS_TIME(9,  30, 0) && BarTime  < HMS_TIME(15,  30, 0);
    return TradingAllowed;
}

void flattenAllAfterCash(SCStudyInterfaceRef sc) {
    const int BarTime = sc.BaseDateTimeIn[sc.Index].GetTime();
    const bool OutOfSession = BarTime < HMS_TIME(9,  30, 0) | BarTime  >= HMS_TIME(15,  30, 0);
    if (OutOfSession) {
        sc.FlattenAndCancelAllOrders();
    }
}

void orderToLogs(SCStudyInterfaceRef sc, s_SCTradeOrder order, int mode) {
    std::string message = "";
}

void highLowCleanPricesInBar(SCStudyInterfaceRef sc, double &minPrice, double &maxPrice, const int offset) {

    // These will be the default values in case the bar has only unclean ticks
    int L, H;
    sc.VolumeAtPriceForStudy->GetHighAndLowPriceTicksForBarIndex(sc.Index, H, L);

    const s_VolumeAtPriceV2* pVAP = nullptr; // pVAP points to a constant s_VolumeAtPriceV2
    const int vapCount = static_cast<int>(sc.VolumeAtPriceForBars->GetSizeAtBarIndex(sc.Index - offset));

    std::optional<int> tmpMin, tmpMax;
    for (int i = 0; i < vapCount; ++i) {
        if (sc.VolumeAtPriceForBars->GetVAPElementAtIndex(sc.Index, i, &pVAP)) {
            if (!tmpMin.has_value() && !tmpMax.has_value() && pVAP->BidVolume > 0 and pVAP->AskVolume > 0) {
                tmpMin = pVAP->PriceInTicks;
                tmpMax = pVAP->PriceInTicks;
            } else {
                if (pVAP->BidVolume > 0 and pVAP->AskVolume > 0) {
                    tmpMin = std::min<int>(tmpMin.value(), pVAP->PriceInTicks);
                    tmpMax = std::max<int>(tmpMin.value(), pVAP->PriceInTicks);
                }
            }
        }
    }
    if (!tmpMin.has_value() && !tmpMax.has_value()) {
        tmpMin = L; tmpMax = H;
    }
    minPrice = sc.TicksToPriceValue(tmpMin.value());
    maxPrice = sc.TicksToPriceValue(tmpMax.value());
}

void BidAskDiffBelowLowestPeak(SCStudyInterfaceRef sc, const int StudyId) {
    float PeakValleyLinePrice= 0;
    int PeakValleyType = 0;
    int StartIndex = sc.Index;
    int PeakValleyExtensionChartColumnEndIndex = 0;
    int keep_going = 1;
    int peakValleyIndex = 0;
    float maxPeakValley = 0;

    // get the first Peak/Valley line from the last volume by price profile
    while (keep_going == 1) {
        keep_going = sc.GetStudyPeakValleyLine(
            sc.ChartNumber,
            StudyId,
            PeakValleyLinePrice,
            PeakValleyType,
            StartIndex,
            PeakValleyExtensionChartColumnEndIndex,
            sc.Index,
            peakValleyIndex);
        peakValleyIndex ++;
        maxPeakValley = std::max<float>(maxPeakValley, PeakValleyLinePrice);
    }
}

void LogAttachedStop(int64_t parentKey, SCStudyInterfaceRef sc) {
    if (s_SCTradeOrder ParentOrder, StopOrder; sc.GetOrderByOrderID(parentKey, ParentOrder) == 1) {
        const int stopOrderSuccess = sc.GetOrderByOrderID(ParentOrder.StopChildInternalOrderID, StopOrder);
        if (stopOrderSuccess == 1) {
            SCString Buffer;
            Buffer.Format("Current stop price of order %d is %f", StopOrder.InternalOrderID, static_cast<float>(StopOrder.Price1));
            sc.AddMessageToLog(Buffer, 1);
        }
    }
}

void ModifyAttachedStop(int64_t parentKey, double newStop, SCStudyInterfaceRef sc) {
    if (s_SCTradeOrder ParentOrder, StopOrder; sc.GetOrderByOrderID(parentKey, ParentOrder) == 1) {
        const int stopOrderSuccess = sc.GetOrderByOrderID(ParentOrder.StopChildInternalOrderID, StopOrder);
        if (stopOrderSuccess == 1) {
            s_SCNewOrder NewStopOrder;
            NewStopOrder.InternalOrderID = StopOrder.InternalOrderID;
            NewStopOrder.Price1 = newStop;
            sc.ModifyOrder(NewStopOrder);
        }
    }
}

bool lowestOfNBars(SCStudyInterfaceRef sc, const int nBars, const int index) {
    int target = 0;
    for (int i = 0; i < nBars; i++) {
        target += sc.Low[index] <= sc.Low[index - i] ? 1 : 0;
    }
    return target == nBars;
}


bool isNewTradingDayRange(SCStudyInterfaceRef sc, int range, int Ix) {
    int result = 0;
    for (int i = 0; i < range; i++) {
        if (sc.IsNewTradingDay(Ix - i)) {
            result++;
        }
    }
    return result > 0;
}


bool highestOfNBars(SCStudyInterfaceRef sc, const int nBars, const int index) {
    int target = 0;
    for (int i = 0; i < nBars; i++) {
        target += sc.High[index] >= sc.High[index - i] ? 1 : 0;
    }
    return target == nBars;
}

bool consecutivePosNegDeltaVol(SCFloatArray& askbidvoldiff, const int nBars, const int index, const int direction) {
    int target = 0;
    for (int i = 0; i < nBars; i++) {
        if (direction >=0) {
            target += askbidvoldiff[index-i] > 0 ? 1 : 0;

        } else {
            target += askbidvoldiff[index-i] < 0 ? 1 : 0;
        }
    }
    return target == nBars;
}

int workingParentOrder(SCStudyInterfaceRef sc, uint32_t &workingParentOrder) {
    int Index = 0;
    s_SCTradeOrder OrderDetails;
    int res = 0;
    while(sc.GetOrderByIndex(Index, OrderDetails) != SCTRADING_ORDER_ERROR)
    {
        Index++;

        if (IsWorkingOrderStatus(OrderDetails.OrderStatusCode) && OrderDetails.ParentInternalOrderID == 0) {
            workingParentOrder = OrderDetails.InternalOrderID;
            res = 1;
        }
    }
    return res;
}

int isInsideTrade(SCStudyInterfaceRef sc) {
    int Index = 0;
    int workingParents = 0;
    int workingAttached = 0;
    s_SCTradeOrder OrderDetails;
    while(sc.GetOrderByIndex(Index, OrderDetails) != SCTRADING_ORDER_ERROR)
    {
        Index++;

        if (IsWorkingOrderStatus(OrderDetails.OrderStatusCode) && OrderDetails.ParentInternalOrderID == 0) {
            workingParents++;
        }

        if (IsWorkingOrderStatus(OrderDetails.OrderStatusCode) && OrderDetails.ParentInternalOrderID != 0) {
            workingAttached ++;
        }
        //Get the internal order ID
    }
    return workingParents == 0 && workingAttached > 0 ? 1 : 0;
}