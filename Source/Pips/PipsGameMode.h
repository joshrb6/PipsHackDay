#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PipsGameMode.generated.h"

UCLASS()
class PIPS_API APipsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APipsGameMode();

	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Pips")
	TSubclassOf<class UPipsGameplayHUDWidget> HUDWidgetClass;
};