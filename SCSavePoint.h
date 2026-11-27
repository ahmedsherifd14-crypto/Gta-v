#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SCInteractable.h"
#include "SCSavePoint.generated.h"

class UStaticMeshComponent;

/** Safehouse marker: interact to save your game. */
UCLASS()
class SANCLEMENTE_API ASCSavePoint : public AActor, public ISCInteractable
{
    GENERATED_BODY()

public:
    ASCSavePoint();

    virtual bool CanInteract_Implementation(APawn* InteractingPawn) const override;
    virtual FText GetInteractionPrompt_Implementation(APawn* InteractingPawn) const override;
    virtual void Interact_Implementation(APawn* InteractingPawn) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SavePoint")
    TObjectPtr<UStaticMeshComponent> Pillar;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SavePoint")
    TObjectPtr<UStaticMeshComponent> Beacon;
};
