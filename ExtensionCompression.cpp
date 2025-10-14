#include "helpers.h"
#include "sierrachart.h"


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
};


template<typename... Subgraphs>
void ResetSubgraphsAtIndex(int index, Subgraphs&... sgs)
{
    // Fold expression: expands the parameter pack and applies the reset
    ((sgs[index] = 0.0f), ...);
}

SCSFExport scsf_CompressionExtensionSignal(SCStudyInterfaceRef sc) {
    SCInputRef StudyLines = sc.Input[0];
    SCInputRef MinNumberOfExtensionCompressionTicks = sc.Input[1];
    SCInputRef TicksForJump = sc.Input[2];
    SCInputRef TicksForRetrace = sc.Input[3];
    SCInputRef ResetOnJump = sc.Input[4];

    SCSubgraphRef ExtensionFlag = sc.Subgraph[0];
    SCSubgraphRef ExtensionStartIndex = sc.Subgraph[1];
    SCSubgraphRef ExtensionSymmetricPriceFH = sc.Subgraph[2];
    SCSubgraphRef ExtensionSymmetricPriceFL = sc.Subgraph[3];

    SCSubgraphRef CompressionFlag = sc.Subgraph[4];
    SCSubgraphRef CompressionStartIndex = sc.Subgraph[5];
    SCSubgraphRef CompressionSymmetricPriceFH = sc.Subgraph[6];
    SCSubgraphRef CompressionSymmetricPriceFL = sc.Subgraph[7];

    SCSubgraphRef Midpoint = sc.Subgraph[8];

    SCSubgraphRef FVDiff = sc.Subgraph[9];
    SCSubgraphRef FVDiffIncrement = sc.Subgraph[10];
    SCSubgraphRef CumulativeFVDiffPos = sc.Subgraph[11];
    SCSubgraphRef CumulativeFVDiffNeg = sc.Subgraph[12];


    SCSubgraphRef CumulativeFVRetraceTicks = sc.Subgraph[13];
    SCSubgraphRef CurrentStatus = sc.Subgraph[14];



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

        TicksForRetrace.Name = "Ticks for retrace";
        TicksForRetrace.SetIntLimits(1,100);
        TicksForRetrace.SetInt(1);

        ResetOnJump.Name = "Reset extension compression on jump";
        ResetOnJump.SetYesNo(1);

        // Outputs
        ExtensionFlag.Name = "Extension flag";
        ExtensionFlag.DrawStyle = DRAWSTYLE_IGNORE;
        ExtensionStartIndex.Name = "Extension start index";
        ExtensionStartIndex.DrawStyle = DRAWSTYLE_IGNORE;
        ExtensionSymmetricPriceFH.Name = "Extension sym price high";
        ExtensionSymmetricPriceFH.DrawStyle = DRAWSTYLE_IGNORE;
        ExtensionSymmetricPriceFL.Name = "Extension sym price low";
        ExtensionSymmetricPriceFL.DrawStyle = DRAWSTYLE_IGNORE;
        CompressionFlag.Name = "Compression flag";
        CompressionFlag.DrawStyle = DRAWSTYLE_IGNORE;
        CompressionStartIndex.Name = "Compression start index";
        CompressionStartIndex.DrawStyle = DRAWSTYLE_IGNORE;
        CompressionSymmetricPriceFH.Name = "Compression sum price high";
        CompressionSymmetricPriceFH.DrawStyle = DRAWSTYLE_IGNORE;
        CompressionSymmetricPriceFL.Name = "Compression sum price low";
        CompressionSymmetricPriceFL.DrawStyle = DRAWSTYLE_IGNORE;

        FVDiff.Name = "FV diff";
        FVDiff.DrawStyle = DRAWSTYLE_IGNORE;
        FVDiffIncrement.Name = "FV Diff increment";
        FVDiffIncrement.DrawStyle = DRAWSTYLE_IGNORE;

        //CumulativeFHIncrementTicks.DrawStyle = DRAWSTYLE_IGNORE;
        //CumulativeFLIncrementTicks.DrawStyle = DRAWSTYLE_IGNORE;
        CumulativeFVDiffPos.Name = "Cumulative FV diff positive";
        CumulativeFVDiffPos.DrawStyle = DRAWSTYLE_IGNORE;
        CumulativeFVDiffNeg.Name = "Cumulative FV diff negative";
        CumulativeFVDiffNeg.DrawStyle = DRAWSTYLE_IGNORE;
        //CumulativeFVIncrementTicks.DrawStyle = DRAWSTYLE_IGNORE;

        CumulativeFVRetraceTicks.Name = "Cumulative FV retrace ticks";
        CumulativeFVRetraceTicks.DrawStyle = DRAWSTYLE_IGNORE;
        CurrentStatus.Name = "Current status";
        CurrentStatus.DrawStyle = DRAWSTYLE_IGNORE;

        Midpoint.Name = "Midpoint";
        Midpoint.DrawStyle = DRAWSTYLE_LINE;
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

    float currentType;

    // FVDiff[i] = FVDiff[i-1] - (FL[i] - FL[i-1]) / sc.TickSize;
    FVDiff[i] = (FH[i] - FL[i]) / sc.TickSize;
    FVDiffIncrement[i] = FVDiff[i] - FVDiff[i-1];

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

    // CumulativeFVDiffPos[i] = CumulativeFVDiffPos[i-1] + FVDiffIncrement[i];
    // CumulativeFVDiff[i] = CumulativeFHIncrementTicks[i] + FVDiff[i];

    if (CumulativeFVDiffPos[i] >= MinNumberOfExtensionCompressionTicks.GetFloat() && CurrentStatus[i-1] != 1) {
        currentType = 1;
        CumulativeFVDiffPos[i] = 0;
        ResetSubgraphsAtIndex(i, CumulativeFVDiffPos, FVDiffIncrement); // DO NOT RESET FVDiff !!!
    } else if (CumulativeFVDiffNeg[i] <= -MinNumberOfExtensionCompressionTicks.GetFloat() && CurrentStatus[i-1] != 2) {
        currentType = 2;
        CumulativeFVDiffPos[i] = 0;
        ResetSubgraphsAtIndex(i, CumulativeFVDiffPos, FVDiffIncrement); // DO NOT RESET FVDiff !!!

    } else {
        currentType = CurrentStatus[i-1];
    }


    // Reset on new day or Beginning of chart history

    if (sc.IsNewTradingDay(i) || i <= 0) {
        CumulativeFVDiffPos[i] = 0;
        CumulativeFVDiffNeg[i] = 0;
        FVDiffIncrement[i] = 0;
        //FVDiff[i] = 0;
        //ResetSubgraphsAtIndex(
        //    i, ExtensionFlag, ExtensionStartIndex, ExtensionSymmetricPriceFH, ExtensionSymmetricPriceFL,
        //    CompressionFlag, CompressionStartIndex, CompressionSymmetricPriceFH, CompressionSymmetricPriceFL,
        //    CumulativeFVDiff, FVDiffIncrement, FVDiff, CumulativeFVRetraceTicks);
        currentType = 0;
    }

    // Reset on jump condition
    if (ResetOnJump.GetInt() == 1 && (abs(FH[i] - FH[i-1]) >= TicksForJump.GetFloat() || abs(FL[i] - FL[i-1]) >= TicksForJump.GetFloat())) {
        CumulativeFVDiffPos[i] = 0;
        FVDiffIncrement[i] = 0;
        CumulativeFVDiffNeg[i] = 0;
//
        //FVDiff[i] = 0;
//
        currentType = 0;
    }

    CurrentStatus[i] = currentType;
    Midpoint[i] = (FH[i] + FL[i]) / 2;
}
