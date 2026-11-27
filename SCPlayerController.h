#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SCPlayerController.generated.h"

class ASCPlayerCharacter;

UCLASS()
class SANCLEMENTE_API ASCPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    /** Pushes a toast message onto the HUD. */
    void ShowMessage(const FString& Message, float Duration = 3.0f) const;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    void QuickSave();
    void QuickLoad();

    UPROPERTY(EditDefaultsOnly, Category="Camera")
    float MinPitch = -55.0f;

    UPROPERTY(EditDefaultsOnly, Category="Camera")
    float MaxPitch = 35.0f;
};
