#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SCSaveSubsystem.generated.h"

class ACharacter;
class USCGameSave;

UCLASS()
class SANCLEMENTE_API USCSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Writes location, rotation, money (and health / time of day when available) to the slot. */
    UFUNCTION(BlueprintCallable, Category="Save")
    bool SavePlayerState(ACharacter* Character, int32 Money, const FString& SlotName = TEXT("PlayerSlot"));

    /** Teleports the character to the saved state and restores money/health/time of day. */
    UFUNCTION(BlueprintCallable, Category="Save")
    bool LoadPlayerState(ACharacter* Character, int32& OutMoney, const FString& SlotName = TEXT("PlayerSlot"));

    UFUNCTION(BlueprintCallable, Category="Save")
    bool DoesSaveExist(const FString& SlotName = TEXT("PlayerSlot")) const;

    UFUNCTION(BlueprintCallable, Category="Save")
    bool DeleteSave(const FString& SlotName = TEXT("PlayerSlot"));

private:
    static constexpr int32 UserIndex = 0;
};
