#ifndef INC_999_LEARN_HELPERS_H
#define INC_999_LEARN_HELPERS_H

#endif //INC_999_LEARN_HELPERS_H

#include "sierrachart.h"




bool IsCleanTick(float priceOfInterest, SCStudyInterfaceRef sc, int minVolume = 0, int offset = 0);

bool tradingAllowedCash(SCStudyInterfaceRef sc);

void orderToLogs(SCStudyInterfaceRef sc, s_SCTradeOrder order);

void highLowCleanPricesInBar(SCStudyInterfaceRef sc, double& minPrice, double& maxPrice, int offset = 0);

void BidAskDiffBelowLowestPeak(SCStudyInterfaceRef sc);


template <typename... Args>
void colorAllSubGraphs(SCStudyInterfaceRef sc, const int i, Args&... args) {
    (
        [&] (SCSubgraphRef& arg) {
            if (arg[i] < 0) {
                arg.DataColor[sc.Index] =
                    sc.CombinedForegroundBackgroundColorRef(COLOR_RED, COLOR_BLACK);
            } else {
                arg.DataColor[sc.Index] =
                    sc.CombinedForegroundBackgroundColorRef(COLOR_LIGHTGREEN, COLOR_BLACK);
            }
        }(args), ...
    );
}
void LogAttachedStop(int64_t parentKey, SCStudyInterfaceRef sc);

void ModifyAttachedStop(int64_t parentKey, double newStop, SCStudyInterfaceRef sc);

void flattenAllAfterCash(SCStudyInterfaceRef sc);

bool lowestOfNBars(SCStudyInterfaceRef sc, int nBars, int index);

bool lowestOfNBarsWithBuffer(SCStudyInterfaceRef sc, int nBars, int index, float buffer);

bool highestOfNBars(SCStudyInterfaceRef sc, int nBars, int index);

bool highestOfNBarsWithBuffer(SCStudyInterfaceRef sc, int nBars, int index, float buffer);

void highestLowestOfNBars(SCStudyInterfaceRef sc, int nBars, int index, double& lowest, double &highest);

bool consecutivePosNegDeltaVol(SCFloatArray& askbidvoldiff,  int nBars, int index, int direction);

bool isNewTradingDayRange(SCStudyInterfaceRef sc, int range, int Ix);

int isInsideTrade(SCStudyInterfaceRef sc);

int workingParentOrder(SCStudyInterfaceRef sc, uint32_t &workingParentOrder);

bool higherHighsHigherLows(SCStudyInterfaceRef sc, int idx, int nBars);

float higherHighsLowerLows(SCStudyInterfaceRef sc, int idx, int nBars);