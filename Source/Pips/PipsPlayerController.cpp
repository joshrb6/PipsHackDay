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

#include "PipsDomino.h"
#include "DrawDebugHelpers.h"

APipsPlayerController::APipsPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
	PrimaryActorTick.bCanEverTick = true;
}

void APipsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindAction("PipsPrimary", IE_Pressed, this, &APipsPlayerController::OnPrimaryPressed);
	//InputComponent->BindAction("PipsPrimary", IE_Released, this, &APipsPlayerController::OnPrimaryReleased);
}

void APipsPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (DraggedDomino)
	{
		FVector PlanePos;
		if (GetCursorPositionOnDragPlane(PlanePos))
		{
			DraggedDomino->SetActorLocation(FVector(PlanePos.X, PlanePos.Y, DragHeight));
		}
	}
	else
	{
		UpdateHover();
	}
}

void APipsPlayerController::OnPrimaryPressed()
{
	if (DraggedDomino)
	{
		// Currently carrying — drop at current position. Slice C: snap to cells.
		DraggedDomino->SetActorLocation(DragOriginalLocation);
		DraggedDomino = nullptr;
		return;
	}

	if (HoveredDomino)
	{
		DraggedDomino = HoveredDomino;
		DragOriginalLocation = DraggedDomino->GetActorLocation();

		FVector PlanePos;
		if (GetCursorPositionOnDragPlane(PlanePos))
		{
			DraggedDomino->SetActorLocation(FVector(PlanePos.X, PlanePos.Y, DragHeight));
		}
	}
}

void APipsPlayerController::OnPrimaryReleased()
{
	if (!DraggedDomino) return;

	// Slice B: just return to where it was. Slice C will add snap.
	DraggedDomino->SetActorLocation(DragOriginalLocation);
	DraggedDomino = nullptr;
}

bool APipsPlayerController::GetCursorPositionOnDragPlane(FVector& OutWorld) const
{
	FVector WorldOrigin, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}
	// Intersect ray with Z = DragHeight plane.
	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}
	const float T = (DragHeight - WorldOrigin.Z) / WorldDirection.Z;
	if (T <= 0.f) return false;
	OutWorld = WorldOrigin + WorldDirection * T;
	OutWorld.Z = DragHeight;
	return true;
}

void APipsPlayerController::UpdateHover()
{
	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/false, Hit))
	{
		if (HoveredDomino)
		{
			HoveredDomino = nullptr;
			DefaultMouseCursor = EMouseCursor::Default;
			CurrentMouseCursor = EMouseCursor::Default;
		}
		return;
	}

	AActor* HitActor = Hit.GetActor();
	APipsDomino* Domino = Cast<APipsDomino>(HitActor);

	if (Domino != HoveredDomino)
	{
		HoveredDomino = Domino;
		CurrentMouseCursor = Domino ? EMouseCursor::GrabHand : EMouseCursor::Default;
	}
}