#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SCMoneyPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

/** Cash bag: walk (or drive) over it to collect. */
UCLASS()
class SANCLEMENTE_API ASCMoneyPickup : public AActor
{
    GENERATED_BODY()

public:
    ASCMoneyPickup();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
    int32 Value = 250;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                         int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditAnywhere, Category="Pickup")
    float SpinDegreesPerSecond = 90.0f;

private:
    bool bCollected = false;
};
