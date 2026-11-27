#include "SCTestWorldBuilder.h"
#include "SCBasicShapes.h"
#include "SCGameMode.h"
#include "SCMoneyPickup.h"
#include "SCNPC.h"
#include "SCSavePoint.h"
#include "SCVehicle.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UnrealType.h"

namespace
{
    constexpr float RoadTopZ = 4.0f;       // asphalt surface height
    constexpr float SidewalkTopZ = 12.0f;  // curb height above the ground

    // Sets a flag/int on a component by reflection so a renamed engine property can never break the build.
    void SetBoolProperty(UObject* Object, const TCHAR* PropertyName, bool bValue)
    {
        if (Object)
        {
            if (FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
            {
                Property->SetPropertyValue_InContainer(Object, bValue);
            }
        }
    }

    void SetIntProperty(UObject* Object, const TCHAR* PropertyName, int32 Value)
    {
        if (Object)
        {
            if (FIntProperty* Property = FindFProperty<FIntProperty>(Object->GetClass(), PropertyName))
            {
                Property->SetPropertyValue_InContainer(Object, Value);
            }
        }
    }
}

ASCTestWorldBuilder::ASCTestWorldBuilder()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    CubeMesh = SCShapes::Cube();
    CylinderMesh = SCShapes::Cylinder();
    SphereMesh = SCShapes::Sphere();
    BaseMaterial = SCShapes::BasicMaterial();

    // Sun (atmosphere sun light #0) and a faint moon (#1).
    SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
    SunLight->SetupAttachment(Root);
    SunLight->SetMobility(EComponentMobility::Movable);
    SunLight->SetIntensity(SunIntensity);
    SetBoolProperty(SunLight, TEXT("bUsedAsAtmosphereSunLight"), true);
    SetIntProperty(SunLight, TEXT("AtmosphereSunLightIndex"), 0);

    MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
    MoonLight->SetupAttachment(Root);
    MoonLight->SetMobility(EComponentMobility::Movable);
    MoonLight->SetIntensity(0.0f);
    MoonLight->SetLightColor(FLinearColor(0.55f, 0.65f, 1.0f));
    SetBoolProperty(MoonLight, TEXT("bUsedAsAtmosphereSunLight"), false);

    SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
    SkyAtmosphere->SetupAttachment(Root);

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(Root);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->bRealTimeCapture = true;   // follows the atmosphere as the sun moves

    HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
    HeightFog->SetupAttachment(Root);
    HeightFog->FogDensity = 0.004f;
}

void ASCTestWorldBuilder::BeginPlay()
{
    Super::BeginPlay();

    BuildCity();
    SpawnGameplayActors();
    UpdateLighting();
}

void ASCTestWorldBuilder::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateLighting();
}

void ASCTestWorldBuilder::UpdateLighting()
{
    float Hours = 12.0f;
    if (const UWorld* World = GetWorld())
    {
        if (const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>())
        {
            Hours = GameMode->GetTimeOfDayHours();
        }
    }

    // 06:00 sunrise (0 deg), 12:00 zenith (90 deg), 18:00 sunset (180 deg).
    const float SunAngle = (Hours / 24.0f) * 360.0f - 90.0f;
    const float Elevation = FMath::Sin(FMath::DegreesToRadians(SunAngle));

    SunLight->SetWorldRotation(FRotator(-SunAngle, SunYawDegrees, 0.0f));
    SunLight->SetIntensity(SunIntensity * FMath::Clamp((Elevation + 0.05f) / 0.15f, 0.0f, 1.0f));

    MoonLight->SetWorldRotation(FRotator(SunAngle, SunYawDegrees + 180.0f, 0.0f));
    MoonLight->SetIntensity(MoonIntensity * FMath::Clamp((-Elevation + 0.05f) / 0.15f, 0.0f, 1.0f));
}

bool ASCTestWorldBuilder::GetRandomRoadPoint(const FVector& Near, float MinDist, float MaxDist, FVector& OutLocation) const
{
    const float Pitch = BlockSize + RoadWidth;
    const int32 Half = FMath::Max(3, BlocksPerSide) / 2;

    for (int32 Attempt = 0; Attempt < 24; ++Attempt)
    {
        const bool bAlongX = FMath::FRand() < 0.5f;
        const int32 K = FMath::RandRange(-Half - 1, Half);
        const float Line = (K + 0.5f) * Pitch;
        const float Lane = FMath::FRand() < 0.5f ? 350.0f : -350.0f;
        const float Along = (bAlongX ? Near.X : Near.Y) + FMath::FRandRange(-MaxDist, MaxDist);

        // Keep inside the city.
        if (FMath::Abs(Along) > (Half + 0.5f) * Pitch) continue;

        const FVector Candidate = bAlongX ? FVector(Along, Line + Lane, RoadTopZ + 110.0f)
                                          : FVector(Line + Lane, Along, RoadTopZ + 110.0f);
        const float Dist = FVector::Dist2D(Candidate, Near);
        if (Dist >= MinDist && Dist <= MaxDist)
        {
            OutLocation = Candidate;
            return true;
        }
    }
    return false;
}

UInstancedStaticMeshComponent* ASCTestWorldBuilder::MakeInstances(const TCHAR* ComponentName, UStaticMesh* Mesh, const FLinearColor& Color, bool bCollision)
{
    UInstancedStaticMeshComponent* Component = NewObject<UInstancedStaticMeshComponent>(this, FName(ComponentName));
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    Component->SetupAttachment(GetRootComponent());
    if (!bCollision)
    {
        Component->SetCastShadow(false);
    }
    Component->RegisterComponent();
    AddInstanceComponent(Component);

    if (BaseMaterial)
    {
        if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this))
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Component->SetMaterial(0, Material);
        }
    }
    return Component;
}

void ASCTestWorldBuilder::BuildCity()
{
    if (!CubeMesh || !CylinderMesh || !SphereMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("SCTestWorldBuilder: engine BasicShapes meshes not found; the test city cannot be built."));
        return;
    }

    BlocksPerSide = FMath::Max(3, BlocksPerSide);
    if (BlocksPerSide % 2 == 0) ++BlocksPerSide;

    const float Pitch = BlockSize + RoadWidth;
    const int32 Half = BlocksPerSide / 2;
    const float Extent = BlocksPerSide * Pitch + RoadWidth;
    FRandomStream Random(Seed);

    // Ground: top face at Z = 0.
    UInstancedStaticMeshComponent* Ground = MakeInstances(TEXT("Ground"), CubeMesh, FLinearColor(0.07f, 0.12f, 0.05f), true);
    Ground->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -50.0f), FVector(Extent * 3.0f / 100.0f, Extent * 3.0f / 100.0f, 1.0f)));

    // Roads (a full-length strip per grid line) and lane markings.
    UInstancedStaticMeshComponent* Roads = MakeInstances(TEXT("Roads"), CubeMesh, FLinearColor(0.025f, 0.025f, 0.03f), true);
    UInstancedStaticMeshComponent* LaneMarks = MakeInstances(TEXT("LaneMarks"), CubeMesh, FLinearColor(0.8f, 0.7f, 0.2f), false);

    auto IsInIntersection = [&](float T)
    {
        for (int32 M = -Half - 1; M <= Half; ++M)
        {
            if (FMath::Abs(T - (M + 0.5f) * Pitch) < RoadWidth * 0.5f + 300.0f) return true;
        }
        return false;
    };

    for (int32 K = -Half - 1; K <= Half; ++K)
    {
        const float C = (K + 0.5f) * Pitch;
        Roads->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, C, RoadTopZ * 0.5f), FVector(Extent / 100.0f, RoadWidth / 100.0f, RoadTopZ / 100.0f)));
        Roads->AddInstance(FTransform(FRotator::ZeroRotator, FVector(C, 0.0f, RoadTopZ * 0.5f), FVector(RoadWidth / 100.0f, Extent / 100.0f, RoadTopZ / 100.0f)));

        for (float T = -Extent * 0.5f + 450.0f; T < Extent * 0.5f; T += 900.0f)
        {
            if (IsInIntersection(T)) continue;
            LaneMarks->AddInstance(FTransform(FRotator::ZeroRotator, FVector(T, C, RoadTopZ + 0.5f), FVector(3.0f, 0.2f, 0.01f)));
            LaneMarks->AddInstance(FTransform(FRotator::ZeroRotator, FVector(C, T, RoadTopZ + 0.5f), FVector(0.2f, 3.0f, 0.01f)));
        }
    }

    // Blocks: sidewalk slab + four towers per block; the centre block is a green plaza.
    UInstancedStaticMeshComponent* Sidewalks = MakeInstances(TEXT("Sidewalks"), CubeMesh, FLinearColor(0.22f, 0.22f, 0.22f), true);
    UInstancedStaticMeshComponent* Plaza = MakeInstances(TEXT("Plaza"), CubeMesh, FLinearColor(0.06f, 0.22f, 0.05f), true);
    UInstancedStaticMeshComponent* Buildings[3] =
    {
        MakeInstances(TEXT("BuildingsSand"), CubeMesh, FLinearColor(0.45f, 0.38f, 0.28f), true),
        MakeInstances(TEXT("BuildingsSlate"), CubeMesh, FLinearColor(0.2f, 0.23f, 0.28f), true),
        MakeInstances(TEXT("BuildingsLight"), CubeMesh, FLinearColor(0.5f, 0.5f, 0.52f), true)
    };

    const float Lot = BlockSize * 0.5f;
    for (int32 I = -Half; I <= Half; ++I)
    {
        for (int32 J = -Half; J <= Half; ++J)
        {
            const FVector BlockCenter(I * Pitch, J * Pitch, 0.0f);
            const FTransform Slab(FRotator::ZeroRotator, BlockCenter + FVector(0.0f, 0.0f, SidewalkTopZ * 0.5f),
                                  FVector(BlockSize / 100.0f, BlockSize / 100.0f, SidewalkTopZ / 100.0f));

            if (I == 0 && J == 0)
            {
                Plaza->AddInstance(Slab);
                continue;
            }
            Sidewalks->AddInstance(Slab);

            const int32 Ring = FMath::Max(FMath::Abs(I), FMath::Abs(J));
            for (int32 SX = -1; SX <= 1; SX += 2)
            {
                for (int32 SY = -1; SY <= 1; SY += 2)
                {
                    const float Width = Lot * Random.FRandRange(0.62f, 0.88f);
                    const float Depth = Lot * Random.FRandRange(0.62f, 0.88f);
                    const float Height = Random.FRandRange(1800.0f, 5200.0f) + (Half - Ring + 1) * 1500.0f * Random.FRandRange(0.4f, 1.0f);
                    const FVector Center = BlockCenter + FVector(SX * Lot * 0.5f + Random.FRandRange(-60.0f, 60.0f),
                                                                 SY * Lot * 0.5f + Random.FRandRange(-60.0f, 60.0f),
                                                                 SidewalkTopZ + Height * 0.5f);
                    Buildings[Random.RandRange(0, 2)]->AddInstance(FTransform(FRotator::ZeroRotator, Center, FVector(Width / 100.0f, Depth / 100.0f, Height / 100.0f)));
                }
            }
        }
    }

    // Plaza dressing: fountain and a ring of trees.
    UInstancedStaticMeshComponent* Stone = MakeInstances(TEXT("PlazaStone"), CylinderMesh, FLinearColor(0.35f, 0.35f, 0.36f), true);
    UInstancedStaticMeshComponent* Water = MakeInstances(TEXT("PlazaWater"), CylinderMesh, FLinearColor(0.03f, 0.18f, 0.5f), false);
    Stone->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, SidewalkTopZ + 25.0f), FVector(8.0f, 8.0f, 0.5f)));
    Stone->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, SidewalkTopZ + 130.0f), FVector(1.2f, 1.2f, 2.0f)));
    Water->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, SidewalkTopZ + 52.0f), FVector(7.0f, 7.0f, 0.1f)));

    UInstancedStaticMeshComponent* Trunks = MakeInstances(TEXT("TreeTrunks"), CylinderMesh, FLinearColor(0.15f, 0.08f, 0.04f), true);
    UInstancedStaticMeshComponent* Crowns = MakeInstances(TEXT("TreeCrowns"), SphereMesh, FLinearColor(0.05f, 0.3f, 0.06f), false);
    for (int32 N = 0; N < 10; ++N)
    {
        const float Angle = FMath::DegreesToRadians(N * 36.0f + 18.0f);
        const FVector Base(FMath::Cos(Angle) * 2000.0f, FMath::Sin(Angle) * 2000.0f, SidewalkTopZ);
        Trunks->AddInstance(FTransform(FRotator::ZeroRotator, Base + FVector(0.0f, 0.0f, 150.0f), FVector(0.4f, 0.4f, 3.0f)));
        Crowns->AddInstance(FTransform(FRotator::ZeroRotator, Base + FVector(0.0f, 0.0f, 360.0f), FVector(3.2f, 3.2f, 3.0f)));
    }
}

void ASCTestWorldBuilder::SpawnGameplayActors()
{
    UWorld* World = GetWorld();
    if (!World) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    const float Pitch = BlockSize + RoadWidth;
    const float LaneOffset = 350.0f;
    FRandomStream Random(Seed + 1);

    // Save point on the plaza.
    World->SpawnActor<ASCSavePoint>(FVector(-500.0f, 700.0f, SidewalkTopZ), FRotator::ZeroRotator, Params);

    // Parked cars on the roads that surround the plaza (x,y,yaw).
    struct FCarSpawn { FVector2D Location; float Yaw; FLinearColor Color; };
    const float R = 0.5f * Pitch;
    const FCarSpawn Cars[] =
    {
        { FVector2D(R - LaneOffset, -1800.0f), 90.0f,  FLinearColor(0.65f, 0.04f, 0.04f) },
        { FVector2D(R + LaneOffset, 1200.0f), -90.0f,  FLinearColor(0.05f, 0.15f, 0.6f) },
        { FVector2D(-R + LaneOffset, 1800.0f), -90.0f, FLinearColor(0.9f, 0.7f, 0.05f) },
        { FVector2D(-R - LaneOffset, -1000.0f), 90.0f, FLinearColor(0.75f, 0.75f, 0.75f) },
        { FVector2D(-1500.0f, R - LaneOffset), 0.0f,   FLinearColor(0.02f, 0.02f, 0.02f) },
        { FVector2D(1800.0f, R + LaneOffset), 180.0f,  FLinearColor(0.05f, 0.45f, 0.4f) },
        { FVector2D(-1800.0f, -R - LaneOffset), 0.0f,  FLinearColor(0.85f, 0.35f, 0.04f) },
        { FVector2D(2200.0f, -R + LaneOffset), 180.0f, FLinearColor(0.35f, 0.08f, 0.5f) }
    };
    for (const FCarSpawn& Car : Cars)
    {
        const FVector Location(Car.Location.X, Car.Location.Y, RoadTopZ + 70.0f);
        if (ASCVehicle* Vehicle = World->SpawnActor<ASCVehicle>(Location, FRotator(0.0f, Car.Yaw, 0.0f), Params))
        {
            Vehicle->SetBodyColor(Car.Color);
        }
    }

    // Pedestrians strolling the roads.
    for (int32 N = 0; N < CivilianCount; ++N)
    {
        FVector Location;
        if (GetRandomRoadPoint(FVector::ZeroVector, 1200.0f, 3.0f * Pitch, Location))
        {
            if (ASCNPC* Civilian = World->SpawnActor<ASCNPC>(Location, FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Params))
            {
                Civilian->SetRole(ESCNPCRole::Civilian);
            }
        }
    }

    // Cash on the plaza...
    for (int32 N = 0; N < 6; ++N)
    {
        const float Angle = FMath::DegreesToRadians(N * 60.0f);
        if (ASCMoneyPickup* Pickup = World->SpawnActor<ASCMoneyPickup>(FVector(FMath::Cos(Angle) * 1300.0f, FMath::Sin(Angle) * 1300.0f, SidewalkTopZ + 60.0f), FRotator::ZeroRotator, Params))
        {
            Pickup->Value = 100;
        }
    }

    // ...and scattered along the roads near the centre.
    for (int32 N = 0; N < RoadsideMoneyPickups; ++N)
    {
        const bool bAlongX = Random.FRand() < 0.5f;
        const int32 K = Random.RandRange(-2, 1);
        const float C = (K + 0.5f) * Pitch;
        const float T = Random.FRandRange(-2.5f * Pitch, 2.5f * Pitch);
        const float Lane = Random.FRand() < 0.5f ? LaneOffset : -LaneOffset;
        const FVector Location = bAlongX ? FVector(T, C + Lane, RoadTopZ + 60.0f) : FVector(C + Lane, T, RoadTopZ + 60.0f);

        if (ASCMoneyPickup* Pickup = World->SpawnActor<ASCMoneyPickup>(Location, FRotator::ZeroRotator, Params))
        {
            Pickup->Value = 50 * Random.RandRange(2, 12);
        }
    }
}
