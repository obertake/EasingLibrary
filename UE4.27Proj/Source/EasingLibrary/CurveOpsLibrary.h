#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Curves/CurveFloat.h"
#include "CurveOpsLibrary.generated.h"

UCLASS()
class EASINGLIBRARY_API UCurveOpsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Duplicates SourceCurve into the same folder with Suffix and inverts it.
        If bInvertValue: v' = 1 - v. If bInvertTime: t' mirrored around [tmin,tmax]. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "CurveOps")
    static UCurveFloat* DuplicateAndInvertCurve(UCurveFloat* SourceCurve, bool bInvertValue, bool bInvertTime, const FString& Suffix = TEXT("_Inverted"));

    /** Creates EaseIn, EaseOut, EaseInOut, and EaseOutIn variants next to the supplied ease-in curve asset. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "CurveOps")
    static bool CreateEaseVariantsFromEaseIn(
        UCurveFloat* EaseInCurve,
        UPARAM(ref) UCurveFloat*& OutEaseInCurve,
        UPARAM(ref) UCurveFloat*& OutEaseOutCurve,
        UPARAM(ref) UCurveFloat*& OutEaseInOutCurve,
        UPARAM(ref) UCurveFloat*& OutEaseOutInCurve,
        const FString& EaseInSuffix = TEXT("_In"),
        const FString& EaseOutSuffix = TEXT("_Out"),
        const FString& EaseInOutSuffix = TEXT("_InOut"),
        const FString& EaseOutInSuffix = TEXT("_OutIn"));
};
