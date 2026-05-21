#include "PipsPlayerController.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void APipsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Find the first CameraActor in the level and view through it.
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		SetViewTargetWithBlend(*It, 0.f);
		UE_LOG(LogTemp, Display, TEXT("PipsPlayerController: view target set to %s"), *It->GetName());
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("PipsPlayerController: no CameraActor found in level"));
}