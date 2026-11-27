#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SCGameSave.generated.h"

UCLASS()
class SANCLEMENTE_API USCGameSave : public USaveGame
{
    GENERATED_BODY()

public:
    /** Bump when the layout changes; the loader accepts 1..CurrentSaveVersion. */
    static constexpr int32 CurrentSaveVersion = 2;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    FVector PlayerLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    FRotator PlayerRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    int32 Money = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    FString CurrentMissionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    TArray<FString> CompletedMissionIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    int32 SaveVersion = 1;

    /** Added in save version 2. Version 1 files load with these defaults. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    float Health = 100.0f;

    /** Added in save version 2. In-game clock, 0..24. */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Save")
    float TimeOfDayHours = 9.0f;
};
