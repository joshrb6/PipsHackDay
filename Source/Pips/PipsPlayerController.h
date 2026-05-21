#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PipsPlayerController.generated.h"

UCLASS()
class PIPS_API APipsPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};