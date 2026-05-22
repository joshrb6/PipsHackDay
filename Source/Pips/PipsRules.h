#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PipsTypes.h"
#include "PipsRules.generated.h"

UENUM(BlueprintType)
enum class EPipsResult : uint8
{
	NotFinished,
	Invalid,
	Solved,
};

UCLASS()
class PIPS_API UPipsRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static EPipsResult EvaluatePuzzle(const FPipsPuzzle& Puzzle, const TMap<FIntPoint, int32>& CellValues);
};