#include "sierrachart.h"

std::map<float, int> retestsMap;

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
