#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SCInteractable.generated.h"

class APawn;

UINTERFACE(MinimalAPI, Blueprintable)
class USCInteractable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Anything the player can use with the Interact key (vehicles, save points, doors, ...).
 * The player character scans for the nearest valid interactable and shows its prompt on the HUD.
 */
class SANCLEMENTE_API ISCInteractable
{
    GENERATED_BODY()

public:
    /** Whether the pawn may interact right now (e.g. a car that already has a driver returns false). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
    bool CanInteract(APawn* InteractingPawn) const;

    /** Short verb phrase shown after "[E]" on the HUD, e.g. "Enter Vehicle". */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
    FText GetInteractionPrompt(APawn* InteractingPawn) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
    void Interact(APawn* InteractingPawn);
};
