#include "PipsBoard.h"
#include "PipsGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

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

void APipsBoard::ClearCells()
{
    for (UStaticMeshComponent* Mesh : CellMeshes)
    {
        if (IsValid(Mesh))
        {
            Mesh->DestroyComponent();
        }
    }
    CellMeshes.Reset();
}

void APipsBoard::SpawnFromPuzzle(const FPipsPuzzle& Puzzle)
{
    ClearCells();

    // Find Engine's built-in cube mesh — perfect placeholder.
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("PipsBoard: could not load /Engine/BasicShapes/Cube"));
        return;
    }

    // First pass: collect unique cells and compute bounding box.
    TSet<FIntPoint> UniqueCells;
    for (const FPipsRegion& Region : Puzzle.Regions)
    {
        for (const FIntPoint& Cell : Region.Cells)
        {
            UniqueCells.Add(Cell);
        }
    }

    if (UniqueCells.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PipsBoard: puzzle has no cells to spawn"));
        return;
    }

    FIntPoint MinCell( INT_MAX,  INT_MAX);
    FIntPoint MaxCell(-INT_MAX, -INT_MAX);
    for (const FIntPoint& Cell : UniqueCells)
    {
        MinCell.X = FMath::Min(MinCell.X, Cell.X);
        MinCell.Y = FMath::Min(MinCell.Y, Cell.Y);
        MaxCell.X = FMath::Max(MaxCell.X, Cell.X);
        MaxCell.Y = FMath::Max(MaxCell.Y, Cell.Y);
    }

    // Centroid in grid coordinates (use float for half-cell offsets).
    const float CentroidRow = (MinCell.X + MaxCell.X) * 0.5f;
    const float CentroidCol = (MinCell.Y + MaxCell.Y) * 0.5f;
    const FVector Centroid(-CentroidRow * CellSize, CentroidCol * CellSize, 0.f);

    // Second pass: spawn tiles, offset so centroid sits at board origin.
    for (const FIntPoint& Cell : UniqueCells)
    {
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

        CellMeshes.Add(Mesh);
    }

    UE_LOG(LogTemp, Display, TEXT("PipsBoard: spawned %d cell tiles"), CellMeshes.Num());
}