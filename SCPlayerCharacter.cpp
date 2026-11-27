#include "SCPlayerCharacter.h"
#include "SCBasicShapes.h"
#include "SCInteractable.h"
#include "SCPlayerController.h"
#include "SCVehicle.h"
#include "SCWeaponComponent.h"
#include "SCGameMode.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace
{
    constexpr float CapsuleRadius = 42.0f;
    constexpr float CapsuleHalfHeight = 96.0f;

    void SetupVisualPart(UStaticMeshComponent* Part, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
    {
        Part->SetStaticMesh(Mesh);
        Part->SetMaterial(0, SCShapes::BasicMaterial());
        Part->SetRelativeLocation(Location);
        Part->SetRelativeScale3D(Scale);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
    }

    void TintPart(UStaticMeshComponent* Part, const FLinearColor& Color)
    {
        if (UMaterialInstanceDynamic* Material = Part->CreateDynamicMaterialInstance(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
        }
    }
}

ASCPlayerCharacter::ASCPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

    // Over-the-shoulder camera.
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 65.0f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 14.0f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    Movement->JumpZVelocity = 620.0f;
    Movement->AirControl = 0.35f;
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->MaxWalkSpeedCrouched = CrouchSpeed;
    Movement->CrouchedHalfHeight = 60.0f;
    Movement->bCrouchMaintainsBaseLocation = true;
    Movement->NavAgentProps.bCanCrouch = true;

    // Placeholder body built from engine primitives (feet at the capsule bottom).
    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
    VisualRoot->SetupAttachment(GetCapsuleComponent());
    VisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -CapsuleHalfHeight));

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(VisualRoot);
    SetupVisualPart(BodyMesh, SCShapes::Cylinder(), FVector(0.0f, 0.0f, 72.0f), FVector(0.7f, 0.7f, 1.44f));

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(VisualRoot);
    SetupVisualPart(HeadMesh, SCShapes::Sphere(), FVector(0.0f, 0.0f, 162.0f), FVector(0.36f, 0.36f, 0.36f));

    FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
    FacingMarker->SetupAttachment(VisualRoot);
    SetupVisualPart(FacingMarker, SCShapes::Cube(), FVector(30.0f, 0.0f, 120.0f), FVector(0.3f, 0.5f, 0.12f));

    // Placeholder gun (right hand, pointing forward) and optional real weapon mesh.
    PlaceholderGun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderGun"));
    PlaceholderGun->SetupAttachment(VisualRoot);
    SetupVisualPart(PlaceholderGun, SCShapes::Cube(), FVector(48.0f, 22.0f, 110.0f), FVector(0.55f, 0.07f, 0.12f));

    HeldWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeldWeaponMesh"));
    HeldWeaponMesh->SetupAttachment(GetMesh());
    HeldWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeldWeaponMesh->SetVisibility(false);

    WeaponComponent = CreateDefaultSubobject<USCWeaponComponent>(TEXT("WeaponComponent"));
}

void ASCPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    Health = MaxHealth;
    Stamina = MaxStamina;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    TintPart(BodyMesh, FLinearColor(0.05f, 0.18f, 0.45f));
    TintPart(HeadMesh, FLinearColor(0.55f, 0.36f, 0.25f));
    TintPart(FacingMarker, FLinearColor(0.9f, 0.9f, 0.9f));
    TintPart(PlaceholderGun, FLinearColor(0.02f, 0.02f, 0.02f));

    DesiredArmLength = CameraBoom->TargetArmLength;
    DefaultFOV = FollowCamera->FieldOfView;

    ApplyRealCharacterMesh();
}

void ASCPlayerCharacter::ApplyRealCharacterMesh()
{
    if (CharacterSkeletalMesh.IsNull()) return;

    USkeletalMesh* LoadedMesh = CharacterSkeletalMesh.LoadSynchronous();
    if (!LoadedMesh)
    {
        UE_LOG(LogTemp, Log, TEXT("SanClemente: character mesh '%s' not found - using placeholder body."), *CharacterSkeletalMesh.ToString());
        return;
    }

    USkeletalMeshComponent* SkeletalMesh = GetMesh();
    SkeletalMesh->SetSkeletalMesh(LoadedMesh);
    SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -GetDefaultHalfHeight()), FRotator(0.0f, CharacterMeshYawOffset, 0.0f));
    SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (UClass* AnimClass = CharacterAnimClass.LoadSynchronous())
    {
        SkeletalMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        SkeletalMesh->SetAnimInstanceClass(AnimClass);
    }

    // Hide the primitive body, keep the real one.
    VisualRoot->SetVisibility(false, true);
    bUsingRealMesh = true;

    if (HeldWeaponMesh && SkeletalMesh->DoesSocketExist(WeaponSocketName))
    {
        HeldWeaponMesh->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
    }

    if (WeaponComponent)
    {
        OnWeaponEquipped(WeaponComponent->GetCurrentIndex());
    }
}

FVector ASCPlayerCharacter::GetMuzzleLocation() const
{
    if (bUsingRealMesh && HeldWeaponMesh && HeldWeaponMesh->IsVisible())
    {
        return HeldWeaponMesh->GetComponentLocation() + GetActorForwardVector() * 40.0f;
    }
    return GetActorLocation() + GetActorForwardVector() * 80.0f + FVector(0.0f, 0.0f, 30.0f);
}

void ASCPlayerCharacter::OnWeaponEquipped(int32 WeaponIndex)
{
    if (!WeaponComponent || !WeaponComponent->Weapons.IsValidIndex(WeaponIndex)) return;

    const FSCWeaponData& Data = WeaponComponent->Weapons[WeaponIndex];

    // Real model if one is assigned and loadable...
    USkeletalMesh* RealWeapon = Data.WeaponMesh.IsNull() ? nullptr : Data.WeaponMesh.LoadSynchronous();
    if (bUsingRealMesh && RealWeapon && HeldWeaponMesh)
    {
        HeldWeaponMesh->SetSkeletalMesh(RealWeapon);
        HeldWeaponMesh->SetVisibility(true);
        PlaceholderGun->SetVisibility(false);
        return;
    }

    // ...otherwise a placeholder whose length hints at the weapon class.
    if (HeldWeaponMesh) HeldWeaponMesh->SetVisibility(false);
    PlaceholderGun->SetVisibility(!bUsingRealMesh);
    const float Length = (WeaponIndex == 0) ? 0.3f : (WeaponIndex == 1) ? 0.5f : 0.75f;
    PlaceholderGun->SetRelativeScale3D(FVector(Length, 0.07f, 0.12f));
    PlaceholderGun->SetRelativeLocation(FVector(30.0f + Length * 50.0f, 22.0f, 110.0f));
}

void ASCPlayerCharacter::FirePressed()
{
    if (bIsDead || CurrentVehicle.IsValid()) return;
    bFireHeld = true;
    RefreshCombatStance();
    if (WeaponComponent) WeaponComponent->StartFire();
}

void ASCPlayerCharacter::FireReleased()
{
    bFireHeld = false;
    RefreshCombatStance();
    if (WeaponComponent) WeaponComponent->StopFire();
}

void ASCPlayerCharacter::AimPressed()
{
    if (bIsDead || CurrentVehicle.IsValid()) return;
    bAimHeld = true;
    RefreshCombatStance();
}

void ASCPlayerCharacter::AimReleased()
{
    bAimHeld = false;
    RefreshCombatStance();
}

void ASCPlayerCharacter::ReloadPressed()
{
    if (!bIsDead && WeaponComponent) WeaponComponent->Reload();
}

void ASCPlayerCharacter::NextWeaponPressed()
{
    if (!bIsDead && WeaponComponent) WeaponComponent->NextWeapon();
}

void ASCPlayerCharacter::SelectWeapon(int32 Index)
{
    if (!bIsDead && WeaponComponent) WeaponComponent->EquipWeapon(Index);
}

void ASCPlayerCharacter::RefreshCombatStance()
{
    // While aiming or shooting the character faces the camera direction (strafing) instead of the move direction.
    const bool bCombat = bAimHeld || bFireHeld;
    GetCharacterMovement()->bOrientRotationToMovement = !bCombat;
    bUseControllerRotationYaw = bCombat;
}

void ASCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASCPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASCPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASCPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASCPlayerCharacter::LookUp);
    PlayerInputComponent->BindAxis(TEXT("TurnRate"), this, &ASCPlayerCharacter::TurnRate);
    PlayerInputComponent->BindAxis(TEXT("LookUpRate"), this, &ASCPlayerCharacter::LookUpRate);
    PlayerInputComponent->BindAxis(TEXT("CameraZoom"), this, &ASCPlayerCharacter::CameraZoom);

    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ASCPlayerCharacter::StartJump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ASCPlayerCharacter::StopJump);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ASCPlayerCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ASCPlayerCharacter::StopSprint);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ASCPlayerCharacter::ToggleCrouch);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ASCPlayerCharacter::Interact);

    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ASCPlayerCharacter::FirePressed);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &ASCPlayerCharacter::FireReleased);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &ASCPlayerCharacter::AimPressed);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &ASCPlayerCharacter::AimReleased);
    PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &ASCPlayerCharacter::ReloadPressed);
    PlayerInputComponent->BindAction(TEXT("NextWeapon"), IE_Pressed, this, &ASCPlayerCharacter::NextWeaponPressed);
    PlayerInputComponent->BindAction(TEXT("Weapon1"), IE_Pressed, this, &ASCPlayerCharacter::SelectWeapon1);
    PlayerInputComponent->BindAction(TEXT("Weapon2"), IE_Pressed, this, &ASCPlayerCharacter::SelectWeapon2);
    PlayerInputComponent->BindAction(TEXT("Weapon3"), IE_Pressed, this, &ASCPlayerCharacter::SelectWeapon3);
    PlayerInputComponent->BindAction(TEXT("Weapon4"), IE_Pressed, this, &ASCPlayerCharacter::SelectWeapon4);
}

void ASCPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Over-the-shoulder zoom-in while aiming.
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, bAimHeld ? AimArmLength : DesiredArmLength, DeltaSeconds, 12.0f);
    FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, bAimHeld ? AimFOV : DefaultFOV, DeltaSeconds, 12.0f));

    // While riding in a car the character is hidden and attached; nothing to simulate.
    if (bIsDead || CurrentVehicle.IsValid())
    {
        return;
    }

    UpdateSprint(DeltaSeconds);

    if (GetCharacterMovement()->IsFalling())
    {
        PeakFallSpeed = FMath::Max(PeakFallSpeed, -GetVelocity().Z);
    }

    InteractScanTimer -= DeltaSeconds;
    if (InteractScanTimer <= 0.0f)
    {
        InteractScanTimer = 0.1f;
        UpdateFocusedInteractable();
    }
}

void ASCPlayerCharacter::UpdateSprint(float DeltaSeconds)
{
    const bool bMoving = GetVelocity().SizeSquared2D() > FMath::Square(50.0f);
    bSprinting = bWantsToSprint && !bIsCrouched && !bSprintLocked && bMoving && !bAimHeld && !bFireHeld;

    if (bSprinting)
    {
        Stamina -= StaminaDrainPerSecond * DeltaSeconds;
        if (Stamina <= 0.0f)
        {
            Stamina = 0.0f;
            bSprintLocked = true;
            bSprinting = false;
        }
    }
    else
    {
        Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * DeltaSeconds);
        if (bSprintLocked && Stamina >= SprintResumeStamina)
        {
            bSprintLocked = false;
        }
    }

    GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
}

void ASCPlayerCharacter::UpdateFocusedInteractable()
{
    AActor* Best = nullptr;
    FText BestPrompt;
    float BestDistSq = FMath::Square(InteractRadius);

    const FVector MyLocation = GetActorLocation();
    const FVector Facing = GetActorForwardVector();

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Candidate = *It;
        if (Candidate == this || !IsValid(Candidate) || !Candidate->Implements<USCInteractable>())
        {
            continue;
        }

        if (!ISCInteractable::Execute_CanInteract(Candidate, this))
        {
            continue;
        }

        const FBox Bounds = Candidate->GetComponentsBoundingBox();
        const float DistSq = Bounds.IsValid ? Bounds.ComputeSquaredDistanceToPoint(MyLocation)
                                            : FVector::DistSquared(MyLocation, Candidate->GetActorLocation());
        if (DistSq >= BestDistSq)
        {
            continue;
        }

        // Ignore things clearly behind the character.
        const FVector ToCandidate = (Candidate->GetActorLocation() - MyLocation).GetSafeNormal2D();
        if (DistSq > FMath::Square(120.0f) && FVector::DotProduct(Facing, ToCandidate) < -0.35f)
        {
            continue;
        }

        BestDistSq = DistSq;
        Best = Candidate;
    }

    if (Best)
    {
        BestPrompt = ISCInteractable::Execute_GetInteractionPrompt(Best, this);
    }

    FocusedInteractable = Best;
    FocusedPrompt = BestPrompt;
}

void ASCPlayerCharacter::Interact()
{
    if (bIsDead || CurrentVehicle.IsValid())
    {
        return;
    }

    AActor* Target = FocusedInteractable.Get();
    if (IsValid(Target) && Target->Implements<USCInteractable>() && ISCInteractable::Execute_CanInteract(Target, this))
    {
        ISCInteractable::Execute_Interact(Target, this);
    }
}

void ASCPlayerCharacter::MoveForward(float Value)
{
    if (bIsDead || !Controller || FMath::IsNearlyZero(Value)) return;
    const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void ASCPlayerCharacter::MoveRight(float Value)
{
    if (bIsDead || !Controller || FMath::IsNearlyZero(Value)) return;
    const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void ASCPlayerCharacter::Turn(float Value) { AddControllerYawInput(Value); }
void ASCPlayerCharacter::LookUp(float Value) { AddControllerPitchInput(Value); }

void ASCPlayerCharacter::TurnRate(float Value)
{
    if (!FMath::IsNearlyZero(Value)) AddControllerYawInput(Value * GamepadLookRate * GetWorld()->GetDeltaSeconds());
}

void ASCPlayerCharacter::LookUpRate(float Value)
{
    if (!FMath::IsNearlyZero(Value)) AddControllerPitchInput(Value * GamepadLookRate * GetWorld()->GetDeltaSeconds());
}

void ASCPlayerCharacter::CameraZoom(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;
    DesiredArmLength = FMath::Clamp(DesiredArmLength - Value * 40.0f, MinCameraDistance, MaxCameraDistance);
}

void ASCPlayerCharacter::StartJump()
{
    if (!bIsDead) Jump();
}

void ASCPlayerCharacter::StopJump()
{
    StopJumping();
}

void ASCPlayerCharacter::StartSprint()
{
    bWantsToSprint = true;
}

void ASCPlayerCharacter::StopSprint()
{
    bWantsToSprint = false;
}

void ASCPlayerCharacter::ToggleCrouch()
{
    if (bIsDead) return;
    if (bIsCrouched) UnCrouch();
    else Crouch();
}

void ASCPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
    Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

    // Shrink the placeholder body with the capsule (feet stay on the ground).
    const float DefaultHalfHeight = GetDefaultHalfHeight();
    VisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -DefaultHalfHeight + HalfHeightAdjust));
    VisualRoot->SetRelativeScale3D(FVector(1.0f, 1.0f, (DefaultHalfHeight - HalfHeightAdjust) / DefaultHalfHeight));
}

void ASCPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
    Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

    VisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -GetDefaultHalfHeight()));
    VisualRoot->SetRelativeScale3D(FVector::OneVector);
}

float ASCPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (Applied > 0.0f && !bIsDead)
    {
        Health = FMath::Clamp(Health - Applied, 0.0f, MaxHealth);
        if (Health <= 0.0f)
        {
            Die();
        }
    }
    return Applied;
}

void ASCPlayerCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    const float ImpactSpeed = PeakFallSpeed;
    PeakFallSpeed = 0.0f;

    if (!bIsDead && ImpactSpeed > FallDamageSpeedThreshold)
    {
        TakeDamage((ImpactSpeed - FallDamageSpeedThreshold) * FallDamagePerSpeed, FDamageEvent(), GetController(), nullptr);
    }
}

void ASCPlayerCharacter::Die()
{
    // Shot while driving: get thrown out of the car first so the respawn teleport works.
    if (ASCVehicle* Vehicle = CurrentVehicle.Get())
    {
        Vehicle->ExitVehicle();
    }

    bIsDead = true;
    bWantsToSprint = false;
    bSprinting = false;
    bFireHeld = false;
    bAimHeld = false;
    RefreshCombatStance();
    if (WeaponComponent) WeaponComponent->StopFire();
    StopJumping();
    UnCrouch();

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();

    GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ASCPlayerCharacter::Respawn, RespawnDelay, false);
}

void ASCPlayerCharacter::Respawn()
{
    const int32 Fee = FMath::Min(Money, RespawnFee);
    Money -= Fee;

    Health = MaxHealth;
    Stamina = MaxStamina;
    bSprintLocked = false;
    PeakFallSpeed = 0.0f;

    if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
    {
        if (AActor* Start = GameMode->FindPlayerStart(GetController()))
        {
            SetActorLocationAndRotation(Start->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f), Start->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
        }
    }

    bIsDead = false;
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);

    if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
    {
        GameMode->ClearWanted();
    }

    ShowMessage(FString::Printf(TEXT("Treated at the hospital. Fee: $%d"), Fee), 4.0f);
}

void ASCPlayerCharacter::RestoreFromSave(int32 SavedMoney, float SavedHealth)
{
    GetWorldTimerManager().ClearTimer(RespawnTimerHandle);

    Money = FMath::Max(0, SavedMoney);
    Health = FMath::Clamp(SavedHealth, 1.0f, MaxHealth);
    Stamina = MaxStamina;
    bSprintLocked = false;
    PeakFallSpeed = 0.0f;

    if (bIsDead)
    {
        bIsDead = false;
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    }
}

void ASCPlayerCharacter::SetCurrentVehicle(ASCVehicle* Vehicle)
{
    CurrentVehicle = Vehicle;
    if (Vehicle)
    {
        bWantsToSprint = false;
        bSprinting = false;
        bFireHeld = false;
        bAimHeld = false;
        RefreshCombatStance();
        if (WeaponComponent) WeaponComponent->StopFire();
        FocusedInteractable = nullptr;
        FocusedPrompt = FText::GetEmpty();
    }
}

void ASCPlayerCharacter::ShowMessage(const FString& Message, float Duration) const
{
    if (ASCPlayerController* PlayerController = Cast<ASCPlayerController>(GetController()))
    {
        PlayerController->ShowMessage(Message, Duration);
    }
}
