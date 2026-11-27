#include "SCMoneyPickup.h"
#include "SCBasicShapes.h"
#include "SCPlayerCharacter.h"
#include "SCVehicle.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ASCMoneyPickup::ASCMoneyPickup()
{
    PrimaryActorTick.bCanEverTick = true;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    Trigger->InitSphereRadius(110.0f);
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetGenerateOverlapEvents(true);
    RootComponent = Trigger;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Trigger);
    Mesh->SetStaticMesh(SCShapes::Cube());
    Mesh->SetMaterial(0, SCShapes::BasicMaterial());
    Mesh->SetRelativeScale3D(FVector(0.45f, 0.28f, 0.14f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASCMoneyPickup::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInstanceDynamic* Material = Mesh->CreateDynamicMaterialInstance(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.65f, 0.15f));
    }

    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ASCMoneyPickup::OnPickupOverlap);
}

void ASCMoneyPickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Mesh->AddLocalRotation(FRotator(0.0f, SpinDegreesPerSecond * DeltaSeconds, 0.0f));
}

void ASCMoneyPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                     int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bCollected || !OtherActor) return;

    ASCPlayerCharacter* Collector = Cast<ASCPlayerCharacter>(OtherActor);
    if (!Collector)
    {
        if (const ASCVehicle* Vehicle = Cast<ASCVehicle>(OtherActor))
        {
            Collector = Vehicle->GetDriver();
        }
    }
    if (!Collector || Collector->IsDead()) return;

    bCollected = true;
    Collector->AddMoney(Value);
    Collector->ShowMessage(FString::Printf(TEXT("+$%d"), Value), 1.5f);
    Destroy();
}
