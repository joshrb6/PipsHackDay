#include "PipsPlayerController.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "PipsBoard.h"
#include "EngineUtils.h"

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
	APipsBoard* B = GetBoard();
	if (!B) return;

	if (DraggedDomino)
	{
		// Currently carrying — try to place.
		FIntPoint A, BCell;
		if (ComputeSnapCells(DraggedDomino, A, BCell)
			&& B->TryPlaceDomino(DraggedDomino, A, BCell))
		{
			// Placed successfully.
		}
		else
		{
			ReturnToTray(DraggedDomino);
		}
		DraggedDomino = nullptr;
		return;
	}

	if (HoveredDomino)
	{
		DraggedDomino = HoveredDomino;
		// Remember where it came from if currently placed; otherwise tray is already remembered.
		DragOriginalLocation = DraggedDomino->GetActorLocation();

		// If it was placed, remove from board occupancy so we can move it freely.
		B->UnplaceDomino(DraggedDomino);

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

APipsBoard* APipsPlayerController::GetBoard()
{
	if (Board) return Board;
	for (TActorIterator<APipsBoard> It(GetWorld()); It; ++It)
	{
		Board = *It;
		return Board;
	}
	return nullptr;
}

bool APipsPlayerController::ComputeSnapCells(APipsDomino* Domino, FIntPoint& OutCellA, FIntPoint& OutCellB) const
{
	if (!Domino || !Board) return false;

	// World positions of each half. The halves sit at ±HalfLength*0.5 along the domino's local +X.
	const float HalfOffset = Board->CellSize * 0.5f;
	const FTransform DT = Domino->GetActorTransform();
	const FVector HalfAWorld = DT.TransformPosition(FVector(-HalfOffset, 0, 0));
	const FVector HalfBWorld = DT.TransformPosition(FVector( HalfOffset, 0, 0));

	return Board->FindNearestCell(HalfAWorld, OutCellA)
		&& Board->FindNearestCell(HalfBWorld, OutCellB);
}

void APipsPlayerController::ReturnToTray(APipsDomino* Domino)
{
	if (!Domino) return;
	GetBoard()->UnplaceDomino(Domino);
	Domino->SetActorLocation(Domino->TrayLocation);
	Domino->SetActorRotation(Domino->TrayRotation);
}