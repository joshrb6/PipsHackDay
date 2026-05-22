#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PipsTypes.h"
#include "PipsBoard.generated.h"

// at the top with other forward declarations or in the class body
class UTextRenderComponent;

DECLARE_MULTICAST_DELEGATE(FOnPuzzleSolved);
DECLARE_MULTICAST_DELEGATE(FOnPuzzleInvalid);

UCLASS()
class PIPS_API APipsBoard : public AActor
{
	GENERATED_BODY()

public:
	APipsBoard();

	/** Size of one grid cell in world units (cm). 100 = 1m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips")
	float CellSize = 100.f;

	/** Which date to load on BeginPlay (debug — will come from GameInstance later). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips|Debug")
	FString DebugDate = TEXT("2026-05-21");

	/** Which difficulty to load on BeginPlay (debug). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips|Debug")
	EPipsDifficulty DebugDifficulty = EPipsDifficulty::Easy;

	/** Builds the visual representation of a puzzle. Clears any existing tiles first. */
	UFUNCTION(BlueprintCallable, Category = "Pips")
	void SpawnFromPuzzle(const FPipsPuzzle& Puzzle);

	/** Stores the currently loaded puzzle, used by validation. */
	UPROPERTY()
	FPipsPuzzle CurrentPuzzle;

	FOnPuzzleSolved OnPuzzleSolved;
	FOnPuzzleInvalid OnPuzzleInvalid;

	/** Builds a cell-to-pip-value map from currently placed dominoes. */
	TMap<FIntPoint, int32> CollectCellValues() const;

	/** Evaluates current state and triggers UI on win/fail. */
	void EvaluateAndNotify();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips")
	UMaterialInterface* CellMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips")
	UMaterialInterface* DominoMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips")
	UMaterialInterface* PipMaterial = nullptr;

	UPROPERTY()
	TArray<class APipsDomino*> TrayDominoes;


	/** Returns true if a cell exists in the currently loaded puzzle. */
	UFUNCTION(BlueprintCallable, Category = "Pips")
	bool IsValidCell(const FIntPoint& Cell) const;

	/** World location of a given cell's center (Z = board surface). */
	UFUNCTION(BlueprintCallable, Category = "Pips")
	FVector CellToWorld(const FIntPoint& Cell) const;

	/** Finds the nearest valid cell to a world position. Returns false if board has no cells. */
	bool FindNearestCell(const FVector& WorldPos, FIntPoint& OutCell) const;

	/** Returns the domino occupying the given cell, or nullptr. */
	class APipsDomino* GetDominoAt(const FIntPoint& Cell) const;

	/** Attempts to place a domino on the given two cells. Updates occupancy on success. */
	bool TryPlaceDomino(class APipsDomino* Domino, const FIntPoint& CellA, const FIntPoint& CellB);

	/** Removes a domino from any cells it currently occupies. */
	void UnplaceDomino(class APipsDomino* Domino);

	/** Z height of the top face of a placed domino. */
	UFUNCTION(BlueprintCallable, Category = "Pips")
	float GetPlacementZ() const;
	
protected:
	virtual void BeginPlay() override;

private:
	/** Root component all spawned cells attach to. */
	UPROPERTY()
	USceneComponent* Root = nullptr;

	/** Spawned cell mesh components, kept for cleanup. */
	UPROPERTY()
	TArray<UStaticMeshComponent*> CellMeshes;

	UPROPERTY()
	TArray<UTextRenderComponent*> BadgeTexts;

	UPROPERTY()
	TArray<UStaticMeshComponent*> BadgeBackings;
	
	/** Cached centroid offset used to center the puzzle at the board origin. */
	FVector CachedCentroid = FVector::ZeroVector;

	/** Cells that exist in the current puzzle (for fast validity queries). */
	TSet<FIntPoint> ValidCells;

	/** Which domino (if any) currently occupies each cell. */
	TMap<FIntPoint, class APipsDomino*> CellOccupancy;

	uint32 LastInvalidHash = 0;
	uint32 ComputePlacementHash() const;

	/** Converts a grid coordinate to a local-space position. */
	FVector GridToLocal(const FIntPoint& Cell) const;

	/** Clears any previously spawned visuals. */
	void ClearVisuals();

	/** Returns a distinct color for a given region index. */
	static FLinearColor GetRegionColor(int32 RegionIndex);
};