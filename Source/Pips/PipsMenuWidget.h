#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PipsTypes.h"
#include "PipsMenuWidget.generated.h"

UCLASS()
class PIPS_API UPipsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bound to the date input in the Blueprint. */
	UPROPERTY(meta = (BindWidgetOptional))
	class UEditableTextBox* DateTextBox = nullptr;

	/** Bound to the three difficulty buttons. */
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* EasyButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* MediumButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* HardButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* PlayButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StatusText = nullptr;

	/** Name of the gameplay map to open on Play. */
	UPROPERTY(EditAnywhere, Category = "Pips")
	FName GameplayMapName = TEXT("L_Gameplay");

protected:
	virtual void NativeConstruct() override;

private:
	EPipsDifficulty SelectedDifficulty = EPipsDifficulty::Easy;

	UFUNCTION() void OnEasyClicked();
	UFUNCTION() void OnMediumClicked();
	UFUNCTION() void OnHardClicked();
	UFUNCTION() void OnPlayClicked();

	void UpdateDifficultyVisuals();
};