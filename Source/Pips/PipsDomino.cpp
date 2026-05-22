#include "PipsDomino.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

APipsDomino::APipsDomino()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

TArray<FVector2D> APipsDomino::GetPipPositions(int32 PipCount)
{
    // Positions are in normalized half-coordinates [-1, 1] x [-1, 1].
    // X = along the long axis of the domino, Y = across the short axis.
    // We use the same 3x3 grid that traditional dominoes use.
    const float Off = 0.45f; // distance from center for corner dots

    switch (PipCount)
    {
        case 0: return {};
        case 1: return { {0, 0} };
        case 2: return { {-Off, -Off}, {Off, Off} };
        case 3: return { {-Off, -Off}, {0, 0}, {Off, Off} };
        case 4: return { {-Off, -Off}, {-Off, Off}, {Off, -Off}, {Off, Off} };
        case 5: return { {-Off, -Off}, {-Off, Off}, {0, 0}, {Off, -Off}, {Off, Off} };
        case 6: return { {-Off, -Off}, {0, -Off}, {Off, -Off},
             {-Off,  Off}, {0,  Off}, {Off,  Off} };
        default: return {};
    }
}

void APipsDomino::SpawnPipsForHalf(int32 PipCount, const FVector& HalfCenter, float HalfDim, float Thickness)
{
    UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!SphereMesh) return;

    const TArray<FVector2D> Positions = GetPipPositions(PipCount);
    const float PipRadius = HalfDim * 0.15f;
    const float PipScale = (PipRadius * 2.f) / 100.f;

    for (const FVector2D& Pos : Positions)
    {
        UStaticMeshComponent* Pip = NewObject<UStaticMeshComponent>(this);
        Pip->SetStaticMesh(SphereMesh);
        Pip->SetupAttachment(Root); // Attach to Root, not the half
        Pip->RegisterComponent();
        Pip->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        const FVector LocalPos = HalfCenter + FVector(
            Pos.X * HalfDim,
            Pos.Y * HalfDim,
            Thickness * 0.5f + PipRadius * 0.5f); // top of half, pip half-embedded

        Pip->SetRelativeLocation(LocalPos);
        Pip->SetRelativeScale3D(FVector(PipScale));

        if (PipMaterial)
        {
            Pip->SetMaterial(0, PipMaterial);
        }

        Pips.Add(Pip);
    }
}

void APipsDomino::Initialize(int32 InPipA, int32 InPipB, float InCellSize)
{
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh) return;

    // A domino covers two cells along its long axis.
    // We orient with long axis = X (so two halves are at +X/2 and -X/2 relative to root).
    const float HalfLength = InCellSize;          // each half spans one full cell
    const float Width      = InCellSize * 0.9f;
    const float Thickness  = InCellSize * 0.2f;

    auto MakeHalf = [&](const FVector& LocalPos) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* Half = NewObject<UStaticMeshComponent>(this);
        Half->SetStaticMesh(CubeMesh);
        Half->SetupAttachment(Root);
        Half->RegisterComponent();
        Half->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Half->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
        Half->SetRelativeLocation(LocalPos);
        Half->SetRelativeScale3D(FVector(
            HalfLength / 100.f,
            Width      / 100.f,
            Thickness  / 100.f));
        if (DominoMaterial)
        {
            UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(DominoMaterial, this);
            MID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.95f, 0.93f, 0.88f)); // ivory
            Half->SetMaterial(0, MID);
        }
        return Half;
    };

    const FVector HalfACenter(-HalfLength * 0.5f, 0.f, Thickness * 0.5f);
    const FVector HalfBCenter( HalfLength * 0.5f, 0.f, Thickness * 0.5f);

    HalfA = MakeHalf(HalfACenter);
    HalfB = MakeHalf(HalfBCenter);

    SpawnPipsForHalf(InPipA, HalfACenter, HalfLength * 0.5f, Thickness);
    SpawnPipsForHalf(InPipB, HalfBCenter, HalfLength * 0.5f, Thickness);
}