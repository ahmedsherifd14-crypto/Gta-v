#include "SCSavePoint.h"
#include "SCBasicShapes.h"
#include "SCPlayerCharacter.h"
#include "SCSaveSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

ASCSavePoint::ASCSavePoint()
{
    PrimaryActorTick.bCanEverTick = false;

    // Actor origin sits on the ground; the pillar is lifted so its base touches the floor.
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    Pillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar"));
    Pillar->SetupAttachment(Root);
    Pillar->SetStaticMesh(SCShapes::Cylinder());
    Pillar->SetMaterial(0, SCShapes::BasicMaterial());
    Pillar->SetRelativeScale3D(FVector(0.8f, 0.8f, 2.0f));
    Pillar->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    Pillar->SetCollisionProfileName(TEXT("BlockAll"));

    Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
    Beacon->SetupAttachment(Pillar);
    Beacon->SetStaticMesh(SCShapes::Sphere());
    Beacon->SetMaterial(0, SCShapes::BasicMaterial());
    Beacon->SetRelativeLocation(FVector(0.0f, 0.0f, 62.0f));
    Beacon->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.3f));
    Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASCSavePoint::BeginPlay()
{
    Super::BeginPlay();

    if (UMaterialInstanceDynamic* Material = Pillar->CreateDynamicMaterialInstance(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.35f, 0.5f));
    }
    if (UMaterialInstanceDynamic* Material = Beacon->CreateDynamicMaterialInstance(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.9f, 1.0f));
    }
}

bool ASCSavePoint::CanInteract_Implementation(APawn* InteractingPawn) const
{
    return Cast<ASCPlayerCharacter>(InteractingPawn) != nullptr;
}

FText ASCSavePoint::GetInteractionPrompt_Implementation(APawn* InteractingPawn) const
{
    return FText::FromString(TEXT("Save Game"));
}

void ASCSavePoint::Interact_Implementation(APawn* InteractingPawn)
{
    ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(InteractingPawn);
    if (!Character) return;

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    USCSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<USCSaveSubsystem>() : nullptr;

    const bool bSaved = SaveSubsystem && SaveSubsystem->SavePlayerState(Character, Character->GetMoney());
    Character->ShowMessage(bSaved ? TEXT("Game saved") : TEXT("Save failed"));
}
