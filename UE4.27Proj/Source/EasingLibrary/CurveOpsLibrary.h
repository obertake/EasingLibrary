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

    /** Creates EaseOut, EaseInOut, and EaseOutIn variants next to the supplied ease-in curve asset. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "CurveOps")
    static bool CreateEaseVariantsFromEaseIn(
        UCurveFloat* EaseInCurve,
        UPARAM(ref) UCurveFloat*& OutEaseOutCurve,
        UPARAM(ref) UCurveFloat*& OutEaseInOutCurve,
        UPARAM(ref) UCurveFloat*& OutEaseOutInCurve,
        const FString& EaseOutSuffix = TEXT("_EaseOut"),
        const FString& EaseInOutSuffix = TEXT("_EaseInOut"),
        const FString& EaseOutInSuffix = TEXT("_EaseOutIn"));
};
