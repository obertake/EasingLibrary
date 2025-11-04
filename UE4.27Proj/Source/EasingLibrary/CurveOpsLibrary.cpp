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
        float ArriveTangent = K.ArriveTangent;
        float LeaveTangent = K.LeaveTangent;

        // Vertical flip reverses the slope (multiply by -1)
        if (bInvertValue)
        {
            ArriveTangent = -ArriveTangent;
            LeaveTangent = -LeaveTangent;
        }

        // Horizontal flip swaps them (the "left" side becomes the "right")
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
