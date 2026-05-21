// PipsTypes.h
#pragma once

#include "CoreMinimal.h"
#include "PipsTypes.generated.h"

UENUM(BlueprintType)
enum class EPipsRegionType : uint8
{
    Empty       UMETA(DisplayName = "Empty"),
    Sum         UMETA(DisplayName = "Sum"),
    Equals      UMETA(DisplayName = "Equals"),
    Unequal     UMETA(DisplayName = "Unequal"),
    Greater     UMETA(DisplayName = "Greater"),
    Less        UMETA(DisplayName = "Less"),
};

UENUM(BlueprintType)
enum class EPipsDifficulty : uint8
{
    Easy    UMETA(DisplayName = "Easy"),
    Medium  UMETA(DisplayName = "Medium"),
    Hard    UMETA(DisplayName = "Hard"),
};

/** A single domino piece: two pip values (0-6). */
USTRUCT(BlueprintType)
struct FPipsDomino
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    int32 A = 0;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    int32 B = 0;
};

/** A constraint region: a set of cells with a rule applied to them. */
USTRUCT(BlueprintType)
struct FPipsRegion
{
    GENERATED_BODY()

    /** Cells belonging to this region. FIntPoint(X=row, Y=col). */
    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    TArray<FIntPoint> Cells;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    EPipsRegionType Type = EPipsRegionType::Empty;

    /** Only meaningful for Sum / Greater / Less. -1 otherwise. */
    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    int32 Target = -1;
};

/** One placed domino in the solution: two adjacent cells. */
USTRUCT(BlueprintType)
struct FPipsSolutionPlacement
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FIntPoint CellA = FIntPoint::ZeroValue;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FIntPoint CellB = FIntPoint::ZeroValue;
};

/** A single puzzle (one difficulty level on one day). */
USTRUCT(BlueprintType)
struct FPipsPuzzle
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    int32 Id = 0;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FString BackendId;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FString Constructors;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    TArray<FPipsDomino> Dominoes;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    TArray<FPipsRegion> Regions;

    /** Solution placements, in the same order as Dominoes by convention. */
    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    TArray<FPipsSolutionPlacement> Solution;

    bool IsValid() const { return Dominoes.Num() > 0 && Regions.Num() > 0; }
};

/** All three puzzles for a single date. */
USTRUCT(BlueprintType)
struct FPipsDailyData
{
    GENERATED_BODY()

    /** ISO date string, e.g. "2026-05-21". Matches the API's printDate. */
    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FString PrintDate;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FString Editor;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FPipsPuzzle Easy;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FPipsPuzzle Medium;

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FPipsPuzzle Hard;

    const FPipsPuzzle& GetByDifficulty(EPipsDifficulty Difficulty) const
    {
        switch (Difficulty)
        {
            case EPipsDifficulty::Easy:   return Easy;
            case EPipsDifficulty::Medium: return Medium;
            case EPipsDifficulty::Hard:   return Hard;
        }
        return Easy;
    }
};