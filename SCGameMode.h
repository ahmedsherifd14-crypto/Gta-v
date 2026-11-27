#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SCGameMode.generated.h"

class APlayerStart;

UCLASS(Config=Game)
class SANCLEMENTE_API ASCGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASCGameMode();

    virtual void StartPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    /** In-game clock, 0..24 hours. */
    UFUNCTION(BlueprintPure, Category="World")
    float GetTimeOfDayHours() const { return TimeOfDayHours; }

    UFUNCTION(BlueprintCallable, Category="World")
    void SetTimeOfDayHours(float NewHours);

    /** "HH:MM" */
    UFUNCTION(BlueprintPure, Category="World")
    FString GetClockString() const;

    // ---- Wanted level (0..5 stars) ----
    /** Adds crime points (hit civilian 1, kill civilian 3, hit cop 3, kill cop 6). */
    UFUNCTION(BlueprintCallable, Category="Wanted")
    void ReportCrime(int32 Points);

    UFUNCTION(BlueprintPure, Category="Wanted")
    int32 GetWantedLevel() const;

    UFUNCTION(BlueprintCallable, Category="Wanted")
    void ClearWanted();

    /** Seconds without new crimes before the heat starts to cool down. */
    UPROPERTY(EditDefaultsOnly, Category="Wanted") float WantedCooldownDelay = 20.0f;
    /** Crime points lost per second once cooling down. */
    UPROPERTY(EditDefaultsOnly, Category="Wanted") float WantedDecayPerSecond = 0.8f;
    /** Police units wanted per star. */
    UPROPERTY(EditDefaultsOnly, Category="Wanted") int32 PolicePerStar = 2;

    /**
     * When true (default) the game mode builds the procedural test city on whatever map is loaded
     * (the engine's blank Entry map by default) and uses its own PlayerStart. Set to False for maps
     * that contain your own authored geometry and PlayerStarts.
     */
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category="World")
    bool bUseGeneratedTestWorld = true;

    /** Real seconds that pass per in-game minute (2.0 = a 48 minute day). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
    float RealSecondsPerGameMinute = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
    float TimeOfDayHours = 9.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
    FVector GeneratedSpawnLocation = FVector(-900.0f, 0.0f, 130.0f);

private:
    bool HasPlacedBuilder() const;
    void EnsureTestWorld();
    void UpdatePolice(float DeltaSeconds);
    void SpawnPoliceUnit();

    float CrimePoints = 0.0f;
    float LastCrimeTime = -1000.0f;
    float PoliceSpawnTimer = 2.0f;

    UPROPERTY(Transient)
    TObjectPtr<APlayerStart> GeneratedPlayerStart;
};
