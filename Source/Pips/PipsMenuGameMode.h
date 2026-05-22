#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PipsMenuGameMode.generated.h"

UCLASS()
class PIPS_API APipsMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APipsMenuGameMode();

	/** Menu widget class — set in Blueprint subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "Pips")
	TSubclassOf<class UPipsMenuWidget> MenuWidgetClass;

protected:
	virtual void BeginPlay() override;
};