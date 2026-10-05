// 项目作者水印：南京大学24届沈逸文
#include "UE5AIStylizedGrassInteractor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"

AUE5AIStylizedGrassInteractor::AUE5AIStylizedGrassInteractor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("InteractionCapture"));
    InteractionCapture->SetupAttachment(SceneRoot);
    InteractionCapture->ProjectionType = ECameraProjectionMode::Orthographic;
    InteractionCapture->OrthoWidth = CaptureWidth;
    InteractionCapture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
    InteractionCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    InteractionCapture->bCaptureEveryFrame = false;
    InteractionCapture->bCaptureOnMovement = false;
    InteractionCapture->SetRelativeLocation(FVector(0.0f, 0.0f, CaptureHeight));
    InteractionCapture->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

    InteractionProxy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InteractionProxy"));
    InteractionProxy->SetupAttachment(SceneRoot);
    InteractionProxy->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    InteractionProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionProxy->SetCastShadow(false);
    InteractionProxy->SetVisibleInSceneCaptureOnly(true);
}

void AUE5AIStylizedGrassInteractor::BeginPlay()
{
    Super::BeginPlay();
    if (!TrackedPawn.IsValid())
    {
        TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    }
    RefreshCapture();
}

void AUE5AIStylizedGrassInteractor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!TrackedPawn.IsValid())
    {
        TrackedPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    }
    if (!TrackedPawn.IsValid())
    {
        return;
    }

    const FVector PawnLocation = TrackedPawn->GetActorLocation();
    InteractionProxy->SetWorldLocation(FVector(PawnLocation.X, PawnLocation.Y, 8.0f));
    InteractionProxy->SetWorldScale3D(FVector(InteractionRadius / 50.0f, InteractionRadius / 50.0f, 0.16f));
    LastCapturedWorldLocation = InteractionProxy->GetComponentLocation();

    CaptureAccumulator += DeltaSeconds;
    if (CaptureAccumulator >= (1.0f / 30.0f))
    {
        CaptureAccumulator = 0.0f;
        RefreshCapture();
    }
}

void AUE5AIStylizedGrassInteractor::SetTrackedPawn(APawn* InPawn)
{
    TrackedPawn = InPawn;
}

APawn* AUE5AIStylizedGrassInteractor::GetTrackedPawn() const
{
    return TrackedPawn.Get();
}

FVector AUE5AIStylizedGrassInteractor::GetLastCapturedWorldLocation() const
{
    return LastCapturedWorldLocation;
}

void AUE5AIStylizedGrassInteractor::RefreshCapture()
{
    if (!InteractionCapture || !InteractionProxy || !InteractionRenderTarget)
    {
        return;
    }

    InteractionCapture->TextureTarget = InteractionRenderTarget;
    InteractionCapture->OrthoWidth = CaptureWidth;
    InteractionCapture->SetWorldLocation(FVector(0.0f, 0.0f, CaptureHeight));
    InteractionCapture->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
    InteractionCapture->ClearShowOnlyComponents();
    InteractionCapture->ShowOnlyComponent(InteractionProxy);
    InteractionCapture->CaptureScene();
}
