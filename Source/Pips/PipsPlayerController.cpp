#include "PipsPlayerController.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "PipsBoard.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"

void APipsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Find the first CameraActor in the level and view through it.
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		SetViewTargetWithBlend(*It, 0.f);
		UE_LOG(LogTemp, Display, TEXT("PipsPlayerController: view target set to %s"), *It->GetName());

		if (APipsBoard* B = GetBoard())
		{
			UE_LOG(LogTemp, Warning, TEXT("BeginPlay: subscribing to OnPuzzleSpawned"));
			B->OnPuzzleSpawned.AddUObject(this, &APipsPlayerController::FitCameraToBoard);
			// In case the board already spawned before we hooked the delegate:
			if (B->ValidCells.Num() > 0)
			{
				FitCameraToBoard();
			}
		}
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
	InputComponent->BindAction("PipsRotate",  IE_Pressed, this, &APipsPlayerController::OnRotatePressed);
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

	// Resolve any dangling domino first (unless the user is clicking *this* dangling
	// domino to pick it up — see below).
	APipsDomino* Dangling = FindDanglingDomino();
	if (Dangling && Dangling != HoveredDomino)
	{
		ResolveDangling(Dangling);
	}

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

		DraggedDomino->bDangling = false;
		DraggedDomino->bHasLastValid = false;  // picking up invalidates the "go back" anchor
		DraggedDomino->VirtualCells.Reset();  // ADD THIS

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

APipsDomino* APipsPlayerController::FindDanglingDomino() const
{
	APipsBoard* B = Board;  // already cached by GetBoard() at this point
	if (!B) return nullptr;
	for (APipsDomino* D : B->TrayDominoes)
	{
		if (D && D->bDangling) return D;
	}
	return nullptr;
}

void APipsPlayerController::ResolveDangling(APipsDomino* Domino)
{
	if (!Domino) return;
	APipsBoard* B = GetBoard();
	if (!B) return;

	if (Domino->bHasLastValid && Domino->LastValidCells.Num() == 2)
	{
		// Try to re-place at its last valid cells. Usually succeeds because nothing else has moved.
		if (B->TryPlaceDomino(Domino, Domino->LastValidCells[0], Domino->LastValidCells[1]))
		{
			return;
		}
	}
	// Fallback: return to tray.
	ReturnToTray(Domino);
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
	Domino->bDangling = false;          // ADD THIS
	Domino->VirtualCells.Reset();       // AND THIS
	Domino->SetActorLocation(Domino->TrayLocation);
	Domino->SetActorRotation(Domino->TrayRotation);
}

void APipsPlayerController::RotatePlacedDomino(APipsDomino* Domino)
{
	if (!Domino) return;
	if (!Domino->bPlaced && !Domino->bDangling) return;
	if (Domino->VirtualCells.Num() != 2) return;

	APipsBoard* B = GetBoard();
	if (!B) return;

	const FIntPoint OldA = Domino->VirtualCells[0];
	const FIntPoint OldB = Domino->VirtualCells[1];
	const FIntPoint Delta = OldB - OldA;
	const FIntPoint Rotated(Delta.Y, -Delta.X);
	const FIntPoint NewB = OldA + Rotated;

	if (B->TryPlaceDomino(Domino, OldA, NewB))
	{
		return;  // VirtualCells gets updated by TryPlaceDomino via the line above
	}

	// Invalid: enter dangling state — show at rotated pose but don't occupy.
	B->UnplaceDomino(Domino);
	Domino->bDangling = true;
	Domino->VirtualCells = { OldA, NewB };  // CRITICAL: remember where we're "showing"

	const FVector PosA = B->CellToWorld(OldA);
	const FVector PosNewB = B->CellToWorld(NewB);
	const FVector Center = (PosA + PosNewB) * 0.5f;
	const FVector Direction = (PosNewB - PosA).GetSafeNormal();
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));

	Domino->SetActorLocation(FVector(Center.X, Center.Y, B->GetPlacementZ()));
	Domino->SetActorRotation(FRotator(0.f, Yaw, 0.f));
}

void APipsPlayerController::OnRotatePressed()
{
	APipsDomino* Dangling = FindDanglingDomino();
	if (Dangling && Dangling != HoveredDomino && Dangling != DraggedDomino)
	{
		ResolveDangling(Dangling);
	}
	
	if (DraggedDomino)
	{
		// In-hand rotation: just spin it 90° around Z, no validation needed.
		const FRotator Current = DraggedDomino->GetActorRotation();
		DraggedDomino->SetActorRotation(Current + FRotator(0.f, 90.f, 0.f));
		return;
	}

	if (HoveredDomino && (HoveredDomino->bPlaced || HoveredDomino->bDangling))
	{
		RotatePlacedDomino(HoveredDomino);
	}
}

void APipsPlayerController::FitCameraToBoard()
{
    UE_LOG(LogTemp, Warning, TEXT("FitCameraToBoard: called"));
    APipsBoard* B = GetBoard();
    if (!B) return;

    ACameraActor* Cam = nullptr;
    for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It) { Cam = *It; break; }
    if (!Cam) return;

    const FBox Bounds = B->GetVisibleBounds();
    if (!Bounds.IsValid) return;

    const FVector Center = Bounds.GetCenter();
    const FVector Extents = Bounds.GetExtent();

    UCameraComponent* CamComp = Cam->GetCameraComponent();
    const float FOV = CamComp ? CamComp->FieldOfView : 90.f;

    float AspectRatio = 16.f / 9.f;
    int32 ViewportSizeX = 0, ViewportSizeY = 0;
    GetViewportSize(ViewportSizeX, ViewportSizeY);
    if (ViewportSizeY > 0)
    {
        AspectRatio = static_cast<float>(ViewportSizeX) / static_cast<float>(ViewportSizeY);
    }

    // Camera setup: pitched down, no yaw.
    const float PitchDeg = -75.f;
    const float PitchRad = FMath::DegreesToRadians(PitchDeg);

    // Project the AABB onto the camera's screen plane.
    // For pitch P (negative = down), the camera forward is (cos|P|, 0, -sin|P|).
    // Camera-right is world Y (no roll/yaw). Camera-up is (sin|P|, 0, cos|P|).
    //
    // For an AABB with half-extents (Ex, Ey, Ez):
    //   horizontal screen half-extent = Ey
    //   vertical   screen half-extent = Ex * sin|P| + Ez * cos|P|
    const float AbsPitchRad = FMath::Abs(PitchRad);
    const float HalfH = Extents.Y;
    const float HalfV = Extents.X * FMath::Sin(AbsPitchRad) + Extents.Z * FMath::Cos(AbsPitchRad);

    // FOV is horizontal in Unreal by default.
    const float HFovRad = FMath::DegreesToRadians(FOV) * 0.5f;
    const float VFovRad = FMath::Atan(FMath::Tan(HFovRad) / AspectRatio);

    // Distance required to fit each axis.
    const float DistForH = HalfH / FMath::Tan(HFovRad);
    const float DistForV = HalfV / FMath::Tan(VFovRad);
    const float Distance = FMath::Max(DistForH, DistForV) * 1.05f; // 5% cushion

    // Camera position along the look direction from the center.
    const FVector LookDir(FMath::Cos(AbsPitchRad), 0.f, -FMath::Sin(AbsPitchRad));
    const FVector CamPos = Center - LookDir * Distance;

    Cam->SetActorLocation(CamPos);

    FRotator CamRot;
    CamRot.Pitch = PitchDeg;
    CamRot.Yaw = 0.f;
    CamRot.Roll = 0.f;
    Cam->SetActorRotation(CamRot);

    UE_LOG(LogTemp, Display, TEXT("FitCameraToBoard: center=%s extent=%s halfH=%.0f halfV=%.0f distance=%.0f camPos=%s"),
        *Center.ToString(), *Extents.ToString(), HalfH, HalfV, Distance, *CamPos.ToString());
}