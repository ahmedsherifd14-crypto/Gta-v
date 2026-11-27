#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SCTestWorldBuilder.generated.h"

class USceneComponent;
class UStaticMesh;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;

/**
 * Builds a small, deterministic GTA-style test city at runtime from engine primitives:
 * road grid, sidewalk blocks, towers, a central plaza, parked cars, cash bags, a save point,
 * plus a sun/moon/sky driven by the game mode's clock.
 *
 * It exists so the project is playable immediately without any authored map or art assets.
 * It is a foundation for blocking out gameplay, not final world art.
 */
UCLASS()
class SANCLEMENTE_API ASCTestWorldBuilder : public AActor
{
    GENERATED_BODY()

public:
    ASCTestWorldBuilder();

    virtual void Tick(float DeltaSeconds) override;

    /** Picks a random point on a road lane between MinDist and MaxDist from Near. Returns false if none found. */
    bool GetRandomRoadPoint(const FVector& Near, float MinDist, float MaxDist, FVector& OutLocation) const;

    UPROPERTY(EditAnywhere, Category="City") int32 CivilianCount = 24;

    /** Number of city blocks per side (forced odd so the plaza sits at the origin). */
    UPROPERTY(EditAnywhere, Category="City") int32 BlocksPerSide = 7;
    UPROPERTY(EditAnywhere, Category="City") float BlockSize = 5600.0f;
    UPROPERTY(EditAnywhere, Category="City") float RoadWidth = 1400.0f;
    UPROPERTY(EditAnywhere, Category="City") int32 Seed = 1337;
    UPROPERTY(EditAnywhere, Category="City") int32 RoadsideMoneyPickups = 28;

    UPROPERTY(EditAnywhere, Category="Lighting") float SunIntensity = 10.0f;
    UPROPERTY(EditAnywhere, Category="Lighting") float MoonIntensity = 0.35f;
    UPROPERTY(EditAnywhere, Category="Lighting") float SunYawDegrees = 35.0f;

protected:
    virtual void BeginPlay() override;

    void BuildCity();
    void SpawnGameplayActors();
    void UpdateLighting();

    UInstancedStaticMeshComponent* MakeInstances(const TCHAR* ComponentName, UStaticMesh* Mesh, const FLinearColor& Color, bool bCollision);

    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;

    UPROPERTY(VisibleAnywhere, Category="Lighting") TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere, Category="Lighting") TObjectPtr<UDirectionalLightComponent> SunLight;
    UPROPERTY(VisibleAnywhere, Category="Lighting") TObjectPtr<UDirectionalLightComponent> MoonLight;
    UPROPERTY(VisibleAnywhere, Category="Lighting") TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;
    UPROPERTY(VisibleAnywhere, Category="Lighting") TObjectPtr<USkyLightComponent> SkyLight;
    UPROPERTY(VisibleAnywhere, Category="Lighting") TObjectPtr<UExponentialHeightFogComponent> HeightFog;
};
