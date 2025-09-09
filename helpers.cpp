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
    const bool TradingAllowed = BarTime >= HMS_TIME(9,  0, 10) && BarTime  < HMS_TIME(14,  45, 0);
    return TradingAllowed;
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

int BidAskDiffBelowLowestPeak(SCStudyInterfaceRef sc, const int StudyId) {
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

long long totalVbP(SCStudyInterfaceRef sc, const int StudyID) {
    // Total volume that exists within the latest volume profile
    const int PricesCount = sc.GetNumPriceLevelsForStudyProfile(StudyID, 0);
    long long res = 0;
    for (int PriceIndex = 0; PriceIndex < PricesCount; PriceIndex++)
    {

        s_VolumeAtPriceV2 VolumeAtPrice;

        int success = sc.GetVolumeAtPriceDataForStudyProfile(StudyID, 0, PriceIndex, VolumeAtPrice);
        if (success > 0) {
            res += VolumeAtPrice.Volume;

        }
    }
    return res;
}


bool lowestOfNBars(SCStudyInterfaceRef sc, const int nBars, const int index) {
    int target = 0;
    for (int i = 0; i < nBars; i++) {
        target += sc.Low[index] <= sc.Low[index - i] ? 1 : 0;
    }
    return target == nBars;
}


bool highestOfNBars(SCStudyInterfaceRef sc, const int nBars, const int index) {
    int target = 0;
    for (int i = 0; i < nBars; i++) {
        target += sc.High[index] >= sc.High[index - i] ? 1 : 0;
    }
    return target == nBars;
}
