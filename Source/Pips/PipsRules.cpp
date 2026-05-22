#include "PipsRules.h"

EPipsResult UPipsRules::EvaluatePuzzle(const FPipsPuzzle& Puzzle, const TMap<FIntPoint, int32>& CellValues)
{
    // Step 1: compute total cells needing values (sum of all region cells).
    int32 TotalCells = 0;
    for (const FPipsRegion& Region : Puzzle.Regions)
    {
        TotalCells += Region.Cells.Num();
    }

    // If not all cells have values, the puzzle isn't finished.
    if (CellValues.Num() < TotalCells)
    {
        return EPipsResult::NotFinished;
    }

    // Step 2: validate each region's constraint.
    for (const FPipsRegion& Region : Puzzle.Regions)
    {
        // Gather values for this region's cells.
        TArray<int32> Values;
        Values.Reserve(Region.Cells.Num());
        for (const FIntPoint& Cell : Region.Cells)
        {
            const int32* Val = CellValues.Find(Cell);
            if (!Val)
            {
                // Should have been caught by the totals check above. Defensive.
                return EPipsResult::NotFinished;
            }
            Values.Add(*Val);
        }

        switch (Region.Type)
        {
            case EPipsRegionType::Empty:
                // No constraint.
                break;

            case EPipsRegionType::Sum:
            {
                int32 Sum = 0;
                for (int32 V : Values) Sum += V;
                if (Sum != Region.Target) return EPipsResult::Invalid;
                break;
            }

            case EPipsRegionType::Equals:
            {
                if (Values.Num() == 0) break;
                const int32 First = Values[0];
                for (int32 V : Values) if (V != First) return EPipsResult::Invalid;
                break;
            }

            case EPipsRegionType::Unequal:
            {
                TSet<int32> Seen;
                for (int32 V : Values)
                {
                    if (Seen.Contains(V)) return EPipsResult::Invalid;
                    Seen.Add(V);
                }
                break;
            }

            case EPipsRegionType::Greater:
            {
                for (int32 V : Values) if (V <= Region.Target) return EPipsResult::Invalid;
                break;
            }

            case EPipsRegionType::Less:
            {
                for (int32 V : Values) if (V >= Region.Target) return EPipsResult::Invalid;
                break;
            }
        }
    }

    return EPipsResult::Solved;
}