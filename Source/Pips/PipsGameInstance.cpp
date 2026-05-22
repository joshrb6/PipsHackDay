// Fill out your copyright notice in the Description page of Project Settings.


#include "PipsGameInstance.h"

#include "PipsJsonParser.h"

#include "PipsApiClient.h"

// PipsGameInstance.cpp (sketch)
void UPipsGameInstance::EnsureDailyLoaded(const FString& Date, TFunction<void(bool)> OnDone)
{
	// Layer 1: in-memory cache
	if (History.Contains(Date))
	{
		OnDone(true);
		return;
	}

	// Layer 2: local file (offline cache / debug data)
	const FString FilePath = FPaths::ProjectContentDir() / TEXT("PipsDebugData") / (Date + TEXT(".json"));
	if (IFileManager::Get().FileExists(*FilePath))
	{
		FString Raw;
		if (FFileHelper::LoadFileToString(Raw, *FilePath))
		{
			FPipsDailyData Parsed;
			if (UPipsJsonParser::ParseDaily(Raw, Parsed))
			{
				History.Add(Date, Parsed);
				UE_LOG(LogTemp, Display, TEXT("Pips: loaded %s from local cache"), *Date);
				OnDone(true);
				return;
			}
		}
	}

	// Layer 3: network
	if (!ApiClient)
	{
		UE_LOG(LogTemp, Error, TEXT("Pips: no API client; cannot fetch %s"), *Date);
		OnDone(false);
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("Pips: fetching %s from API"), *Date);
	ApiClient->FetchPuzzleForDate(Date,
		FOnPipsApiResult::CreateLambda([this, Date, OnDone](bool bSuccess, FPipsDailyData Data)
		{
			if (!bSuccess)
			{
				OnDone(false);
				return;
			}

			History.Add(Date, Data);

			// Persist to disk for offline access next time.
			const FString Path = FPaths::ProjectContentDir() / TEXT("PipsDebugData") / (Date + TEXT(".json"));
			// Re-serialize from struct would be ideal; for now save the raw response.
			// Easier: just don't cache to disk for the moment. Could add later.

			OnDone(true);
		}));
}

void UPipsGameInstance::Init()
{
	Super::Init();
	ApiClient = NewObject<UPipsApiClient>(this, UPipsApiClient::StaticClass());
}

const FPipsDailyData* UPipsGameInstance::FindDaily(const FString& Date) const
{
	return History.Find(Date);
}
