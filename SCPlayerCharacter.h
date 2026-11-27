#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/TimerHandle.h"
#include "SCPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;
class ASCVehicle;
class USCWeaponComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimInstance;

UCLASS()
class SANCLEMENTE_API ASCPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASCPlayerCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

    // ---- Wallet / vitals ----
    UFUNCTION(BlueprintPure, Category="Player") int32 GetMoney() const { return Money; }
    UFUNCTION(BlueprintCallable, Category="Player") void SetMoney(int32 NewMoney) { Money = FMath::Max(0, NewMoney); }
    UFUNCTION(BlueprintCallable, Category="Player") void AddMoney(int32 Amount) { SetMoney(Money + Amount); }

    UFUNCTION(BlueprintPure, Category="Player") float GetHealth() const { return Health; }
    UFUNCTION(BlueprintPure, Category="Player") float GetMaxHealth() const { return MaxHealth; }
    UFUNCTION(BlueprintPure, Category="Player") float GetStamina() const { return Stamina; }
    UFUNCTION(BlueprintPure, Category="Player") float GetMaxStamina() const { return MaxStamina; }
    UFUNCTION(BlueprintPure, Category="Player") bool IsDead() const { return bIsDead; }
    UFUNCTION(BlueprintPure, Category="Player") bool IsSprinting() const { return bSprinting; }

    /** Applies a loaded save: wallet + health, and clears any death state. */
    UFUNCTION(BlueprintCallable, Category="Player")
    void RestoreFromSave(int32 SavedMoney, float SavedHealth);

    // ---- Interaction ----
    UFUNCTION(BlueprintPure, Category="Interaction") AActor* GetFocusedInteractable() const { return FocusedInteractable.Get(); }
    UFUNCTION(BlueprintPure, Category="Interaction") FText GetFocusedPrompt() const { return FocusedPrompt; }

    // ---- Vehicle link (set by ASCVehicle) ----
    void SetCurrentVehicle(ASCVehicle* Vehicle);
    UFUNCTION(BlueprintPure, Category="Vehicle") bool IsInVehicle() const { return CurrentVehicle.IsValid(); }

    /** Shows a short toast on the HUD (no-op if there is no local player controller). */
    void ShowMessage(const FString& Message, float Duration = 3.0f) const;

    // ---- Combat ----
    UFUNCTION(BlueprintPure, Category="Combat") USCWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }
    UFUNCTION(BlueprintPure, Category="Combat") bool IsAiming() const { return bAimHeld; }
    UFUNCTION(BlueprintPure, Category="Combat") FVector GetMuzzleLocation() const;

    /** Called by the weapon component whenever the active weapon changes (updates the held-weapon visual). */
    void OnWeaponEquipped(int32 WeaponIndex);

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void TurnRate(float Value);
    void LookUpRate(float Value);
    void CameraZoom(float Value);
    void StartJump();
    void StopJump();
    void StartSprint();
    void StopSprint();
    void ToggleCrouch();
    void Interact();

    void FirePressed();
    void FireReleased();
    void AimPressed();
    void AimReleased();
    void ReloadPressed();
    void NextWeaponPressed();
    void SelectWeapon1() { SelectWeapon(0); }
    void SelectWeapon2() { SelectWeapon(1); }
    void SelectWeapon3() { SelectWeapon(2); }
    void SelectWeapon4() { SelectWeapon(3); }
    void SelectWeapon(int32 Index);
    void RefreshCombatStance();
    void ApplyRealCharacterMesh();

    void UpdateSprint(float DeltaSeconds);
    void UpdateFocusedInteractable();
    void Die();
    void Respawn();

    // ---- Components ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    /** Placeholder body (engine primitives) so the character is visible without any art assets. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
    TObjectPtr<USceneComponent> VisualRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
    TObjectPtr<UStaticMeshComponent> HeadMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
    TObjectPtr<UStaticMeshComponent> FacingMarker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<USCWeaponComponent> WeaponComponent;

    /** Placeholder gun model shown when no real weapon mesh is assigned. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
    TObjectPtr<UStaticMeshComponent> PlaceholderGun;

    /** Real weapon mesh component (attached to the hand socket when a weapon mesh asset is assigned). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
    TObjectPtr<USkeletalMeshComponent> HeldWeaponMesh;

    // ---- Real character (optional art) ----
    // Defaults point at the UE5 Third Person template mannequin. If those assets exist in your project
    // (Add > Add Feature or Content Pack > Third Person) the character uses them automatically; otherwise
    // the primitive placeholder body stays. Change these to any skeletal mesh / anim blueprint
    // (MetaHuman, Mixamo, Fab/Marketplace characters...).
    UPROPERTY(EditDefaultsOnly, Category="Visual|Real Character")
    TSoftObjectPtr<USkeletalMesh> CharacterSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));

    UPROPERTY(EditDefaultsOnly, Category="Visual|Real Character")
    TSoftClassPtr<UAnimInstance> CharacterAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C")));

    /** Socket on the skeleton where weapons are attached. */
    UPROPERTY(EditDefaultsOnly, Category="Visual|Real Character")
    FName WeaponSocketName = TEXT("hand_r");

    /** Mannequin faces +Y; most imported characters need -90 yaw so they face +X. */
    UPROPERTY(EditDefaultsOnly, Category="Visual|Real Character")
    float CharacterMeshYawOffset = -90.0f;

    // ---- Aiming camera ----
    UPROPERTY(EditDefaultsOnly, Category="Camera") float AimArmLength = 140.0f;
    UPROPERTY(EditDefaultsOnly, Category="Camera") float DefaultFOV = 90.0f;
    UPROPERTY(EditDefaultsOnly, Category="Camera") float AimFOV = 62.0f;

    // ---- Movement ----
    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float WalkSpeed = 450.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float SprintSpeed = 720.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float CrouchSpeed = 220.0f;

    UPROPERTY(EditDefaultsOnly, Category="Movement")
    float GamepadLookRate = 120.0f;

    UPROPERTY(EditDefaultsOnly, Category="Camera")
    float MinCameraDistance = 180.0f;

    UPROPERTY(EditDefaultsOnly, Category="Camera")
    float MaxCameraDistance = 600.0f;

    // ---- Vitals ----
    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float MaxHealth = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float MaxStamina = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float StaminaDrainPerSecond = 14.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float StaminaRegenPerSecond = 18.0f;

    /** After running dry, sprinting is locked until stamina climbs back above this. */
    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float SprintResumeStamina = 25.0f;

    /** Fall speed (cm/s) above which landing hurts. A normal jump lands at ~620. */
    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float FallDamageSpeedThreshold = 1200.0f;

    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float FallDamagePerSpeed = 0.05f;

    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    float RespawnDelay = 3.0f;

    /** Cash lost when you "wake up at the hospital". */
    UPROPERTY(EditDefaultsOnly, Category="Vitals")
    int32 RespawnFee = 250;

    // ---- Interaction ----
    UPROPERTY(EditDefaultsOnly, Category="Interaction")
    float InteractRadius = 260.0f;

private:
    int32 Money = 0;
    float Health = 100.0f;
    float Stamina = 100.0f;
    float PeakFallSpeed = 0.0f;
    bool bWantsToSprint = false;
    bool bSprinting = false;
    bool bSprintLocked = false;
    bool bIsDead = false;
    bool bFireHeld = false;
    bool bAimHeld = false;
    bool bUsingRealMesh = false;
    float DesiredArmLength = 320.0f;
    float InteractScanTimer = 0.0f;
    FTimerHandle RespawnTimerHandle;

    TWeakObjectPtr<AActor> FocusedInteractable;
    FText FocusedPrompt;
    TWeakObjectPtr<ASCVehicle> CurrentVehicle;
};
