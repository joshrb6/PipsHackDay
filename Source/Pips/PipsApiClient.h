#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PipsTypes.h"
#include "PipsApiClient.generated.h"

DECLARE_DELEGATE_TwoParams(FOnPipsApiResult, bool /*bSuccess*/, FPipsDailyData /*Data*/);

UCLASS()
class PIPS_API UPipsApiClient : public UObject
{
	GENERATED_BODY()

public:
	/** Bearer token loaded from DefaultGame.ini. */
	UPROPERTY()
	FString ApiBearerToken;

	/** Base URL — overridable via config if the endpoint changes. */
	UPROPERTY()
	FString BaseUrl = TEXT("https://jindrich-bar--nyt-games-api.apify.actor/pips");

	UPipsApiClient();

	/** Fetches the puzzle for a given date. Callback fires on completion (success or failure). */
	void FetchPuzzleForDate(const FString& Date, FOnPipsApiResult Callback);
};