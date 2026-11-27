#include "SCNPC.h"
#include "SCBasicShapes.h"
#include "SCGameMode.h"
#include "SCMoneyPickup.h"
#include "SCPlayerCharacter.h"
#include "SCPlayerController.h"
#include "SCVehicle.h"
#include "SCWeaponComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    constexpr float NPCHalfHeight = 96.0f;

    void SetupPart(UStaticMeshComponent* Part, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
    {
        Part->SetStaticMesh(Mesh);
        Part->SetMaterial(0, SCShapes::BasicMaterial());
        Part->SetRelativeLocation(Location);
        Part->SetRelativeScale3D(Scale);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
    }

    void Tint(UStaticMeshComponent* Part, const FLinearColor& Color)
    {
        if (UMaterialInstanceDynamic* Material = Part->CreateDynamicMaterialInstance(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
        }
    }
}

ASCNPC::ASCNPC()
{
    PrimaryActorTick.bCanEverTick = true;

    // Without this, spawned characters get no controller and cannot be driven by AddMovementInput.
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    GetCapsuleComponent()->InitCapsuleSize(42.0f, NPCHalfHeight);
    bUseControllerRotationYaw = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
    Movement->MaxWalkSpeed = WalkSpeed;

    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
    VisualRoot->SetupAttachment(GetCapsuleComponent());
    VisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -NPCHalfHeight));

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(VisualRoot);
    SetupPart(BodyMesh, SCShapes::Cylinder(), FVector(0.0f, 0.0f, 72.0f), FVector(0.7f, 0.7f, 1.44f));

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(VisualRoot);
    SetupPart(HeadMesh, SCShapes::Sphere(), FVector(0.0f, 0.0f, 162.0f), FVector(0.36f, 0.36f, 0.36f));

    FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
    FacingMarker->SetupAttachment(VisualRoot);
    SetupPart(FacingMarker, SCShapes::Cube(), FVector(30.0f, 0.0f, 120.0f), FVector(0.3f, 0.5f, 0.12f));
}

void ASCNPC::BeginPlay()
{
    Super::BeginPlay();

    Health = (Role == ESCNPCRole::Police) ? PoliceHealth : CivilianHealth;
    NextRepickTime = 0.0f;
    ApplyLook();

    // Real character mesh, if the assets exist.
    if (!CharacterSkeletalMesh.IsNull())
    {
        if (USkeletalMesh* LoadedMesh = CharacterSkeletalMesh.LoadSynchronous())
        {
            USkeletalMeshComponent* SkeletalMesh = GetMesh();
            SkeletalMesh->SetSkeletalMesh(LoadedMesh);
            SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -NPCHalfHeight), FRotator(0.0f, CharacterMeshYawOffset, 0.0f));
            SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            if (UClass* AnimClass = CharacterAnimClass.LoadSynchronous())
            {
                SkeletalMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
                SkeletalMesh->SetAnimInstanceClass(AnimClass);
            }
            VisualRoot->SetVisibility(false, true);
            bUsingRealMesh = true;
        }
    }
}

void ASCNPC::SetRole(ESCNPCRole NewRole)
{
    Role = NewRole;
    Health = (Role == ESCNPCRole::Police) ? PoliceHealth : CivilianHealth;
    ApplyLook();
}

void ASCNPC::ApplyLook()
{
    if (Role == ESCNPCRole::Police)
    {
        Tint(BodyMesh, FLinearColor(0.02f, 0.05f, 0.25f));
        Tint(HeadMesh, FLinearColor(0.55f, 0.38f, 0.28f));
        Tint(FacingMarker, FLinearColor(0.9f, 0.8f, 0.1f));
    }
    else
    {
        const float Hue = FMath::FRand() * 360.0f;
        Tint(BodyMesh, FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue / 360.0f * 255.0f), 150, 150));
        Tint(HeadMesh, FLinearColor(0.5f, 0.34f, 0.24f));
        Tint(FacingMarker, FLinearColor(0.15f, 0.15f, 0.15f));
    }
}

ASCPlayerCharacter* ASCNPC::FindPlayer() const
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(PlayerPawn))
    {
        return Character;
    }
    if (const ASCVehicle* Vehicle = Cast<ASCVehicle>(PlayerPawn))
    {
        return Vehicle->GetDriver();
    }
    return nullptr;
}

void ASCNPC::MoveAlong(const FVector& Direction, float Speed)
{
    GetCharacterMovement()->MaxWalkSpeed = Speed;
    AddMovementInput(Direction.GetSafeNormal2D(), 1.0f);
}

void ASCNPC::WanderStep(float DeltaSeconds, float Speed)
{
    const float Now = GetWorld()->GetTimeSeconds();

    // Detect "walking into a wall" and pick a new direction.
    const bool bShouldBeMoving = Speed > 0.0f;
    if (bShouldBeMoving && GetVelocity().SizeSquared2D() < FMath::Square(15.0f))
    {
        StuckTime += DeltaSeconds;
    }
    else
    {
        StuckTime = 0.0f;
    }

    if (Now >= NextRepickTime || StuckTime > 0.8f)
    {
        const float Yaw = FMath::FRandRange(0.0f, 360.0f);
        WanderDirection = FRotator(0.0f, Yaw, 0.0f).Vector();
        NextRepickTime = Now + FMath::FRandRange(3.0f, 8.0f);
        StuckTime = 0.0f;
    }

    MoveAlong(WanderDirection, Speed);
}

void ASCNPC::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bDead || !GetWorld()) return;

    ASCPlayerCharacter* Player = FindPlayer();
    if (Role == ESCNPCRole::Police)
    {
        const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>();
        TickPolice(DeltaSeconds, Player, GameMode ? GameMode->GetWantedLevel() : 0);
    }
    else
    {
        TickCivilian(DeltaSeconds, Player);
    }
}

void ASCNPC::TickCivilian(float DeltaSeconds, ASCPlayerCharacter* Player)
{
    const float Now = GetWorld()->GetTimeSeconds();

    // Panic when the player fires nearby.
    if (Player && Player->GetWeaponComponent() && Player->GetWeaponComponent()->GetSecondsSinceLastShot() < 3.0f
        && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(3500.0f))
    {
        FleeUntilTime = Now + 6.0f;
    }

    if (Now < FleeUntilTime && Player)
    {
        FVector Away = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
        if (GetVelocity().SizeSquared2D() < FMath::Square(40.0f))
        {
            // Blocked: slide sideways around the obstacle.
            Away = FRotator(0.0f, FMath::RandBool() ? 70.0f : -70.0f, 0.0f).RotateVector(Away);
        }
        MoveAlong(Away, RunSpeed);
        return;
    }

    WanderStep(DeltaSeconds, WalkSpeed);
}

void ASCNPC::TickPolice(float DeltaSeconds, ASCPlayerCharacter* Player, int32 WantedLevel)
{
    const float Now = GetWorld()->GetTimeSeconds();

    if (WantedLevel <= 0 || !Player || Player->IsDead())
    {
        // Calm down and leave.
        if (NoWantedSince < 0.0f) NoWantedSince = Now;
        WanderStep(DeltaSeconds, WalkSpeed);
        if (Now - NoWantedSince > 12.0f)
        {
            Destroy();
        }
        return;
    }
    NoWantedSince = -1.0f;

    const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
    const float Distance = ToPlayer.Size();

    if (Distance > PoliceDespawnDistance)
    {
        Destroy(); // fell too far behind; the game mode will spawn a fresh unit closer
        return;
    }

    // Movement: close in, hold at a distance, back off if the player is on top of us.
    if (Distance > 1300.0f)
    {
        MoveAlong(ToPlayer, PoliceChaseSpeed);
    }
    else if (Distance < 600.0f)
    {
        MoveAlong(-ToPlayer, WalkSpeed * 2.0f);
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        // Strafe a little while shooting.
        MoveAlong(FVector::CrossProduct(ToPlayer.GetSafeNormal2D(), FVector::UpVector), WalkSpeed * 0.8f);
    }

    // Face the player when engaging.
    if (Distance < PoliceFireRange)
    {
        const FRotator Face = FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f);
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 8.0f));
    }

    // Shooting with line of sight.
    if (Distance < PoliceFireRange && Now >= NextShotTime)
    {
        const FVector Eye = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
        const FVector Target = Player->GetActorLocation();

        FCollisionQueryParams Params(SCENE_QUERY_STAT(SCPoliceLOS), true, this);
        FHitResult Hit;
        // The player counts as visible when the first thing hit is them or the car they are driving.
        const AActor* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
        const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Eye, Target, ECC_Pawn, Params)
            && Hit.GetActor() != Player && Hit.GetActor() != PlayerPawn;

        if (!bBlocked)
        {
            NextShotTime = Now + FMath::FRandRange(0.55f, 0.9f);
            const bool bHit = FMath::FRand() < PoliceHitChance;
            if (bHit)
            {
                Player->TakeDamage(PoliceDamage, FDamageEvent(), GetController(), this);
            }
#if ENABLE_DRAW_DEBUG
            const FVector End = bHit ? Target : Target + FMath::VRand() * 120.0f;
            DrawDebugLine(GetWorld(), Eye, End, FColor(255, 90, 90), false, 0.06f, 0, 2.0f);
#endif
        }
    }
}

float ASCNPC::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (Applied <= 0.0f || bDead) return Applied;

    Health -= Applied;

    const bool bByPlayer = EventInstigator && EventInstigator->IsA<ASCPlayerController>();
    ASCGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASCGameMode>() : nullptr;

    if (Health <= 0.0f)
    {
        Die(bByPlayer);
    }
    else if (bByPlayer)
    {
        if (GameMode) GameMode->ReportCrime(Role == ESCNPCRole::Police ? 3 : 1);
        if (Role == ESCNPCRole::Civilian)
        {
            FleeUntilTime = GetWorld()->GetTimeSeconds() + 8.0f;
        }
    }
    return Applied;
}

void ASCNPC::Die(bool bKilledByPlayer)
{
    bDead = true;

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Lie down (placeholder body pivots at the feet).
    VisualRoot->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
    if (bUsingRealMesh)
    {
        GetMesh()->SetRelativeRotation(FRotator(0.0f, CharacterMeshYawOffset, 90.0f));
    }

    if (bKilledByPlayer)
    {
        if (ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
        {
            GameMode->ReportCrime(Role == ESCNPCRole::Police ? 6 : 3);
        }
    }

    // Drop some cash.
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FVector DropLocation = GetActorLocation() - FVector(0.0f, 0.0f, NPCHalfHeight - 60.0f);
    if (ASCMoneyPickup* Pickup = GetWorld()->SpawnActor<ASCMoneyPickup>(DropLocation, FRotator::ZeroRotator, Params))
    {
        Pickup->Value = (Role == ESCNPCRole::Police) ? FMath::RandRange(100, 300) : FMath::RandRange(20, 150);
    }

    SetLifeSpan(10.0f);
}
