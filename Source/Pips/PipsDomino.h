#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PipsDomino.generated.h"

UCLASS()
class PIPS_API APipsDomino : public AActor
{
	GENERATED_BODY()

public:
	APipsDomino();

	/** Initializes the domino visuals with two pip values (0-6). Call after spawn. */
	UFUNCTION(BlueprintCallable, Category = "Pips")
	void Initialize(int32 InPipA, int32 InPipB, float InCellSize);

	/** Materials supplied by the board. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips")
	UMaterialInterface* DominoMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pips")
	UMaterialInterface* PipMaterial = nullptr;


	/** Cells this domino currently occupies, if placed. Empty otherwise. */
	UPROPERTY()
	TArray<FIntPoint> OccupiedCells;

	/** True when the domino has been placed on the board (vs. in tray or dragging). */
	UPROPERTY()
	bool bPlaced = false;

	/** Tray position to return to when an invalid placement is attempted. */
	UPROPERTY()
	FVector TrayLocation = FVector::ZeroVector;

	/** Tray rotation to return to. */
	UPROPERTY()
	FRotator TrayRotation = FRotator::ZeroRotator;
	
	/** Last valid on-board placement (cells + transform), for revert-on-abandon. */
	UPROPERTY()
	FVector LastValidLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator LastValidRotation = FRotator::ZeroRotator;

	UPROPERTY()
	TArray<FIntPoint> LastValidCells;

	/** Visual cells the domino is currently shown at (may be invalid). Used as rotation pivot. */
	UPROPERTY()
	TArray<FIntPoint> VirtualCells;

	UPROPERTY()
	int32 PipA = 0;

	UPROPERTY()
	int32 PipB = 0;

	/** True if this domino has ever been validly placed (and thus has a "last valid" to return to). */
	UPROPERTY()
	bool bHasLastValid = false;

	/** True if domino is shown on the board but not occupying cells (mid-rotation state). */
	UPROPERTY()
	bool bDangling = false;
	
protected:
	UPROPERTY()
	USceneComponent* Root = nullptr;

	UPROPERTY()
	UStaticMeshComponent* HalfA = nullptr;

	UPROPERTY()
	UStaticMeshComponent* HalfB = nullptr;

	UPROPERTY()
	TArray<UStaticMeshComponent*> Pips;

private:
	void SpawnPipsForHalf(int32 PipCount, const FVector& HalfCenter, float HalfDim, float Thickness);
	static TArray<FVector2D> GetPipPositions(int32 PipCount);
};