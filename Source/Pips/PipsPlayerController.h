#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PipsPlayerController.generated.h"

UCLASS()
class PIPS_API APipsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APipsPlayerController();

protected:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void BeginPlay() override;

private:
	/** Currently hovered domino, if any. */
	UPROPERTY()
	class APipsDomino* HoveredDomino = nullptr;

	/** The drag plane height above the board surface (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Pips")
	float DragHeight = 80.f;

	UPROPERTY()
	APipsDomino* DraggedDomino = nullptr;

	/** World position the drag started, used to detect a click vs. drag (optional later). */
	FVector DragStartWorld = FVector::ZeroVector;

	/** Domino's spawn position, used to return on release in Slice B. */
	FVector DragOriginalLocation = FVector::ZeroVector;

	void OnPrimaryPressed();
	void OnPrimaryReleased();

	/** Projects the cursor ray onto a horizontal plane at the given Z. */
	bool GetCursorPositionOnDragPlane(FVector& OutWorld) const;

	void UpdateHover();

	APipsDomino* FindDanglingDomino() const;
	void ResolveDangling(APipsDomino* Domino);

	/** Cached board reference. Found on first use. */
	UPROPERTY()
	class APipsBoard* Board = nullptr;

	APipsBoard* GetBoard();

	/** Determines the two cells a dragged domino is closest to. */
	bool ComputeSnapCells(APipsDomino* Domino, FIntPoint& OutCellA, FIntPoint& OutCellB) const;

	/** Returns the domino to its tray position (used on invalid drop). */
	void ReturnToTray(APipsDomino* Domino);

	void RotatePlacedDomino(APipsDomino* Domino);

	void OnRotatePressed();
};