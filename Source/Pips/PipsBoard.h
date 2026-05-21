#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PipsTypes.h"
#include "PipsBoard.generated.h"

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

protected:
	virtual void BeginPlay() override;

private:
	/** Root component all spawned cells attach to. */
	UPROPERTY()
	USceneComponent* Root = nullptr;

	/** Spawned cell mesh components, kept for cleanup. */
	UPROPERTY()
	TArray<UStaticMeshComponent*> CellMeshes;

	/** Converts a grid coordinate to a local-space position. */
	FVector GridToLocal(const FIntPoint& Cell) const;

	/** Clears any previously spawned visuals. */
	void ClearCells();
};