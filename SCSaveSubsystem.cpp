#include "SCSaveSubsystem.h"
#include "SCGameSave.h"
#include "SCGameMode.h"
#include "SCPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

bool USCSaveSubsystem::SavePlayerState(ACharacter* Character, int32 Money, const FString& SlotName)
{
    if (!IsValid(Character) || SlotName.IsEmpty()) return false;

    USCGameSave* SaveObject = Cast<USCGameSave>(UGameplayStatics::CreateSaveGameObject(USCGameSave::StaticClass()));
    if (!SaveObject) return false;

    SaveObject->SaveVersion = USCGameSave::CurrentSaveVersion;
    SaveObject->PlayerLocation = Character->GetActorLocation();
    SaveObject->PlayerRotation = Character->GetActorRotation();
    SaveObject->Money = FMath::Max(0, Money);

    if (const ASCPlayerCharacter* SCCharacter = Cast<ASCPlayerCharacter>(Character))
    {
        SaveObject->Health = SCCharacter->GetHealth();
    }

    if (const UWorld* World = Character->GetWorld())
    {
        if (const ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>())
        {
            SaveObject->TimeOfDayHours = GameMode->GetTimeOfDayHours();
        }
    }

    return UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, UserIndex);
}

bool USCSaveSubsystem::LoadPlayerState(ACharacter* Character, int32& OutMoney, const FString& SlotName)
{
    if (!IsValid(Character) || SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex)) return false;

    USCGameSave* SaveObject = Cast<USCGameSave>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
    if (!SaveObject || SaveObject->SaveVersion < 1 || SaveObject->SaveVersion > USCGameSave::CurrentSaveVersion) return false;

    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }

    Character->SetActorLocation(SaveObject->PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Character->SetActorRotation(FRotator(0.0f, SaveObject->PlayerRotation.Yaw, 0.0f));

    if (AController* Controller = Character->GetController())
    {
        Controller->SetControlRotation(FRotator(-10.0f, SaveObject->PlayerRotation.Yaw, 0.0f));
    }

    OutMoney = SaveObject->Money;

    if (ASCPlayerCharacter* SCCharacter = Cast<ASCPlayerCharacter>(Character))
    {
        SCCharacter->RestoreFromSave(SaveObject->Money, SaveObject->Health);
    }

    if (UWorld* World = Character->GetWorld())
    {
        if (ASCGameMode* GameMode = World->GetAuthGameMode<ASCGameMode>())
        {
            GameMode->SetTimeOfDayHours(SaveObject->TimeOfDayHours);
        }
    }

    return true;
}

bool USCSaveSubsystem::DoesSaveExist(const FString& SlotName) const
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool USCSaveSubsystem::DeleteSave(const FString& SlotName)
{
    return !SlotName.IsEmpty() && UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
}
