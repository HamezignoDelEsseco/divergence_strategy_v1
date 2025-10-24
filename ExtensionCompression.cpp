#include "sierrachart.h"
#include "helpers.h"

enum PendingEntryType
{
    EXTENSION_LONG = 0
    , EXTENSION_SHORT = 1
    , COMPRESSION_LONG = 2
    , COMPRESSION_SHORT = 3
    , IDLE = 4
};

enum CompressionExtensionType {NONE = 0, EXTENSION = 1, COMPRESSION = 2};

void resetNBars(int& aboveFH, int& belowFH, int& aboveFL, int& belowFL) {
    aboveFH = 1;
    belowFH = 1;
    aboveFL = 1;
    belowFL = 1;
}

void updateNBars(const double high, const double low, const float fh, const float fl, int& aboveFH, int& belowFH, int& aboveFL, int& belowFL) {
    if (high >= fh)
        aboveFH += 1;
    if (low <= fh)
        belowFH += 1;
    if (high >= fl)
        aboveFL += 1;
    if (high <= fl)
        belowFL += 1;
}

int modiFyParentOrder(SCStudyInterfaceRef sc, const uint32_t parentOrderId, float newPrice) {
    s_SCNewOrder ModifiyOrder;
    ModifiyOrder.InternalOrderID = parentOrderId;
    ModifiyOrder.InternalOrderID2 = parentOrderId;

    ModifiyOrder.Price1 = newPrice;
    const int success = sc.ModifyOrder(ModifiyOrder);
    return success;
}

double createAndSubmitOrder(
    SCStudyInterfaceRef sc, const float price1,
    const BuySellEnum direction
    ) {
    s_SCNewOrder NewOrder;
    NewOrder.OrderQuantity = 1;
    NewOrder.OrderType = SCT_ORDERTYPE_LIMIT;
    NewOrder.TimeInForce = SCT_TIF_DAY;
    NewOrder.Price1 = price1;
    double success = 0;
    if (direction == BSE_BUY) {
        success = sc.BuyOrder(NewOrder);
    } else if (direction == BSE_SELL) {
        success = sc.SellOrder(NewOrder);
    }
    return success;
};

template<typename... Subgraphs>
void ResetSubgraphsAtIndex(int index, Subgraphs&... sgs)
{
    // Fold expression: expands the parameter pack and applies the reset
    ((sgs[index] = 0.0f), ...);
}

float getEntryPrice(const float fvhsr, const float fvlsr, const float h, const float l) {
    if (fvhsr !=0) {
        return h;
    }
    if (fvlsr != 0) {
        return l;
    }
    return 0;
}

SCSFExport scsf_CompressionExtensionSignal(SCStudyInterfaceRef sc) {
    SCInputRef StudyLines = sc.Input[0];
    SCInputRef MinNumberOfExtensionCompressionTicks = sc.Input[1];
    SCInputRef TicksForJump = sc.Input[2];
    SCInputRef ResetOnJump = sc.Input[3];

    SCSubgraphRef ExtensionID = sc.Subgraph[0];
    SCSubgraphRef ExtensionSymmetricPriceFH = sc.Subgraph[1];
    SCSubgraphRef ExtensionSymmetricPriceFL = sc.Subgraph[2];

    SCSubgraphRef CurrentStatus = sc.Subgraph[3];



    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Compression extension zones";

        // Inputs
        StudyLines.Name = "Study lines";
        StudyLines.SetStudyID(1);

        MinNumberOfExtensionCompressionTicks.Name = "Minimum number of extension compression ticks";
        MinNumberOfExtensionCompressionTicks.SetIntLimits(0, 50);
        MinNumberOfExtensionCompressionTicks.SetInt(1);

        TicksForJump.Name = "Ticks for jump";
        TicksForJump.SetIntLimits(5,100);
        TicksForJump.SetInt(10);

        ResetOnJump.Name = "Reset extension compression on jump";
        ResetOnJump.SetYesNo(1);

        // Outputs
        ExtensionID.Name = "Extension start index";
        ExtensionID.DrawStyle = DRAWSTYLE_IGNORE;
        ExtensionSymmetricPriceFH.Name = "Extension sym price high";
        ExtensionSymmetricPriceFH.DrawStyle = DRAWSTYLE_LINE;
        ExtensionSymmetricPriceFL.Name = "Extension sym price low";
        ExtensionSymmetricPriceFL.DrawStyle = DRAWSTYLE_LINE;

        // FVDiff.Name = "FV diff";
        // FVDiff.DrawStyle = DRAWSTYLE_IGNORE;
        // FVDiffIncrement.Name = "FV Diff increment";
        // FVDiffIncrement.DrawStyle = DRAWSTYLE_IGNORE;
//
        // CumulativeFVDiffPos.Name = "Cumulative FV diff positive";
        // CumulativeFVDiffPos.DrawStyle = DRAWSTYLE_IGNORE;
        // CumulativeFVDiffNeg.Name = "Cumulative FV diff negative";
        // CumulativeFVDiffNeg.DrawStyle = DRAWSTYLE_IGNORE;

        CurrentStatus.Name = "Current status";
        CurrentStatus.DrawStyle = DRAWSTYLE_IGNORE;
    }
    const int i = sc.Index;
    SCString Buffer;

    // Retrieving Studies
    SCFloatArray FH;
    SCFloatArray FL;
    SCFloatArray P;
    sc.GetStudyArrayUsingID(StudyLines.GetStudyID(), 0, P);
    sc.GetStudyArrayUsingID(StudyLines.GetStudyID(), 1, FH);
    sc.GetStudyArrayUsingID(StudyLines.GetStudyID(), 2, FL);

    // Intermediate results used for plotting the symmetric extensions / compressions
    SCFloatArray& FVHDiff = ExtensionSymmetricPriceFH.Arrays[0];
    SCFloatArray& FVHDiffCum = ExtensionSymmetricPriceFH.Arrays[1];
    SCFloatArray& FVLDiff = ExtensionSymmetricPriceFL.Arrays[0];
    SCFloatArray& FVLDiffCum = ExtensionSymmetricPriceFL.Arrays[1];

    // Intermediate results for computing the compression / extension zones
    SCFloatArray& FVDiff = CurrentStatus.Arrays[0];
    SCFloatArray& FVDiffIncrement = CurrentStatus.Arrays[1];
    SCFloatArray& CumulativeFVDiffNeg = CurrentStatus.Arrays[3];
    SCFloatArray& CumulativeFVDiffPos = CurrentStatus.Arrays[4];


    float currentType;

    FVDiff[i] = (FH[i] - FL[i]) / sc.TickSize;
    FVDiffIncrement[i] = FVDiff[i] - FVDiff[i-1];


    FVHDiff[i] = FH[i] - FH[i-1];
    FVHDiffCum[i] = FVHDiff[i] + FVHDiffCum[i-1];
    FVLDiff[i] = FL[i] - FL[i-1];
    FVLDiffCum[i] = FVLDiff[i] + FVLDiffCum[i-1];



    if(FVDiffIncrement[i] == 0) {
        CumulativeFVDiffNeg[i] = CumulativeFVDiffNeg[i-1];
        CumulativeFVDiffPos[i] = CumulativeFVDiffPos[i-1];
    } else if (FVDiffIncrement[i] < 0) { // As soon as negative increment, we reset the positive sum and vice versa
        CumulativeFVDiffPos[i] = 0;
        CumulativeFVDiffNeg[i] = CumulativeFVDiffNeg[i-1] + FVDiffIncrement[i];
    } else {
        CumulativeFVDiffNeg[i] = 0;
        CumulativeFVDiffPos[i] = CumulativeFVDiffPos[i-1] + FVDiffIncrement[i];
     }


    // Regime change logic
    if (CumulativeFVDiffPos[i] >= MinNumberOfExtensionCompressionTicks.GetFloat() && CurrentStatus[i-1] != 1) { // Switch to Extension
        currentType = 1;
        CumulativeFVDiffPos[i] = 0;
        ResetSubgraphsAtIndex(i, CumulativeFVDiffPos, FVDiffIncrement, FVHDiffCum, FVLDiffCum); // DO NOT RESET FVDiff !!!
        //FVHDiffCum[i] = 0;
        //FVLDiffCum[i] = 0;
        ExtensionID[i] = ExtensionID[i-1] + 1;

    } else if (CumulativeFVDiffNeg[i] <= -MinNumberOfExtensionCompressionTicks.GetFloat() && CurrentStatus[i-1] != 2) { // Switch to Compression
        currentType = 2;
        CumulativeFVDiffPos[i] = 0;
        ResetSubgraphsAtIndex(i, CumulativeFVDiffPos, FVDiffIncrement, FVHDiffCum, FVLDiffCum); // DO NOT RESET FVDiff !!!
        // FVHDiffCum[i] = 0;
        // FVLDiffCum[i] = 0;
        ExtensionID[i] = ExtensionID[i-1] + 1;

    } else {
        currentType = CurrentStatus[i-1];
        ExtensionID[i] = ExtensionID[i-1];
    }


    // Reset on new day or beginning of chart
    if (sc.IsNewTradingDay(i) || i <= 0) {
        CumulativeFVDiffPos[i] = 0;
        CumulativeFVDiffNeg[i] = 0;
        FVDiffIncrement[i] = 0;
        FVHDiffCum[i] = 0;
        FVLDiffCum[i] = 0;
        ExtensionID[i] = 0;
        currentType = 0;
    }

    // Reset on jump condition
    if (ResetOnJump.GetInt() == 1 && (abs(FH[i] - FH[i-1]) >= TicksForJump.GetFloat() || abs(FL[i] - FL[i-1]) >= TicksForJump.GetFloat())) {
        CumulativeFVDiffPos[i] = 0;
        FVDiffIncrement[i] = 0;
        CumulativeFVDiffNeg[i] = 0;
        currentType = 0;
    }

    CurrentStatus[i] = currentType;
    ExtensionSymmetricPriceFH[i] = FH[i] - FVLDiffCum[i];
    ExtensionSymmetricPriceFL[i] = FL[i] - FVHDiffCum[i];

}

SCSFExport scsf_CompressionExtensionSignalLag(SCStudyInterfaceRef sc) {
    SCInputRef StudyLines = sc.Input[0];
    SCInputRef MinNumberOfExtensionCompressionTicks = sc.Input[1];
    SCInputRef TicksForJump = sc.Input[2];
    SCInputRef ResetOnJump = sc.Input[3];

    SCSubgraphRef ExtensionID = sc.Subgraph[0];
    SCSubgraphRef ExtensionSymmetricPriceFH = sc.Subgraph[1];
    SCSubgraphRef ExtensionSymmetricPriceFL = sc.Subgraph[2];
    SCSubgraphRef CurrentStatus = sc.Subgraph[3];
    SCSubgraphRef ExtensionStartIndex = sc.Subgraph[4];




    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "CEZ - Compression extension zones (lag)";

        // Inputs
        StudyLines.Name = "Study lines";
        StudyLines.SetStudyID(1);

        MinNumberOfExtensionCompressionTicks.Name = "Minimum number of extension compression ticks";
        MinNumberOfExtensionCompressionTicks.SetIntLimits(0, 50);
        MinNumberOfExtensionCompressionTicks.SetInt(1);

        TicksForJump.Name = "Ticks for jump";
        TicksForJump.SetIntLimits(5,100);
        TicksForJump.SetInt(10);

        ResetOnJump.Name = "Reset extension compression on jump";
        ResetOnJump.SetYesNo(1);

        // Outputs
        ExtensionID.Name = "Extension ID";
        ExtensionID.DrawStyle = DRAWSTYLE_IGNORE;
        ExtensionSymmetricPriceFH.Name = "Extension sym price high";
        ExtensionSymmetricPriceFH.DrawStyle = DRAWSTYLE_LINE;
        ExtensionSymmetricPriceFL.Name = "Extension sym price low";
        ExtensionSymmetricPriceFL.DrawStyle = DRAWSTYLE_LINE;

        CurrentStatus.Name = "Current status";
        CurrentStatus.DrawStyle = DRAWSTYLE_IGNORE;

        ExtensionStartIndex.Name = "Zone start index";
        ExtensionStartIndex.DrawStyle = DRAWSTYLE_IGNORE;
    }
    const int prevIx = sc.Index - 1;

    // Recalculation logic for study optimization
    // Because we are LAGGING, we do not need to recompute iat every tick as long as the previous bar (closed...)
    // is already computed. It's already computed if its symmetric FVH is set (meaning it's non 0)
    if (ExtensionSymmetricPriceFH[prevIx] != 0 && !sc.IsNewTradingDay(prevIx) && prevIx > 0) {
        return;
    }

    // Retrieving Studies
    SCFloatArray FH;
    SCFloatArray FL;
    SCFloatArray P;
    sc.GetStudyArrayUsingID(StudyLines.GetStudyID(), 0, P);
    sc.GetStudyArrayUsingID(StudyLines.GetStudyID(), 1, FH);
    sc.GetStudyArrayUsingID(StudyLines.GetStudyID(), 2, FL);

    // Intermediate results used for plotting the symmetric extensions / compressions
    SCFloatArray& FVHDiff = ExtensionSymmetricPriceFH.Arrays[0];
    SCFloatArray& FVHDiffCum = ExtensionSymmetricPriceFH.Arrays[1];
    SCFloatArray& FVLDiff = ExtensionSymmetricPriceFL.Arrays[0];
    SCFloatArray& FVLDiffCum = ExtensionSymmetricPriceFL.Arrays[1];

    // Intermediate results for computing the compression / extension zones
    SCFloatArray& FVDiff = CurrentStatus.Arrays[0];
    SCFloatArray& FVDiffIncrement = CurrentStatus.Arrays[1];
    SCFloatArray& CumulativeFVDiffNeg = CurrentStatus.Arrays[3];
    SCFloatArray& CumulativeFVDiffPos = CurrentStatus.Arrays[4];


    float currentType;

    FVDiff[prevIx] = (FH[prevIx] - FL[prevIx]) / sc.TickSize;
    FVDiffIncrement[prevIx] = FVDiff[prevIx] - FVDiff[prevIx-1];


    FVHDiff[prevIx] = FH[prevIx] - FH[prevIx-1];
    FVHDiffCum[prevIx] = FVHDiff[prevIx] + FVHDiffCum[prevIx-1];
    FVLDiff[prevIx] = FL[prevIx] - FL[prevIx-1];
    FVLDiffCum[prevIx] = FVLDiff[prevIx] + FVLDiffCum[prevIx-1];



    if(FVDiffIncrement[prevIx] == 0) {
        CumulativeFVDiffNeg[prevIx] = CumulativeFVDiffNeg[prevIx-1];
        CumulativeFVDiffPos[prevIx] = CumulativeFVDiffPos[prevIx-1];
    } else if (FVDiffIncrement[prevIx] < 0) { // As soon as negative increment, we reset the positive sum and vice versa
        CumulativeFVDiffPos[prevIx] = 0;
        CumulativeFVDiffNeg[prevIx] = CumulativeFVDiffNeg[prevIx-1] + FVDiffIncrement[prevIx];
    } else {
        CumulativeFVDiffNeg[prevIx] = 0;
        CumulativeFVDiffPos[prevIx] = CumulativeFVDiffPos[prevIx-1] + FVDiffIncrement[prevIx];
     }


    // Regime change logic
    if (CumulativeFVDiffPos[prevIx] >= MinNumberOfExtensionCompressionTicks.GetFloat() && CurrentStatus[prevIx-1] != 1) { // Switch to Extension
        currentType = 1;
        CumulativeFVDiffPos[prevIx] = 0;
        ResetSubgraphsAtIndex(prevIx, CumulativeFVDiffPos, FVDiffIncrement, FVHDiffCum, FVLDiffCum); // DO NOT RESET FVDiff !!!
        //FVHDiffCum[i] = 0;
        //FVLDiffCum[i] = 0;
        ExtensionID[prevIx] = ExtensionID[prevIx-1] + 1;
        ExtensionStartIndex[prevIx] = prevIx;

    } else if (CumulativeFVDiffNeg[prevIx] <= -MinNumberOfExtensionCompressionTicks.GetFloat() && CurrentStatus[prevIx-1] != 2) { // Switch to Compression
        currentType = 2;
        CumulativeFVDiffPos[prevIx] = 0;
        ResetSubgraphsAtIndex(prevIx, CumulativeFVDiffPos, FVDiffIncrement, FVHDiffCum, FVLDiffCum); // DO NOT RESET FVDiff !!!
        // FVHDiffCum[i] = 0;
        // FVLDiffCum[i] = 0;
        ExtensionID[prevIx] = ExtensionID[prevIx-1] + 1;
        ExtensionStartIndex[prevIx] = prevIx;

    } else {
        currentType = CurrentStatus[prevIx-1];
        ExtensionID[prevIx] = ExtensionID[prevIx-1];
        ExtensionStartIndex[prevIx] = ExtensionStartIndex[prevIx-1];
    }


    // Reset on new day or beginning of chart
    if (sc.IsNewTradingDay(prevIx) || prevIx <= 0) {
        CumulativeFVDiffPos[prevIx] = 0;
        CumulativeFVDiffNeg[prevIx] = 0;
        FVDiffIncrement[prevIx] = 0;
        FVHDiffCum[prevIx] = 0;
        FVLDiffCum[prevIx] = 0;
        ExtensionID[prevIx] = 0;
        currentType = 0;
    }

    // Reset on jump condition
    if (ResetOnJump.GetInt() == 1 && (abs(FH[prevIx] - FH[prevIx-1]) >= TicksForJump.GetFloat() || abs(FL[prevIx] - FL[prevIx-1]) >= TicksForJump.GetFloat())) {
        CumulativeFVDiffPos[prevIx] = 0;
        FVDiffIncrement[prevIx] = 0;
        CumulativeFVDiffNeg[prevIx] = 0;
        currentType = 0;
    }

    CurrentStatus[prevIx] = currentType;
    ExtensionSymmetricPriceFH[prevIx] = FH[prevIx] - FVLDiffCum[prevIx];
    ExtensionSymmetricPriceFL[prevIx] = FL[prevIx] - FVHDiffCum[prevIx];

}

SCSFExport scsf_BlindExtensionsTrader(SCStudyInterfaceRef sc) {
    SCInputRef CompressionZones = sc.Input[0];
    SCInputRef VVA = sc.Input[1];
    SCInputRef MaxTicksFromFV = sc.Input[2];
    SCInputRef StopTicksUnderFV = sc.Input[3];
    SCInputRef TargetTicks = sc.Input[4];
    SCInputRef NumRetests = sc.Input[5];
    SCInputRef UseSymmetricBounds = sc.Input[6];
    SCInputRef StartTradingTime = sc.Input[7];
    SCInputRef EndTradingTime = sc.Input[8];
    SCInputRef MaxPLTick = sc.Input[9];
    SCInputRef MinPLTick = sc.Input[10];
    SCInputRef Enabled = sc.Input[11];

    SCSubgraphRef ParentOrderID = sc.Subgraph[0];
    SCSubgraphRef BuyPrice = sc.Subgraph[1];
    SCSubgraphRef SellPrice = sc.Subgraph[2];
    SCSubgraphRef RemainingRetests = sc.Subgraph[3];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.MaintainTradeStatisticsAndTradesData = true;
        sc.MaximumPositionAllowed = 2;
        sc.AllowOnlyOneTradePerBar = true;
        sc.UseGUIAttachedOrderSetting = 1;
        sc.GraphName = "Blind extensions trader";

        // Inputs
        CompressionZones.Name = "Compression zones study";
        CompressionZones.SetStudyID(1);

        VVA.Name = "VVA Study";
        VVA.SetStudyID(1);

        MaxTicksFromFV.Name = "Max ticks from FV";
        MaxTicksFromFV.SetIntLimits(0, 15);
        MaxTicksFromFV.SetInt(5);

        StopTicksUnderFV.Name = "Stop ticks under FV";
        StopTicksUnderFV.SetIntLimits(5,100);
        StopTicksUnderFV.SetInt(10);

        TargetTicks.Name = "Target ticks";
        TargetTicks.SetIntLimits(1,100);
        TargetTicks.SetInt(20);

        UseSymmetricBounds.Name = "Use symmetric bounds or real ones";
        UseSymmetricBounds.SetYesNo(0);

        NumRetests.Name = "Number of retests";
        NumRetests.SetIntLimits(1,5);
        NumRetests.SetInt(1);

        StartTradingTime.Name = "Start trading time";
        StartTradingTime.SetTime(HMS_TIME(0,0,1));

        EndTradingTime.Name = "Start trading time";
        EndTradingTime.SetTime(HMS_TIME(16,55,0));

        MaxPLTick.Name = "Max daily P&L in Ticks";
        MaxPLTick.SetIntLimits(20, 200);
        MaxPLTick.SetInt(20);

        MinPLTick.Name = "Min daily P&L in Ticks";
        MinPLTick.SetIntLimits(20, 200);
        MinPLTick.SetInt(20);

        Enabled.Name = "Enable blind trader";
        Enabled.SetYesNo(0);

        // Outputs
        ParentOrderID.Name = "Parent order ID";
        ParentOrderID.DrawStyle = DRAWSTYLE_IGNORE;

        SellPrice.Name = "Buy Price";
        SellPrice.DrawStyle = DRAWSTYLE_IGNORE;

        BuyPrice.Name = "Sell Price";
        BuyPrice.DrawStyle = DRAWSTYLE_IGNORE;

        RemainingRetests.Name = "Remaining Retests";
        RemainingRetests.DrawStyle = DRAWSTYLE_IGNORE;

    }
    if (Enabled.GetInt() == 0) {
        return;
    }

    const int prevIx = sc.Index - 1;
    const int currIx = sc.Index;

    if (sc.IsFullRecalculation) { // Not doing anything during recalculation
        return;
    }

    float entryPrice = 0;
    uint32_t parentOrderId = 0;
    BuySellEnum direction = BSE_UNDEFINED;
    SCString Buffer;

    // Retrieving Studiesp
    SCFloatArray ZoneID;
    SCFloatArray SymH;
    SCFloatArray SymL;
    SCFloatArray Status;
    SCFloatArray ZoneStartIx;


    sc.GetStudyArrayUsingID(CompressionZones.GetStudyID(), 0, ZoneID);
    sc.GetStudyArrayUsingID(CompressionZones.GetStudyID(), 1, SymH);
    sc.GetStudyArrayUsingID(CompressionZones.GetStudyID(), 2, SymL);
    sc.GetStudyArrayUsingID(CompressionZones.GetStudyID(), 3, Status);
    sc.GetStudyArrayUsingID(CompressionZones.GetStudyID(), 4, ZoneStartIx);

    SCFloatArray FH;
    SCFloatArray FL;
    sc.GetStudyArrayUsingID(VVA.GetStudyID(), 1, FH);
    sc.GetStudyArrayUsingID(VVA.GetStudyID(), 2, FL);

    const SCFloatArray& H = UseSymmetricBounds.GetInt() == 0 ? FH : SymH;
    const SCFloatArray& L = UseSymmetricBounds.GetInt() == 0 ? FL : SymL;

    int& numRetests = sc.GetPersistentInt(1);

    // SCFloatArray& RemainingRetests = ParentOrderID.Arrays[0];

    // Resets and initial conditions
    const bool outOfTradingHours = sc.BaseDateTimeIn[currIx].GetTime() > EndTradingTime.GetTime() && sc.BaseDateTimeIn[currIx].GetTime() < StartTradingTime.GetTime();
    if (sc.IsNewTradingDay(currIx) || currIx <= 0 || outOfTradingHours) {
        BuyPrice[currIx] = 0;
        SellPrice[currIx] = 0;
        sc.FlattenAndCancelAllOrders();
        ParentOrderID[currIx] = 0;
        numRetests = 0;
        return;
    }

    // CEZ study is not computed yet: do not do anything
    if (ZoneID[prevIx] == 0 || SymH[prevIx] == 0 || SymL[prevIx] == 0 || Status[prevIx] == 0 || ZoneStartIx[prevIx] == 0) {
        return;;
    }

    // By default, all outputs stay constant. Based on certain conditions, outputs will change.
    SellPrice[currIx] = SellPrice[prevIx];
    BuyPrice[currIx] = BuyPrice[prevIx];
    ParentOrderID[currIx] = ParentOrderID[prevIx];

    // Change of Zone reset
    // Condition ZoneID[prevIx] !=0 is there in case the input CEZ study is not computed yet (will have 0 as default value)
    if (ZoneID[prevIx] != ZoneID[prevIx-1]) {
        numRetests = NumRetests.GetInt();
    }

    // No trading conditions
    const bool zoneTooSmall = currIx - ZoneStartIx[prevIx] < 3;
    if (Status[prevIx] == 0 || ZoneID[prevIx] == 0 || zoneTooSmall) {
        return;
    }

    // The plan: what we want to do (sell/buy and at which price)
    if (sc.High[prevIx] < L[prevIx] && Status[prevIx] == 1) {
        entryPrice = L[prevIx];
        direction = BSE_SELL;
        SellPrice[currIx] = entryPrice;
        BuyPrice[currIx] = 0;
    }
    if (sc.Low[prevIx] > H[prevIx] && Status[prevIx] == 1) {
        entryPrice = H[prevIx];
        SellPrice[currIx] = 0;
        direction = BSE_BUY;
        BuyPrice[currIx] = entryPrice;
    }

    // Executing the plan
    // If we have no executed orders AND that the retest is 0: flatten all and pass)


    const int workingParent = workingParentOrder(sc, parentOrderId);

    if (numRetests <= 0 && isInsideTrade(sc) == 0) {
        if (workingParent == 1) {
            sc.FlattenAndCancelAllOrders();
        }
        return;
    }

    if (s_SCTradeOrder parentOrder; workingParent == 1) {
        int success = sc.GetOrderByOrderID(parentOrderId, parentOrder);
        if (parentOrder.BuySell == direction && success == 1) {
            if (entryPrice != parentOrder.Price1) {
                modiFyParentOrder(sc, parentOrder.InternalOrderID, entryPrice);
            }
        } else if (parentOrder.BuySell == BuySellInverse(direction)){
            sc.FlattenAndCancelAllOrders();
            Buffer.Format("Current working order %d has opposite direction from signal. Flattening and re-creating", parentOrderId);
            createAndSubmitOrder(sc, entryPrice, direction);
        } else {
            Buffer.Format("Could not retrieve order from ID %d. Flattening all positions", parentOrderId);
            sc.AddMessageToLog(Buffer, 1);
            sc.FlattenAndCancelAllOrders();
        }
    } else {
        createAndSubmitOrder(sc, entryPrice, direction);
    }

    // Updating the number of retests
    if (entryPrice <= sc.High[prevIx] && entryPrice >= sc.Low[prevIx] && Status[prevIx] == 1) {
        numRetests--;
    }

    RemainingRetests[currIx] = static_cast<float>(numRetests);
}