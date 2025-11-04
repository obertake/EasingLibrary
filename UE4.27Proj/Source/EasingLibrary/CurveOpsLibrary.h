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
};
