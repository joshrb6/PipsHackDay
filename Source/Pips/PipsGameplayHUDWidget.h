// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PipsGameplayHUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class PIPS_API UPipsGameplayHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* BackButton = nullptr;

	UPROPERTY(EditAnywhere, Category = "Pips")
	FName MenuMapName = TEXT("L_Menu");

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION() void OnBackClicked();
};
