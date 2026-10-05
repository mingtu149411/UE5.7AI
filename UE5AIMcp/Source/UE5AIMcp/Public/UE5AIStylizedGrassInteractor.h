// 项目作者水印：南京大学24届沈逸文
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UE5AIStylizedGrassInteractor.generated.h"

class APawn;
class USceneCaptureComponent2D;
class USceneComponent;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 * Runtime owner for the stylized-grass interaction field.  A Blueprint based
 * on this actor is saved with the sample map, while this native base handles
 * the inexpensive per-frame RT update during PIE and packaged play.
 */
UCLASS(Blueprintable, BlueprintType)
class UE5AIMCP_API AUE5AIStylizedGrassInteractor : public AActor
{
    GENERATED_BODY()

public:
    AUE5AIStylizedGrassInteractor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "UE5AI|Grass Interaction")
    void SetTrackedPawn(APawn* InPawn);

    UFUNCTION(BlueprintPure, Category = "UE5AI|Grass Interaction")
    APawn* GetTrackedPawn() const;

    UFUNCTION(BlueprintPure, Category = "UE5AI|Grass Interaction")
    FVector GetLastCapturedWorldLocation() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UE5AI|Grass Interaction")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UE5AI|Grass Interaction")
    TObjectPtr<USceneCaptureComponent2D> InteractionCapture;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UE5AI|Grass Interaction")
    TObjectPtr<UStaticMeshComponent> InteractionProxy;

    /** Persistent RT sampled by the grass material using world-position UVs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UE5AI|Grass Interaction")
    TObjectPtr<UTextureRenderTarget2D> InteractionRenderTarget;

    /** World-space radius of the local bend imprint in centimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UE5AI|Grass Interaction", meta = (ClampMin = "50.0"))
    float InteractionRadius = 180.0f;

    /** Fixed height of the orthographic world capture. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UE5AI|Grass Interaction", meta = (ClampMin = "1000.0"))
    float CaptureHeight = 12000.0f;

    /** Covers the complete grass showcase field, centred at world origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UE5AI|Grass Interaction", meta = (ClampMin = "1000.0"))
    float CaptureWidth = 29000.0f;

private:
    TWeakObjectPtr<APawn> TrackedPawn;
    FVector LastCapturedWorldLocation = FVector::ZeroVector;
    float CaptureAccumulator = 0.0f;

    void RefreshCapture();
};
