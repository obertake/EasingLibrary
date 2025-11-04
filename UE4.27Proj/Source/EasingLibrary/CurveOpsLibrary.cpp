#include "CurveOpsLibrary.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

UCurveFloat* UCurveOpsLibrary::DuplicateAndInvertCurve(
    UCurveFloat* SourceCurve, bool bInvertValue, bool bInvertTime, const FString& Suffix)
{
    if (!SourceCurve) return nullptr;

    const FString SrcPkgName = SourceCurve->GetOutermost()->GetName();
    const FString SrcPath = FPackageName::GetLongPackagePath(SrcPkgName);
    const FString NewName = SourceCurve->GetName() + Suffix;

    // Duplicate beside the source
    FAssetToolsModule& ATM = FAssetToolsModule::GetModule();
    UObject* NewObj = ATM.Get().DuplicateAsset(NewName, SrcPath, SourceCurve);
    UCurveFloat* NewCurve = Cast<UCurveFloat>(NewObj);
    if (!NewCurve) return nullptr;

    const FRichCurve& Src = SourceCurve->FloatCurve;
    const int32 NumKeys = Src.GetNumKeys();
    if (NumKeys == 0)
    {
        FAssetRegistryModule::AssetCreated(NewCurve);
        NewCurve->MarkPackageDirty();
        return NewCurve;
    }

    float MinTime, MaxTime;
    SourceCurve->GetTimeRange(MinTime, MaxTime);

    FRichCurve& Dst = NewCurve->FloatCurve;
    Dst.Reset();

    // Copy keys with transforms using UE4.27 API (handles + setters)
    for (auto It = Src.GetConstRefOfKeys().CreateConstIterator(); It; ++It)
    {
        const FRichCurveKey& K = *It;

        float T = K.Time;
        float V = K.Value;

        if (bInvertTime)
        {
            const float Range = (MaxTime - MinTime);
            if (Range > KINDA_SMALL_NUMBER)
            {
                T = (MaxTime - (T - MinTime));
            }
        }
        if (bInvertValue)
        {
            V = 1.0f - V;
        }

        // Add key (UE4: AddKey takes time/value)
        const FKeyHandle Handle = Dst.AddKey(T, V);

        // Preserve interpolation & tangent modes
        Dst.SetKeyInterpMode(Handle, K.InterpMode);
        Dst.SetKeyTangentMode(Handle, K.TangentMode);

        // Compute flipped tangents
        // Adjust tangents using the derivative transform: dV'/dT' = (ValueScale / TimeScale) * dV/dT
        float TangentScale = 1.0f;
        if (bInvertValue)
        {
            TangentScale *= -1.0f;
        }
        if (bInvertTime)
        {
            TangentScale *= -1.0f;
        }

        float ArriveTangent = K.ArriveTangent * TangentScale;
        float LeaveTangent = K.LeaveTangent * TangentScale;

        // Horizontal flip swaps them so the "left" tangent becomes the "right"
        if (bInvertTime)
        {
            Swap(ArriveTangent, LeaveTangent);
        }

        // ---- UE4.27: mutate the key struct directly ----
        FRichCurveKey& OutKey = Dst.GetKey(Handle);
        OutKey.ArriveTangent = ArriveTangent;
        OutKey.LeaveTangent = LeaveTangent;

        // Tangent weights exist in 4.26+; guard if you need to support earlier 4.x
#if (ENGINE_MAJOR_VERSION == 4) && (ENGINE_MINOR_VERSION >= 26)
        OutKey.TangentWeightMode = K.TangentWeightMode;
        OutKey.ArriveTangentWeight = K.ArriveTangentWeight;
        OutKey.LeaveTangentWeight = K.LeaveTangentWeight;
#elif

        // Older 4.x fallback
        Dst.SetKeyArriveTangent(Handle, K.ArriveTangent);
        Dst.SetKeyLeaveTangent(Handle, K.LeaveTangent);
#endif
    }

    // Recompute auto tangents if needed
    Dst.AutoSetTangents();

    FAssetRegistryModule::AssetCreated(NewCurve);
    NewCurve->MarkPackageDirty();
    NewCurve->GetOutermost()->SetDirtyFlag(true);
    return NewCurve;
}

bool UCurveOpsLibrary::CreateEaseVariantsFromEaseIn(
    UCurveFloat* EaseInCurve,
    UCurveFloat*& OutEaseInCurve,
    UCurveFloat*& OutEaseOutCurve,
    UCurveFloat*& OutEaseInOutCurve,
    UCurveFloat*& OutEaseOutInCurve,
    const FString& EaseInSuffix,
    const FString& EaseOutSuffix,
    const FString& EaseInOutSuffix,
    const FString& EaseOutInSuffix)
{
    OutEaseInCurve = nullptr;
    OutEaseOutCurve = nullptr;
    OutEaseInOutCurve = nullptr;
    OutEaseOutInCurve = nullptr;

    if (!EaseInCurve)
    {
        return false;
    }

    float MinTime = 0.0f;
    float MaxTime = 0.0f;
    EaseInCurve->GetTimeRange(MinTime, MaxTime);
    const float Range = MaxTime - MinTime;
    if (Range <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    OutEaseOutCurve = DuplicateAndInvertCurve(EaseInCurve, /*bInvertValue*/ true, /*bInvertTime*/ true, EaseOutSuffix);
    if (!OutEaseOutCurve)
    {
        return false;
    }

    const FString SourcePackageName = EaseInCurve->GetOutermost()->GetName();
    const FString SourcePath = FPackageName::GetLongPackagePath(SourcePackageName);

    FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();

    auto DuplicateForEditing = [&](const FString& Suffix) -> UCurveFloat*
    {
        UObject* NewObject = AssetToolsModule.Get().DuplicateAsset(EaseInCurve->GetName() + Suffix, SourcePath, EaseInCurve);
        return Cast<UCurveFloat>(NewObject);
    };

    OutEaseInCurve = DuplicateForEditing(EaseInSuffix);
    if (!OutEaseInCurve)
    {
        return false;
    }
    FAssetRegistryModule::AssetCreated(OutEaseInCurve);
    OutEaseInCurve->MarkPackageDirty();
    OutEaseInCurve->GetOutermost()->SetDirtyFlag(true);

    const FRichCurve& EaseInData = EaseInCurve->FloatCurve;
    const TArray<FRichCurveKey>& EaseInKeys = EaseInData.GetConstRefOfKeys();
    const int32 NumEaseInKeys = EaseInKeys.Num();

    const FRichCurve& EaseOutData = OutEaseOutCurve->FloatCurve;
    const TArray<FRichCurveKey>& EaseOutKeys = EaseOutData.GetConstRefOfKeys();
    const int32 NumEaseOutKeys = EaseOutKeys.Num();

    float EaseOutMinTime = 0.0f;
    float EaseOutMaxTime = 0.0f;
    OutEaseOutCurve->GetTimeRange(EaseOutMinTime, EaseOutMaxTime);

    auto CopyKeyWithTransform = [](FRichCurve& DestCurve, const FRichCurveKey& SourceKey, float SourceMinTime, float TimeScale, float TimeOffset, float ValueScale, float ValueOffset) -> FKeyHandle
    {
        const float NewTime = TimeOffset + (SourceKey.Time - SourceMinTime) * TimeScale;
        const float NewValue = ValueOffset + ValueScale * SourceKey.Value;

        const FKeyHandle Handle = DestCurve.AddKey(NewTime, NewValue);
        DestCurve.SetKeyInterpMode(Handle, SourceKey.InterpMode);
        DestCurve.SetKeyTangentMode(Handle, SourceKey.TangentMode);

        FRichCurveKey& DestKey = DestCurve.GetKey(Handle);
        const float TangentScale = (FMath::Abs(TimeScale) > KINDA_SMALL_NUMBER) ? (ValueScale / TimeScale) : 0.0f;
        DestKey.ArriveTangent = SourceKey.ArriveTangent * TangentScale;
        DestKey.LeaveTangent = SourceKey.LeaveTangent * TangentScale;

#if (ENGINE_MAJOR_VERSION == 4) && (ENGINE_MINOR_VERSION >= 26)
        DestKey.TangentWeightMode = SourceKey.TangentWeightMode;
        DestKey.ArriveTangentWeight = SourceKey.ArriveTangentWeight * FMath::Abs(TimeScale);
        DestKey.LeaveTangentWeight = SourceKey.LeaveTangentWeight * FMath::Abs(TimeScale);
#endif

        return Handle;
    };

    OutEaseInOutCurve = DuplicateForEditing(EaseInOutSuffix);
    if (!OutEaseInOutCurve)
    {
        return false;
    }

    {
        FRichCurve& TargetCurve = OutEaseInOutCurve->FloatCurve;
        TargetCurve.Reset();

        if (NumEaseInKeys > 0)
        {
            for (int32 KeyIndex = 0; KeyIndex < NumEaseInKeys; ++KeyIndex)
            {
                CopyKeyWithTransform(TargetCurve, EaseInKeys[KeyIndex], MinTime, 0.5f, MinTime, 0.5f, 0.0f);
            }
        }

        for (int32 KeyIndex = 0; KeyIndex < NumEaseOutKeys; ++KeyIndex)
        {
            if (KeyIndex == 0 && TargetCurve.GetNumKeys() > 0)
            {
                continue;
            }

            CopyKeyWithTransform(TargetCurve, EaseOutKeys[KeyIndex], EaseOutMinTime, 0.5f, MinTime + Range * 0.5f, 0.5f, 0.5f);
        }

        TargetCurve.AutoSetTangents();

        FAssetRegistryModule::AssetCreated(OutEaseInOutCurve);
        OutEaseInOutCurve->MarkPackageDirty();
        OutEaseInOutCurve->GetOutermost()->SetDirtyFlag(true);
    }

    OutEaseOutInCurve = DuplicateForEditing(EaseOutInSuffix);
    if (!OutEaseOutInCurve)
    {
        return false;
    }

    {
        FRichCurve& TargetCurve = OutEaseOutInCurve->FloatCurve;
        TargetCurve.Reset();

        if (NumEaseOutKeys > 0)
        {
            for (int32 KeyIndex = 0; KeyIndex < NumEaseOutKeys; ++KeyIndex)
            {
                CopyKeyWithTransform(TargetCurve, EaseOutKeys[KeyIndex], EaseOutMinTime, 0.5f, MinTime, 0.5f, 0.0f);
            }
        }

        for (int32 KeyIndex = 0; KeyIndex < NumEaseInKeys; ++KeyIndex)
        {
            if (KeyIndex == 0 && TargetCurve.GetNumKeys() > 0)
            {
                continue;
            }

            CopyKeyWithTransform(TargetCurve, EaseInKeys[KeyIndex], MinTime, 0.5f, MinTime + Range * 0.5f, 0.5f, 0.5f);
        }

        TargetCurve.AutoSetTangents();

        FAssetRegistryModule::AssetCreated(OutEaseOutInCurve);
        OutEaseOutInCurve->MarkPackageDirty();
        OutEaseOutInCurve->GetOutermost()->SetDirtyFlag(true);
    }

    return true;
}
