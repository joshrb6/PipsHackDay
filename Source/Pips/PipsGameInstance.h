// PipsGameInstance.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PipsTypes.h"
#include "PipsGameInstance.generated.h"

UCLASS()
class PIPS_API UPipsGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Cache of all loaded daily puzzles, keyed by ISO date string. */
	UPROPERTY(BlueprintReadOnly, Category = "Pips")
	TMap<FString, FPipsDailyData> History;

	/** Currently selected date and difficulty (set by menu, read by gameplay level). */
	UPROPERTY(BlueprintReadWrite, Category = "Pips")
	FString SelectedDate;

	UPROPERTY(BlueprintReadWrite, Category = "Pips")
	EPipsDifficulty SelectedDifficulty = EPipsDifficulty::Easy;

	/** True when we should bypass the network and load from embedded/local JSON. */
	UPROPERTY(BlueprintReadOnly, Category = "Pips")
	bool bDebugMode = true;

	virtual void Init() override;

	/** Returns cached data, or nullptr if not yet loaded. */
	const FPipsDailyData* FindDaily(const FString& Date) const;

	/** Async: ensures the given date is loaded (debug = from disk, prod = from API). */
	void EnsureDailyLoaded(const FString& Date, TFunction<void(bool /*bSuccess*/)> OnDone);
};