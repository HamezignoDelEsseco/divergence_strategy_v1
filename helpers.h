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
