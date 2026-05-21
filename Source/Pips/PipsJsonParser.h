#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PipsTypes.h"
#include "PipsJsonParser.generated.h"

UCLASS()
class PIPS_API UPipsJsonParser : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Parses the full daily JSON payload (printDate + easy/medium/hard) into FPipsDailyData. */
	UFUNCTION(BlueprintCallable, Category = "Pips|Json")
	static bool ParseDaily(const FString& JsonString, FPipsDailyData& OutData);
};