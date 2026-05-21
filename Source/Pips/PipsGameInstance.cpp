// Fill out your copyright notice in the Description page of Project Settings.


#include "PipsGameInstance.h"

#include "PipsJsonParser.h"

// PipsGameInstance.cpp (sketch)
void UPipsGameInstance::EnsureDailyLoaded(const FString& Date, TFunction<void(bool)> OnDone)
{
	if (History.Contains(Date))
	{
		OnDone(true);
		return;
	}

	if (bDebugMode)
	{
		const FString FilePath = FPaths::ProjectContentDir() / TEXT("PipsDebugData") / (Date + TEXT(".json"));
		UE_LOG(LogTemp, Display, TEXT("Pips: looking for %s, exists=%d"), 
			*FilePath, IFileManager::Get().FileExists(*FilePath));
		
		FString Raw;
		if (!FFileHelper::LoadFileToString(Raw, *FilePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Pips debug: no local file for %s at %s"), *Date, *FilePath);
			OnDone(false);
			return;
		}

		FPipsDailyData Parsed;
		if (UPipsJsonParser::ParseDaily(Raw, Parsed))   // your boundary parser
		{
			History.Add(Date, Parsed);
			OnDone(true);
		}
		else
		{
			OnDone(false);
		}
	}
	else
	{
		// Production: call UPipsApiClient::FetchPuzzleForDate, parse, add to History, callback.
		// Not implemented yet.
		OnDone(false);
	}
}

void UPipsGameInstance::Init()
{
	Super::Init();

	EnsureDailyLoaded(TEXT("2026-05-21"), [this](bool bOk)
	{
		if (bOk)
		{
			const FPipsDailyData* D = FindDaily(TEXT("2026-05-21"));
			UE_LOG(LogTemp, Display, TEXT("Pips loaded: editor=%s easy dominoes=%d hard regions=%d"),
				*D->Editor, D->Easy.Dominoes.Num(), D->Hard.Regions.Num());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Pips: failed to load daily"));
		}
	});
}

const FPipsDailyData* UPipsGameInstance::FindDaily(const FString& Date) const
{
	return History.Find(Date);
}
