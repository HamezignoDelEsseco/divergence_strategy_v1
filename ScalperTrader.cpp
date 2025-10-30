#include "helpers.h"
#include "sierrachart.h"

SCSFExport scsf_ScalperTraderSignal(SCStudyInterfaceRef sc) {

    SCInputRef MaxContracts = sc.Input[0];
    SCInputRef TargetInTicks = sc.Input[1];
    SCInputRef LossInTicks = sc.Input[2];
    SCInputRef MaxDailyPLInTicks = sc.Input[3];
    SCInputRef MinDailyPLInTicks = sc.Input[4];
    SCInputRef NumberBarsStudy = sc.Input[5];

    SCSubgraphRef InstantMode = sc.Subgraph[0];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Scalper trader signal";

        // Inputs
        MaxContracts.Name = "Maximum number of contracts";
        MaxContracts.SetIntLimits(1, 10);
        MaxContracts.SetInt(8);

        TargetInTicks.Name = "Max target per contract (ticks)";
        TargetInTicks.SetIntLimits(5, 50);
        TargetInTicks.SetInt(10);

        LossInTicks.Name = "Stop loss per contract (ticks)";
        LossInTicks.SetIntLimits(10, 100);
        LossInTicks.SetInt(15);

        MaxDailyPLInTicks.Name = "Max risk per contract (in ticks)";
        MaxDailyPLInTicks.SetIntLimits(10, 200);
        MaxDailyPLInTicks.SetInt(200);

        MinDailyPLInTicks.Name = "Max risk per contract (in ticks)";
        MinDailyPLInTicks.SetIntLimits(-200, -10);
        MinDailyPLInTicks.SetInt(-100);

        NumberBarsStudy.Name = "Number bars study";
        NumberBarsStudy.SetStudyID(1);

        InstantMode.Name = "Instant mode";
        InstantMode.DrawStyle = DRAWSTYLE_LINE;

    }

    float& instantMode = sc.GetPersistentFloat(1);

    const int currIdx = sc.Index;
    const int i = currIdx - 1;

    SCFloatArray AskVBidV;
    sc.GetStudyArrayUsingID(NumberBarsStudy.GetStudyID(), 0, AskVBidV);

    const bool conditionBuy = AskVBidV[i] > 0 && sc.High[currIdx] > sc.High[i];
    const bool conditionSell = AskVBidV[i] < 0 && sc.Low[currIdx] < sc.Low[i];

    if (!conditionBuy && !conditionSell) {
        instantMode = -2;
    }
    if (conditionBuy) {
        instantMode = 1;
    }
    if (conditionSell) {
        instantMode = -1;
    }

    InstantMode[currIdx] = instantMode;

    //s_SCNewOrder newOrder;
    //newOrder.OrderQuantity = 1;  // We buy them one by one, with a maximum of N
    //newOrder.OrderType = SCT_ORDERTYPE_MARKET;
    //newOrder.Target1Offset = TargetInTicks.GetFloat() * sc.TickSize;
    //newOrder.Stop1Offset = LossInTicks.GetFloat() * sc.TickSize;

    //if (instantMode == 1 && PositionData.PositionQuantity == 0) {
    //    //newOrder.Price1 = sc.Open[currIdx] - sc.TickSize;h
    //    sc.BuyOrder(newOrder);
    //}
    //else if (instantMode == -1 && PositionData.PositionQuantity <= 0) {
    //    //newOrder.Price1 = sc.Open[currIdx] + sc.TickSize;
    //    sc.SellOrder(newOrder);
    //}

    //if (PositionData.PositionQuantity > 0 && AskVBidV[i] < 0 && sc.Close[i] < sc.Low[i-1]) {
    //    sc.FlattenAndCancelAllOrders();
    //}

}
