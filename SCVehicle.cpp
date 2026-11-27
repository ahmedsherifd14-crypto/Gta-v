#include "SCVehicle.h"
#include "SCBasicShapes.h"
#include "SCPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    // Collision box half extents (cm). Car is ~4.5 m long, 2 m wide.
    const FVector BoxHalfExtent(225.0f, 100.0f, 60.0f);

    void SetupCarPart(UStaticMeshComponent* Part, UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
    {
        Part->SetStaticMesh(Mesh);
        Part->SetMaterial(0, SCShapes::BasicMaterial());
        Part->SetRelativeLocationAndRotation(Location, Rotation);
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

ASCVehicle::ASCVehicle()
{
    PrimaryActorTick.bCanEverTick = true;

    // A pawn defaults to following controller yaw; the car steers itself instead.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetBoxExtent(BoxHalfExtent);
    CollisionBox->SetCollisionProfileName(TEXT("Vehicle"));
    CollisionBox->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    CollisionBox->SetSimulatePhysics(false);
    CollisionBox->SetGenerateOverlapEvents(true);
    RootComponent = CollisionBox;

    UStaticMesh* Cube = SCShapes::Cube();
    UStaticMesh* Cylinder = SCShapes::Cylinder();

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(CollisionBox);
    SetupCarPart(BodyMesh, Cube, FVector(0.0f, 0.0f, -25.0f), FRotator::ZeroRotator, FVector(4.5f, 2.0f, 0.6f));

    CabinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinMesh"));
    CabinMesh->SetupAttachment(CollisionBox);
    SetupCarPart(CabinMesh, Cube, FVector(-30.0f, 0.0f, 30.0f), FRotator::ZeroRotator, FVector(2.4f, 1.8f, 0.5f));

    // Wheels: cylinders lying on their side (Roll 90 turns the cylinder axis onto the car's Y axis).
    const FVector WheelOffsets[4] =
    {
        FVector(140.0f, 100.0f, -25.0f), FVector(140.0f, -100.0f, -25.0f),
        FVector(-140.0f, 100.0f, -25.0f), FVector(-140.0f, -100.0f, -25.0f)
    };
    for (int32 i = 0; i < 4; ++i)
    {
        UStaticMeshComponent* Wheel = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Wheel%d"), i));
        Wheel->SetupAttachment(CollisionBox);
        SetupCarPart(Wheel, Cylinder, WheelOffsets[i], FRotator(0.0f, 0.0f, 90.0f), FVector(0.7f, 0.7f, 0.25f));
        WheelMeshes.Add(Wheel);
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(CollisionBox);
    CameraBoom->TargetArmLength = 700.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 110.0f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 10.0f;
    CameraBoom->ProbeSize = 12.0f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ASCVehicle::BeginPlay()
{
    Super::BeginPlay();

    InitialLocation = GetActorLocation();

    TintPart(BodyMesh, FLinearColor(0.6f, 0.05f, 0.05f));
    TintPart(CabinMesh, FLinearColor(0.02f, 0.03f, 0.05f));
    for (UStaticMeshComponent* Wheel : WheelMeshes)
    {
        TintPart(Wheel, FLinearColor(0.01f, 0.01f, 0.01f));
    }
}

void ASCVehicle::SetBodyColor(const FLinearColor& Color)
{
    TintPart(BodyMesh, Color);
}

void ASCVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("Throttle"), this, &ASCVehicle::ThrottleInputAxis);
    PlayerInputComponent->BindAxis(TEXT("Steer"), this, &ASCVehicle::SteerInputAxis);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASCVehicle::LookTurn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASCVehicle::LookUp);
    PlayerInputComponent->BindAxis(TEXT("TurnRate"), this, &ASCVehicle::LookTurnRate);
    PlayerInputComponent->BindAxis(TEXT("LookUpRate"), this, &ASCVehicle::LookUpRate);
    PlayerInputComponent->BindAxis(TEXT("CameraZoom"), this, &ASCVehicle::CameraZoom);

    PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Pressed, this, &ASCVehicle::HandbrakePressed);
    PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Released, this, &ASCVehicle::HandbrakeReleased);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ASCVehicle::ExitPressed);
}

void ASCVehicle::LookTurn(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;
    AddControllerYawInput(Value);
    LastLookInputTime = GetWorld()->GetTimeSeconds();
}

void ASCVehicle::LookUp(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;
    AddControllerPitchInput(Value);
    LastLookInputTime = GetWorld()->GetTimeSeconds();
}

void ASCVehicle::LookTurnRate(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;
    AddControllerYawInput(Value * 120.0f * GetWorld()->GetDeltaSeconds());
    LastLookInputTime = GetWorld()->GetTimeSeconds();
}

void ASCVehicle::LookUpRate(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;
    AddControllerPitchInput(Value * 120.0f * GetWorld()->GetDeltaSeconds());
    LastLookInputTime = GetWorld()->GetTimeSeconds();
}

void ASCVehicle::CameraZoom(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;
    CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - Value * 60.0f, 400.0f, 1200.0f);
}

void ASCVehicle::UnPossessed()
{
    Super::UnPossessed();
    ThrottleInput = 0.0f;
    SteerInput = 0.0f;
    bHandbrake = false;
}

bool ASCVehicle::CanInteract_Implementation(APawn* InteractingPawn) const
{
    return Driver == nullptr && Cast<ASCPlayerCharacter>(InteractingPawn) != nullptr;
}

FText ASCVehicle::GetInteractionPrompt_Implementation(APawn* InteractingPawn) const
{
    return FText::FromString(TEXT("Enter Vehicle"));
}

void ASCVehicle::Interact_Implementation(APawn* InteractingPawn)
{
    EnterVehicle(Cast<ASCPlayerCharacter>(InteractingPawn));
}

void ASCVehicle::EnterVehicle(ASCPlayerCharacter* Character)
{
    if (!IsValid(Character) || Driver != nullptr) return;

    AController* PlayerController = Character->GetController();
    if (!PlayerController) return;

    Character->UnCrouch();
    Character->SetCurrentVehicle(this);
    Driver = Character;

    // Hide the driver and carry them inside the car.
    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }
    Character->SetActorEnableCollision(false);
    Character->SetActorHiddenInGame(true);
    Character->AttachToComponent(CollisionBox, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    ThrottleInput = 0.0f;
    SteerInput = 0.0f;
    bHandbrake = false;

    PlayerController->Possess(this);
    PlayerController->SetControlRotation(FRotator(-12.0f, GetActorRotation().Yaw, 0.0f));
    LastLookInputTime = GetWorld()->GetTimeSeconds();
}

bool ASCVehicle::FindExitLocation(const ASCPlayerCharacter* Character, FVector& OutLocation) const
{
    const float Radius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
    const float HalfHeight = Character->GetDefaultHalfHeight();
    const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(SCVehicleExit), false, this);
    Params.AddIgnoredActor(Character);

    // Standing height: car floor is the bottom of the collision box.
    const float FloorZ = GetActorLocation().Z - BoxHalfExtent.Z;
    const float StandZ = FloorZ + HalfHeight + 6.0f;

    const float SideDistance = BoxHalfExtent.Y + Radius + 40.0f;
    const float EndDistance = BoxHalfExtent.X + Radius + 40.0f;
    const FVector LocalCandidates[4] =
    {
        FVector(0.0f, -SideDistance, 0.0f),
        FVector(0.0f, SideDistance, 0.0f),
        FVector(-EndDistance, 0.0f, 0.0f),
        FVector(EndDistance, 0.0f, 0.0f)
    };

    for (const FVector& Local : LocalCandidates)
    {
        FVector Candidate = GetActorLocation() + GetActorTransform().TransformVectorNoScale(Local);
        Candidate.Z = StandZ;
        if (!GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, Shape, Params))
        {
            OutLocation = Candidate;
            return true;
        }
    }

    // Everything around is blocked: pop out above the roof.
    OutLocation = GetActorLocation() + FVector(0.0f, 0.0f, BoxHalfExtent.Z + HalfHeight + 40.0f);
    return false;
}

void ASCVehicle::ExitVehicle()
{
    if (!Driver) return;

    ASCPlayerCharacter* Character = Driver;
    AController* PlayerController = GetController();

    FVector ExitLocation;
    FindExitLocation(Character, ExitLocation);

    Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Character->SetActorLocationAndRotation(ExitLocation, FRotator(0.0f, GetActorRotation().Yaw, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
    Character->SetActorEnableCollision(true);
    Character->SetActorHiddenInGame(false);
    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->SetMovementMode(MOVE_Falling);
    }
    Character->SetCurrentVehicle(nullptr);
    Driver = nullptr;

    ThrottleInput = 0.0f;
    SteerInput = 0.0f;
    bHandbrake = false;

    if (PlayerController)
    {
        PlayerController->Possess(Character);
        PlayerController->SetControlRotation(FRotator(-10.0f, GetActorRotation().Yaw, 0.0f));
    }
}

void ASCVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const float Dt = FMath::Min(DeltaSeconds, 0.05f);
    const bool bDriven = Driver != nullptr && GetController() != nullptr;
    const float Throttle = bDriven ? ThrottleInput : 0.0f;
    const float Steer = bDriven ? SteerInput : 0.0f;

    // ---- Longitudinal speed ----
    if (!bDriven)
    {
        ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.0f, Dt, ParkedBrakeDeceleration);
    }
    else if (bHandbrake)
    {
        ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.0f, Dt, HandbrakeDeceleration);
    }
    else if (Throttle > KINDA_SMALL_NUMBER)
    {
        if (ForwardSpeed < -10.0f)
        {
            ForwardSpeed = FMath::Min(0.0f, ForwardSpeed + BrakeDeceleration * Dt);
        }
        else
        {
            const float Taper = 1.0f - FMath::Clamp(ForwardSpeed / MaxForwardSpeed, 0.0f, 1.0f);
            ForwardSpeed += Throttle * Acceleration * Taper * Dt;
        }
    }
    else if (Throttle < -KINDA_SMALL_NUMBER)
    {
        if (ForwardSpeed > 10.0f)
        {
            ForwardSpeed = FMath::Max(0.0f, ForwardSpeed - BrakeDeceleration * Dt);
        }
        else
        {
            ForwardSpeed += Throttle * ReverseAcceleration * Dt;
        }
    }
    else
    {
        ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.0f, Dt, CoastDeceleration);
    }
    ForwardSpeed = FMath::Clamp(ForwardSpeed, -MaxReverseSpeed, MaxForwardSpeed);

    // ---- Steering (needs forward motion; eases off at top speed) ----
    const float AbsSpeed = FMath::Abs(ForwardSpeed);
    if (AbsSpeed > 5.0f && !FMath::IsNearlyZero(Steer))
    {
        float SpeedFactor = FMath::Clamp(AbsSpeed / 600.0f, 0.0f, 1.0f) * (1.0f - 0.55f * FMath::Clamp(AbsSpeed / MaxForwardSpeed, 0.0f, 1.0f));
        if (bHandbrake && AbsSpeed > 800.0f)
        {
            SpeedFactor *= 1.6f; // handbrake turn
        }
        const float Direction = ForwardSpeed >= 0.0f ? 1.0f : -1.0f;
        AddActorWorldRotation(FRotator(0.0f, Steer * MaxSteerRateDegrees * SpeedFactor * Direction * Dt, 0.0f));
    }

    // ---- Ground following / gravity ----
    const FVector Location = GetActorLocation();
    FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(SCVehicleGround), false, this);
    if (Driver) GroundParams.AddIgnoredActor(Driver);

    FHitResult GroundHit;
    const bool bGrounded = GetWorld()->LineTraceSingleByChannel(
        GroundHit, Location, Location - FVector(0.0f, 0.0f, BoxHalfExtent.Z + GroundSnapDistance), ECC_Visibility, GroundParams);

    if (bGrounded)
    {
        VerticalVelocity = 0.0f;
        const float TargetZ = GroundHit.ImpactPoint.Z + BoxHalfExtent.Z + RideHeight;
        if (!FMath::IsNearlyEqual(Location.Z, TargetZ, 0.1f))
        {
            AddActorWorldOffset(FVector(0.0f, 0.0f, TargetZ - Location.Z), false);
        }
    }
    else
    {
        VerticalVelocity -= GravityZ * Dt;
        FHitResult FallHit;
        AddActorWorldOffset(FVector(0.0f, 0.0f, VerticalVelocity * Dt), true, &FallHit);
        if (FallHit.bBlockingHit)
        {
            VerticalVelocity = 0.0f;
        }
    }

    // ---- Horizontal movement with slide / bounce ----
    const FVector Forward = GetActorForwardVector();
    const FVector Delta = Forward * ForwardSpeed * Dt;
    if (!Delta.IsNearlyZero())
    {
        FHitResult Hit;
        AddActorWorldOffset(Delta, true, &Hit);
        if (Hit.bBlockingHit)
        {
            const FVector Slide = FVector::VectorPlaneProject(Delta, Hit.Normal) * (1.0f - Hit.Time);
            if (!Slide.IsNearlyZero())
            {
                AddActorWorldOffset(Slide, true);
            }

            const float TravelSign = ForwardSpeed >= 0.0f ? 1.0f : -1.0f;
            const float HeadOn = FMath::Clamp(FVector::DotProduct(Forward * TravelSign, -Hit.ImpactNormal), 0.0f, 1.0f);
            ForwardSpeed *= FMath::Lerp(0.85f, -0.25f, HeadOn);
        }
    }

    // ---- Chase camera slowly swings back behind the car when the player stops looking around ----
    if (bDriven && GetWorld()->GetTimeSeconds() - LastLookInputTime > AutoCenterDelay)
    {
        if (AController* DriverController = GetController())
        {
            const FRotator Current = DriverController->GetControlRotation();
            const FRotator Target(-12.0f, GetActorRotation().Yaw, 0.0f);
            DriverController->SetControlRotation(FMath::RInterpTo(Current, Target, Dt, AutoCenterSpeed));
        }
    }

    // Safety net: never let a car fall out of the world forever.
    if (GetActorLocation().Z < -20000.0f)
    {
        SetActorLocation(InitialLocation + FVector(0.0f, 0.0f, 200.0f), false, nullptr, ETeleportType::TeleportPhysics);
        ForwardSpeed = 0.0f;
        VerticalVelocity = 0.0f;
    }
}
