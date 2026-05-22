#include "PipsGameMode.h"
#include "PipsPlayerController.h"
#include "GameFramework/DefaultPawn.h"
#include "PipsGameplayHUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

APipsGameMode::APipsGameMode()
{
	PlayerControllerClass = APipsPlayerController::StaticClass();
	// No pawn needed — we view through a CameraActor, not a pawn.
	DefaultPawnClass = nullptr;
}

void APipsGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!HUDWidgetClass) return;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (UUserWidget* HUD = CreateWidget(PC, HUDWidgetClass))
		{
			HUD->AddToViewport();
		}
	}
}