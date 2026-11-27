#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/TimerHandle.h"
#include "SCNPC.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UAnimInstance;
class USkeletalMesh;
class ASCPlayerCharacter;

UENUM(BlueprintType)
enum class ESCNPCRole : uint8
{
    Civilian,
    Police
};

/**
 * Street pedestrian or police officer with simple navmesh-free AI:
 * civilians wander and flee from gunfire; police hunt the player while the wanted level is above 0.
 * Real characters: set CivilianMesh/PoliceMesh/AnimClass (defaults point at the UE5 mannequin if present).
 */
UCLASS()
class SANCLEMENTE_API ASCNPC : public ACharacter
{
    GENERATED_BODY()

public:
    ASCNPC();

    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    /** Configure after spawning. */
    void SetRole(ESCNPCRole NewRole);

    UFUNCTION(BlueprintPure, Category="NPC") ESCNPCRole GetRole() const { return Role; }
    UFUNCTION(BlueprintPure, Category="NPC") bool IsDead() const { return bDead; }

protected:
    virtual void BeginPlay() override;

    void ApplyLook();
    void TickCivilian(float DeltaSeconds, ASCPlayerCharacter* Player);
    void TickPolice(float DeltaSeconds, ASCPlayerCharacter* Player, int32 WantedLevel);
    void WanderStep(float DeltaSeconds, float Speed);
    void MoveAlong(const FVector& Direction, float Speed);
    void Die(bool bKilledByPlayer);

    ASCPlayerCharacter* FindPlayer() const;

    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<USceneComponent> VisualRoot;
    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<UStaticMeshComponent> BodyMesh;
    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<UStaticMeshComponent> HeadMesh;
    UPROPERTY(VisibleAnywhere, Category="Visual") TObjectPtr<UStaticMeshComponent> FacingMarker;

    UPROPERTY(EditAnywhere, Category="NPC") ESCNPCRole Role = ESCNPCRole::Civilian;
    UPROPERTY(EditAnywhere, Category="NPC") float CivilianHealth = 60.0f;
    UPROPERTY(EditAnywhere, Category="NPC") float PoliceHealth = 120.0f;
    UPROPERTY(EditAnywhere, Category="NPC") float WalkSpeed = 140.0f;
    UPROPERTY(EditAnywhere, Category="NPC") float RunSpeed = 520.0f;
    UPROPERTY(EditAnywhere, Category="NPC") float PoliceChaseSpeed = 480.0f;

    // Police combat
    UPROPERTY(EditAnywhere, Category="Police") float PoliceDamage = 7.0f;
    UPROPERTY(EditAnywhere, Category="Police") float PoliceHitChance = 0.3f;
    UPROPERTY(EditAnywhere, Category="Police") float PoliceFireRange = 3500.0f;
    UPROPERTY(EditAnywhere, Category="Police") float PoliceDespawnDistance = 10000.0f;

    // Real characters (optional art)
    UPROPERTY(EditDefaultsOnly, Category="Visual|Real Character")
    TSoftObjectPtr<USkeletalMesh> CharacterSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")));

    UPROPERTY(EditDefaultsOnly, Category="Visual|Real Character")
    TSoftClassPtr<UAnimInstance> CharacterAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C")));

    UPROPERTY(EditDefaultsOnly, Category="Visual|Real Character")
    float CharacterMeshYawOffset = -90.0f;

private:
    float Health = 60.0f;
    bool bDead = false;
    bool bUsingRealMesh = false;

    // Wander / flee state
    FVector WanderDirection = FVector::ForwardVector;
    float NextRepickTime = 0.0f;
    float FleeUntilTime = 0.0f;
    float StuckTime = 0.0f;
    float NextShotTime = 0.0f;
    float NoWantedSince = -1.0f;
};
