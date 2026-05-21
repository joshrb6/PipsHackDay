#include "PipsGameMode.h"
#include "PipsPlayerController.h"
#include "GameFramework/DefaultPawn.h"

APipsGameMode::APipsGameMode()
{
	PlayerControllerClass = APipsPlayerController::StaticClass();
	// No pawn needed — we view through a CameraActor, not a pawn.
	DefaultPawnClass = nullptr;
}