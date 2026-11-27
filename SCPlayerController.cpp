#include "SCPlayerController.h"
#include "SCHUD.h"
#include "SCPlayerCharacter.h"
#include "SCSaveSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"

void ASCPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (PlayerCameraManager)
    {
        PlayerCameraManager->ViewPitchMin = MinPitch;
        PlayerCameraManager->ViewPitchMax = MaxPitch;
    }
}

void ASCPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Bound on the controller so save/load work both on foot and in a vehicle.
    InputComponent->BindAction(TEXT("QuickSave"), IE_Pressed, this, &ASCPlayerController::QuickSave);
    InputComponent->BindAction(TEXT("QuickLoad"), IE_Pressed, this, &ASCPlayerController::QuickLoad);
}

void ASCPlayerController::ShowMessage(const FString& Message, float Duration) const
{
    if (ASCHUD* SCHUD = Cast<ASCHUD>(GetHUD()))
    {
        SCHUD->PushMessage(Message, Duration);
    }
}

void ASCPlayerController::QuickSave()
{
    ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(GetPawn());
    if (!Character)
    {
        ShowMessage(TEXT("Get out of the vehicle to save"));
        return;
    }
    if (Character->IsDead())
    {
        return;
    }

    USCSaveSubsystem* SaveSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<USCSaveSubsystem>() : nullptr;
    const bool bSaved = SaveSubsystem && SaveSubsystem->SavePlayerState(Character, Character->GetMoney());
    ShowMessage(bSaved ? TEXT("Game saved") : TEXT("Save failed"));
}

void ASCPlayerController::QuickLoad()
{
    ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(GetPawn());
    if (!Character)
    {
        ShowMessage(TEXT("Get out of the vehicle to load"));
        return;
    }

    USCSaveSubsystem* SaveSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<USCSaveSubsystem>() : nullptr;
    if (!SaveSubsystem || !SaveSubsystem->DoesSaveExist())
    {
        ShowMessage(TEXT("No saved game found"));
        return;
    }

    int32 LoadedMoney = 0;
    const bool bLoaded = SaveSubsystem->LoadPlayerState(Character, LoadedMoney);
    ShowMessage(bLoaded ? TEXT("Game loaded") : TEXT("Load failed"));
}
