#include "PipsJsonParser.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    /** Convert a JSON 2-element array [row, col] into FIntPoint(X=row, Y=col). */
    bool ParseIntPoint(const TSharedPtr<FJsonValue>& Value, FIntPoint& Out)
    {
        if (!Value.IsValid() || Value->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
        if (Arr.Num() != 2) return false;
        Out.X = static_cast<int32>(Arr[0]->AsNumber());
        Out.Y = static_cast<int32>(Arr[1]->AsNumber());
        return true;
    }

    EPipsRegionType ParseRegionType(const FString& Type)
    {
        if (Type == TEXT("empty"))    return EPipsRegionType::Empty;
        if (Type == TEXT("sum"))      return EPipsRegionType::Sum;
        if (Type == TEXT("equals"))   return EPipsRegionType::Equals;
        if (Type == TEXT("unequal"))  return EPipsRegionType::Unequal;
        if (Type == TEXT("greater"))  return EPipsRegionType::Greater;
        if (Type == TEXT("less"))     return EPipsRegionType::Less;
        UE_LOG(LogTemp, Warning, TEXT("Pips: unknown region type '%s'"), *Type);
        return EPipsRegionType::Empty;
    }

    bool ParsePuzzle(const TSharedPtr<FJsonObject>& Obj, FPipsPuzzle& Out)
    {
        if (!Obj.IsValid()) return false;

        Out.Id           = Obj->GetIntegerField(TEXT("id"));
        Out.BackendId    = Obj->GetStringField(TEXT("backendId"));
        Out.Constructors = Obj->GetStringField(TEXT("constructors"));

        // Dominoes: array of 2-element arrays.
        const TArray<TSharedPtr<FJsonValue>>* DomArr = nullptr;
        if (Obj->TryGetArrayField(TEXT("dominoes"), DomArr))
        {
            for (const TSharedPtr<FJsonValue>& V : *DomArr)
            {
                if (!V.IsValid() || V->Type != EJson::Array) continue;
                const TArray<TSharedPtr<FJsonValue>>& Pair = V->AsArray();
                if (Pair.Num() != 2) continue;
                FPipsDomino D;
                D.A = static_cast<int32>(Pair[0]->AsNumber());
                D.B = static_cast<int32>(Pair[1]->AsNumber());
                Out.Dominoes.Add(D);
            }
        }

        // Regions: array of { indices: [[r,c],...], type: "...", target?: int }
        const TArray<TSharedPtr<FJsonValue>>* RegArr = nullptr;
        if (Obj->TryGetArrayField(TEXT("regions"), RegArr))
        {
            for (const TSharedPtr<FJsonValue>& V : *RegArr)
            {
                if (!V.IsValid() || V->Type != EJson::Object) continue;
                const TSharedPtr<FJsonObject>& RegObj = V->AsObject();
                FPipsRegion R;

                R.Type = ParseRegionType(RegObj->GetStringField(TEXT("type")));

                int32 Target = -1;
                if (RegObj->TryGetNumberField(TEXT("target"), Target))
                {
                    R.Target = Target;
                }

                const TArray<TSharedPtr<FJsonValue>>* IdxArr = nullptr;
                if (RegObj->TryGetArrayField(TEXT("indices"), IdxArr))
                {
                    for (const TSharedPtr<FJsonValue>& IdxV : *IdxArr)
                    {
                        FIntPoint P;
                        if (ParseIntPoint(IdxV, P))
                        {
                            R.Cells.Add(P);
                        }
                    }
                }

                Out.Regions.Add(R);
            }
        }

        // Solution: array of 2-element arrays of 2-element arrays. [[r,c],[r,c]]
        const TArray<TSharedPtr<FJsonValue>>* SolArr = nullptr;
        if (Obj->TryGetArrayField(TEXT("solution"), SolArr))
        {
            for (const TSharedPtr<FJsonValue>& V : *SolArr)
            {
                if (!V.IsValid() || V->Type != EJson::Array) continue;
                const TArray<TSharedPtr<FJsonValue>>& Pair = V->AsArray();
                if (Pair.Num() != 2) continue;
                FPipsSolutionPlacement S;
                if (ParseIntPoint(Pair[0], S.CellA) && ParseIntPoint(Pair[1], S.CellB))
                {
                    Out.Solution.Add(S);
                }
            }
        }

        return Out.IsValid();
    }
}

bool UPipsJsonParser::ParseDaily(const FString& JsonString, FPipsDailyData& OutData)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Pips: failed to deserialize JSON"));
        return false;
    }

    Root->TryGetStringField(TEXT("printDate"), OutData.PrintDate);
    Root->TryGetStringField(TEXT("editor"),    OutData.Editor);

    const TSharedPtr<FJsonObject>* EasyObj = nullptr;
    const TSharedPtr<FJsonObject>* MediumObj = nullptr;
    const TSharedPtr<FJsonObject>* HardObj = nullptr;

    const bool bEasy   = Root->TryGetObjectField(TEXT("easy"),   EasyObj)   && ParsePuzzle(*EasyObj,   OutData.Easy);
    const bool bMedium = Root->TryGetObjectField(TEXT("medium"), MediumObj) && ParsePuzzle(*MediumObj, OutData.Medium);
    const bool bHard   = Root->TryGetObjectField(TEXT("hard"),   HardObj)   && ParsePuzzle(*HardObj,   OutData.Hard);

    if (!bEasy || !bMedium || !bHard)
    {
        UE_LOG(LogTemp, Warning, TEXT("Pips: parsed daily but some difficulties are missing (E=%d M=%d H=%d)"),
            bEasy, bMedium, bHard);
    }

    return bEasy || bMedium || bHard;
}