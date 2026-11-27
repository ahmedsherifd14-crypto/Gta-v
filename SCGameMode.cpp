#include "SCGameMode.h"
#include "SCPlayerCharacter.h"
#include "SCPlayerController.h"
#include "SCHUD.h"
#include "SCNPC.h"
#include "SCTestWorldBuilder.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

ASCGameMode::ASCGameMode()
{
    PrimaryActorTick.bCanEverTick = true;

    DefaultPawnClass = ASCPlayerCharacter::StaticClass();
    PlayerControllerClass = ASCPlayerController::StaticClass();
    HUDClass = ASCHUD::StaticClass();
}

bool ASCGameMode::HasPlacedBuilder() const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    for (TActorIterator<ASCTestWorldBuilder> It(World); It; ++It)
    {
        return true;
    }
    return false;
}

AActor* ASCGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // Player login happens before BeginPlay, so the generated world does not exist yet.
    // Provide our own PlayerStart so spawning never fails on an empty map.
    if (bUseGeneratedTestWorld && !HasPlacedBuilder())
    {
        if (!IsValid(GeneratedPlayerStart))
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            GeneratedPlayerStart = GetWorld()->SpawnActor<APlayerStart>(GeneratedSpawnLocation, FRotator::ZeroRotator, Params);
        }
        if (IsValid(GeneratedPlayerStart))
        {
            return GeneratedPlayerStart;
        }
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}

void ASCGameMode::StartPlay()
{
    Super::StartPlay();

    if (bUseGeneratedTestWorld)
    {
        EnsureTestWorld();
    }
}

void ASCGameMode::EnsureTestWorld()
{
    if (HasPlacedBuilder())
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<ASCTestWorldBuilder>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

void ASCGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (RealSecondsPerGameMinute > KINDA_SMALL_NUMBER)
    {
        SetTimeOfDayHours(TimeOfDayHours + DeltaSeconds / (60.0f * RealSecondsPerGameMinute));
    }

    // Heat cools down after a while without new crimes.
    if (CrimePoints > 0.0f && GetWorld()->GetTimeSeconds() - LastCrimeTime > WantedCooldownDelay)
    {
        CrimePoints = FMath::Max(0.0f, CrimePoints - WantedDecayPerSecond * DeltaSeconds);
    }

    UpdatePolice(DeltaSeconds);
}

void ASCGameMode::ReportCrime(int32 Points)
{
    if (Points <= 0 || !GetWorld()) return;
    CrimePoints = FMath::Min(CrimePoints + Points, 60.0f);
    LastCrimeTime = GetWorld()->GetTimeSeconds();
}

int32 ASCGameMode::GetWantedLevel() const
{
    // Star thresholds in crime points.
    static const float Thresholds[5] = { 1.0f, 5.0f, 12.0f, 22.0f, 35.0f };
    int32 Level = 0;
    for (int32 i = 0; i < 5; ++i)
    {
        if (CrimePoints >= Thresholds[i]) Level = i + 1;
    }
    return Level;
}

void ASCGameMode::ClearWanted()
{
    CrimePoints = 0.0f;
}

void ASCGameMode::UpdatePolice(float DeltaSeconds)
{
    PoliceSpawnTimer -= DeltaSeconds;
    if (PoliceSpawnTimer > 0.0f) return;
    PoliceSpawnTimer = 2.5f;

    const int32 Wanted = GetWantedLevel();
    if (Wanted <= 0) return;

    int32 ActivePolice = 0;
    for (TActorIterator<ASCNPC> It(GetWorld()); It; ++It)
    {
        if (It->GetRole() == ESCNPCRole::Police && !It->IsDead()) ++ActivePolice;
    }

    if (ActivePolice < Wanted * PolicePerStar)
    {
        SpawnPoliceUnit();
    }
}

void ASCGameMode::SpawnPoliceUnit()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return;

    FVector SpawnLocation = FVector::ZeroVector;
    bool bFound = false;

    // Prefer a spot on a road a few seconds away from the player.
    for (TActorIterator<ASCTestWorldBuilder> It(GetWorld()); It && !bFound; ++It)
    {
        bFound = It->GetRandomRoadPoint(PlayerPawn->GetActorLocation(), 3000.0f, 5500.0f, SpawnLocation);
    }
    if (!bFound)
    {
        const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
        SpawnLocation = PlayerPawn->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * 4000.0f;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (ASCNPC* Officer = GetWorld()->SpawnActor<ASCNPC>(SpawnLocation, FRotator::ZeroRotator, Params))
    {
        Officer->SetRole(ESCNPCRole::Police);
    }
}

void ASCGameMode::SetTimeOfDayHours(float NewHours)
{
    TimeOfDayHours = FMath::Fmod(FMath::Max(0.0f, NewHours), 24.0f);
}

FString ASCGameMode::GetClockString() const
{
    const int32 TotalMinutes = FMath::FloorToInt(TimeOfDayHours * 60.0f);
    return FString::Printf(TEXT("%02d:%02d"), (TotalMinutes / 60) % 24, TotalMinutes % 60);
}
