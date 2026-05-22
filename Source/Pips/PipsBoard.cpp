#include "PipsBoard.h"
#include "PipsGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/TextRenderComponent.h"
#include "PipsDomino.h"


static FString GetRegionBadgeText(const FPipsRegion& Region)
{
    switch (Region.Type)
    {
    case EPipsRegionType::Sum:     return FString::FromInt(Region.Target);
    case EPipsRegionType::Equals:  return TEXT("=");
    case EPipsRegionType::Unequal: return TEXT("≠");
    case EPipsRegionType::Greater: return FString::Printf(TEXT(">%d"), Region.Target);
    case EPipsRegionType::Less:    return FString::Printf(TEXT("<%d"), Region.Target);
    case EPipsRegionType::Empty:   return FString();
    default: return FString();
    }
}

static FIntPoint GetBadgeAnchorCell(const FPipsRegion& Region)
{
    FIntPoint Anchor(-INT_MAX, -INT_MAX);
    for (const FIntPoint& Cell : Region.Cells)
    {
        if (Cell.X > Anchor.X || (Cell.X == Anchor.X && Cell.Y > Anchor.Y))
        {
            Anchor = Cell;
        }
    }
    return Anchor;
}

APipsBoard::APipsBoard()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

void APipsBoard::BeginPlay()
{
    Super::BeginPlay();

    UPipsGameInstance* GI = GetGameInstance<UPipsGameInstance>();
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("PipsBoard: no PipsGameInstance found"));
        return;
    }

    GI->EnsureDailyLoaded(DebugDate, [this, GI](bool bOk)
    {
        if (!bOk)
        {
            UE_LOG(LogTemp, Error, TEXT("PipsBoard: failed to load daily for %s"), *DebugDate);
            return;
        }
        const FPipsDailyData* Daily = GI->FindDaily(DebugDate);
        if (!Daily) return;

        const FPipsPuzzle& Puzzle = Daily->GetByDifficulty(DebugDifficulty);
        UE_LOG(LogTemp, Display, TEXT("PipsBoard: spawning %s puzzle with %d regions"),
            *DebugDate, Puzzle.Regions.Num());
        SpawnFromPuzzle(Puzzle);
    });
}

FVector APipsBoard::GridToLocal(const FIntPoint& Cell) const
{
    // Convention: Cell.X = row (grows backward in world X),
    //             Cell.Y = col (grows right in world Y).
    return FVector(-Cell.X * CellSize, Cell.Y * CellSize, 0.f);
}

void APipsBoard::ClearVisuals()
{
    for (UStaticMeshComponent* Mesh : CellMeshes)
    {
        if (IsValid(Mesh)) Mesh->DestroyComponent();
    }
    CellMeshes.Reset();

    for (UTextRenderComponent* Text : BadgeTexts)
    {
        if (IsValid(Text)) Text->DestroyComponent();
    }
    BadgeTexts.Reset();

    for (UStaticMeshComponent* Backing : BadgeBackings)
    {
        if (IsValid(Backing)) Backing->DestroyComponent();
    }
    BadgeBackings.Reset();
    
    for (APipsDomino* D : TrayDominoes)
    {
        if (IsValid(D)) D->Destroy();
    }
    TrayDominoes.Reset();
}

void APipsBoard::SpawnFromPuzzle(const FPipsPuzzle& Puzzle)
{
    ClearVisuals();

    // Find Engine's built-in cube mesh — perfect placeholder.
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("PipsBoard: could not load /Engine/BasicShapes/Cube"));
        return;
    }

    // First pass: collect cells with their region index, and compute bounding box.
    TMap<FIntPoint, int32> CellToRegion;   // cell -> region index
    for (int32 RegionIdx = 0; RegionIdx < Puzzle.Regions.Num(); ++RegionIdx)
    {
        for (const FIntPoint& Cell : Puzzle.Regions[RegionIdx].Cells)
        {
            // First region claiming a cell wins (cells shouldn't overlap in valid data).
            if (!CellToRegion.Contains(Cell))
            {
                CellToRegion.Add(Cell, RegionIdx);
            }
        }
    }

    if (CellToRegion.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PipsBoard: puzzle has no cells to spawn"));
        return;
    }

    FIntPoint MinCell( INT_MAX,  INT_MAX);
    FIntPoint MaxCell(-INT_MAX, -INT_MAX);
    for (const TPair<FIntPoint, int32>& Pair : CellToRegion)
    {
        MinCell.X = FMath::Min(MinCell.X, Pair.Key.X);
        MinCell.Y = FMath::Min(MinCell.Y, Pair.Key.Y);
        MaxCell.X = FMath::Max(MaxCell.X, Pair.Key.X);
        MaxCell.Y = FMath::Max(MaxCell.Y, Pair.Key.Y);
    }

    const float CentroidRow = (MinCell.X + MaxCell.X) * 0.5f;
    const float CentroidCol = (MinCell.Y + MaxCell.Y) * 0.5f;
    const FVector Centroid(-CentroidRow * CellSize, CentroidCol * CellSize, 0.f);

    CachedCentroid = Centroid;
    ValidCells.Reset();
    CellOccupancy.Reset();
    for (const TPair<FIntPoint, int32>& Pair : CellToRegion)
    {
        ValidCells.Add(Pair.Key);
    }

    // Second pass: spawn tiles, tinted by region color.
    for (const TPair<FIntPoint, int32>& Pair : CellToRegion)
    {
        const FIntPoint& Cell = Pair.Key;
        const int32 RegionIdx = Pair.Value;

        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
        Mesh->SetStaticMesh(CubeMesh);
        Mesh->SetupAttachment(Root);
        Mesh->RegisterComponent();

        const float TileHeight = 10.f;
        const FVector LocalPos = GridToLocal(Cell) - Centroid;
        Mesh->SetRelativeLocation(LocalPos + FVector(0, 0, TileHeight * 0.5f));
        Mesh->SetRelativeScale3D(FVector(
            CellSize / 100.f,
            CellSize / 100.f,
            TileHeight / 100.f));

        // Create a dynamic material instance and tint it.
        if (CellMaterial)
        {
            UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(CellMaterial, this);
            const EPipsRegionType RegionType = Puzzle.Regions[RegionIdx].Type;
            const FLinearColor TileColor = (RegionType == EPipsRegionType::Empty)
                ? FLinearColor(0.78f, 0.70f, 0.56f)   // muted beige
                : GetRegionColor(RegionIdx);
            MID->SetVectorParameterValue(TEXT("BaseColor"), TileColor);
            Mesh->SetMaterial(0, MID);
        }

        CellMeshes.Add(Mesh);
    }

    UE_LOG(LogTemp, Display, TEXT("PipsBoard: spawned %d cell tiles across %d regions"),
        CellMeshes.Num(), Puzzle.Regions.Num());

    // Third pass: spawn constraint badges for non-empty regions.
    for (int32 RegionIdx = 0; RegionIdx < Puzzle.Regions.Num(); ++RegionIdx)
    {
        const FPipsRegion& Region = Puzzle.Regions[RegionIdx];
        const FString BadgeText = GetRegionBadgeText(Region);
        if (BadgeText.IsEmpty()) continue;

        const FIntPoint AnchorCell = GetBadgeAnchorCell(Region);
        const FVector AnchorLocal = GridToLocal(AnchorCell) - Centroid;

        // Position badge at the bottom-right corner of the anchor cell, slightly raised.
        const FVector BadgeOffset(-CellSize * 0.35f, CellSize * 0.35f, 100.f);
        const FVector BadgePos = AnchorLocal + BadgeOffset;

        // Colored backing cube.
        UStaticMeshComponent* Backing = NewObject<UStaticMeshComponent>(this);
        Backing->SetStaticMesh(CubeMesh);
        Backing->SetupAttachment(Root);
        Backing->RegisterComponent();
        Backing->SetRelativeLocation(BadgePos);
        Backing->SetRelativeScale3D(FVector(0.30f, 0.30f, 0.30f));

        if (CellMaterial)
        {
            UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(CellMaterial, this);
            MID->SetVectorParameterValue(TEXT("BaseColor"), GetRegionColor(RegionIdx));
            Backing->SetMaterial(0, MID);
        }

        BadgeBackings.Add(Backing);

        // Text on top of the backing.
        UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
        Text->SetupAttachment(Root);
        Text->RegisterComponent();
        Text->SetText(FText::FromString(BadgeText));
        Text->SetTextRenderColor(FColor::White);
        Text->SetWorldSize(40.f);
        Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetVerticalAlignment(EVRTA_TextCenter);

        // Position slightly above the backing, rotated to lie flat (face up toward camera).
        Text->SetRelativeLocation(BadgePos + FVector(0, 0, 18.f));
        Text->SetRelativeRotation(FRotator(90.f, 180.f, 0.f)); // face up, readable from +X direction

        BadgeTexts.Add(Text);
    }

    UE_LOG(LogTemp, Display, TEXT("PipsBoard: spawned %d badges"), BadgeTexts.Num());

    // Tray: spawn the puzzle's dominoes below the board.
    // Dominoes are oriented "short" (long axis = Y, vertical on screen) to fit more per row.
    const int32 NumDominoes = Puzzle.Dominoes.Num();

    // Pack tray as a roughly-square grid, with up to 5 per row.
    const int32 TrayCols = FMath::Min(NumDominoes, 5);
    const int32 TrayRows = FMath::DivideAndRoundUp(NumDominoes, TrayCols);

    // When dominoes are rotated 90° (vertical), they occupy:
    //   X (depth into screen) = CellSize (their long axis)
    //   Y (across screen)     = CellSize * 0.9 (their width)
    const float DominoSlotX = CellSize * 1.10f;   // depth spacing (between rows on screen)
    const float DominoSlotY = CellSize * 2.20f;   // horizontal spacing (along on-screen X)

    // Place tray below the board (more negative X from centroid).
    const float TrayStartX = -(MaxCell.X - CentroidRow + 1.0f) * CellSize - DominoSlotX;  // was +1.5f
    const float RowWidth = (TrayCols - 1) * DominoSlotY;

    for (int32 i = 0; i < NumDominoes; ++i)
    {
        const int32 Row = i / TrayCols;
        const int32 Col = i % TrayCols;

        const FVector SpawnLocal(
            TrayStartX - Row * DominoSlotX,
            (Col * DominoSlotY) - (RowWidth * 0.5f),
            20.f);

        const FVector SpawnWorld = GetActorLocation() + SpawnLocal;
        const FRotator SpawnRot(0.f, 90.f, 0.f);  // was (0, 90, 0)

        FActorSpawnParameters Params;
        Params.Owner = this;
        APipsDomino* Domino = GetWorld()->SpawnActor<APipsDomino>(
            APipsDomino::StaticClass(), SpawnWorld, SpawnRot, Params);
        if (Domino)
        {
            Domino->DominoMaterial = DominoMaterial;
            Domino->PipMaterial = PipMaterial;
            Domino->Initialize(Puzzle.Dominoes[i].A, Puzzle.Dominoes[i].B, CellSize);
            Domino->TrayLocation = SpawnWorld;
            Domino->TrayRotation = SpawnRot;
            TrayDominoes.Add(Domino);
        }
    }

    UE_LOG(LogTemp, Display, TEXT("PipsBoard: spawned %d dominoes in tray (%dx%d)"),
        TrayDominoes.Num(), TrayRows, TrayCols);
}

bool APipsBoard::IsValidCell(const FIntPoint& Cell) const
{
    return ValidCells.Contains(Cell);
}

FVector APipsBoard::CellToWorld(const FIntPoint& Cell) const
{
    const FVector Local = GridToLocal(Cell) - CachedCentroid;
    return GetActorTransform().TransformPosition(Local);
}

float APipsBoard::GetPlacementZ() const
{
    // Top face of the board tiles (TileHeight = 10) + half domino thickness.
    const float TileHeight = 10.f;
    const float DominoThickness = CellSize * 0.2f;
    return GetActorLocation().Z + TileHeight + DominoThickness * 0.5f;
}

bool APipsBoard::FindNearestCell(const FVector& WorldPos, FIntPoint& OutCell) const
{
    if (ValidCells.Num() == 0) return false;

    float BestDistSq = FLT_MAX;
    for (const FIntPoint& Cell : ValidCells)
    {
        const FVector CellWorld = CellToWorld(Cell);
        const float DistSq = FVector::DistSquared2D(WorldPos, CellWorld);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            OutCell = Cell;
        }
    }
    return true;
}

APipsDomino* APipsBoard::GetDominoAt(const FIntPoint& Cell) const
{
    APipsDomino* const* Found = CellOccupancy.Find(Cell);
    return Found ? *Found : nullptr;
}

void APipsBoard::UnplaceDomino(APipsDomino* Domino)
{
    if (!Domino) return;
    for (const FIntPoint& Cell : Domino->OccupiedCells)
    {
        CellOccupancy.Remove(Cell);
    }
    Domino->OccupiedCells.Reset();
    Domino->bPlaced = false;
}

bool APipsBoard::TryPlaceDomino(APipsDomino* Domino, const FIntPoint& CellA, const FIntPoint& CellB)
{
    if (!Domino) return false;

    // Both cells must exist on the board.
    if (!IsValidCell(CellA) || !IsValidCell(CellB)) return false;

    // Cells must be adjacent (differ by exactly 1 in exactly one axis).
    const int32 DRow = FMath::Abs(CellA.X - CellB.X);
    const int32 DCol = FMath::Abs(CellA.Y - CellB.Y);
    if (DRow + DCol != 1) return false;

    // Cells must be unoccupied, or occupied only by this same domino.
    APipsDomino* OccA = GetDominoAt(CellA);
    APipsDomino* OccB = GetDominoAt(CellB);
    if ((OccA && OccA != Domino) || (OccB && OccB != Domino)) return false;

    // Clear any prior placement.
    UnplaceDomino(Domino);

    // Position the domino so it spans the two cells. Long axis goes from CellA to CellB.
    const FVector PosA = CellToWorld(CellA);
    const FVector PosB = CellToWorld(CellB);
    const FVector Center = (PosA + PosB) * 0.5f;
    const FVector Final(Center.X, Center.Y, GetPlacementZ());

    // Determine yaw: domino's "long axis" is +X in its local frame.
    // We want HalfA at CellA and HalfB at CellB, so the +X local direction
    // should point from CellA to CellB.
    const FVector Direction = (PosB - PosA).GetSafeNormal();
    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));

    Domino->SetActorLocation(Final);
    Domino->SetActorRotation(FRotator(0.f, Yaw, 0.f));

    Domino->OccupiedCells = { CellA, CellB };
    Domino->bPlaced = true;
    CellOccupancy.Add(CellA, Domino);
    CellOccupancy.Add(CellB, Domino);

    return true;
}

FLinearColor APipsBoard::GetRegionColor(int32 RegionIndex)
{
    // 12-color palette loosely inspired by the NYT puzzle: muted but distinct.
    static const TArray<FLinearColor> Palette = {
        FLinearColor(0.78f, 0.55f, 0.82f), // purple
        FLinearColor(0.96f, 0.62f, 0.71f), // pink
        FLinearColor(0.50f, 0.78f, 0.82f), // teal
        FLinearColor(0.98f, 0.72f, 0.45f), // orange
        FLinearColor(0.70f, 0.70f, 0.78f), // slate
        FLinearColor(0.75f, 0.80f, 0.50f), // olive
        FLinearColor(0.92f, 0.45f, 0.50f), // coral
        FLinearColor(0.45f, 0.62f, 0.85f), // blue
        FLinearColor(0.55f, 0.82f, 0.55f), // green
        FLinearColor(0.92f, 0.85f, 0.45f), // yellow
        FLinearColor(0.65f, 0.50f, 0.40f), // brown
        FLinearColor(0.40f, 0.78f, 0.85f), // cyan
    };
    return Palette[RegionIndex % Palette.Num()];
}