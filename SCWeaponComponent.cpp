#include "SCWeaponComponent.h"
#include "SCPlayerCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USCWeaponComponent::USCWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Original, generic weapon classes (no licensed names).
    FSCWeaponData Pistol;
    Pistol.Name = TEXT("Pistol");
    Pistol.Damage = 28.0f; Pistol.FireInterval = 0.28f; Pistol.MagazineSize = 12; Pistol.StartingReserve = 96; Pistol.MaxReserve = 240;
    Pistol.ReloadTime = 1.4f; Pistol.Range = 7000.0f; Pistol.SpreadDegrees = 0.8f; Pistol.Pellets = 1; Pistol.bAutomatic = false;

    FSCWeaponData Smg;
    Smg.Name = TEXT("SMG");
    Smg.Damage = 16.0f; Smg.FireInterval = 0.075f; Smg.MagazineSize = 30; Smg.StartingReserve = 150; Smg.MaxReserve = 400;
    Smg.ReloadTime = 1.9f; Smg.Range = 6000.0f; Smg.SpreadDegrees = 2.6f; Smg.Pellets = 1; Smg.bAutomatic = true;

    FSCWeaponData Rifle;
    Rifle.Name = TEXT("Assault Rifle");
    Rifle.Damage = 26.0f; Rifle.FireInterval = 0.1f; Rifle.MagazineSize = 30; Rifle.StartingReserve = 120; Rifle.MaxReserve = 360;
    Rifle.ReloadTime = 2.3f; Rifle.Range = 12000.0f; Rifle.SpreadDegrees = 1.4f; Rifle.Pellets = 1; Rifle.bAutomatic = true;

    FSCWeaponData Shotgun;
    Shotgun.Name = TEXT("Shotgun");
    Shotgun.Damage = 14.0f; Shotgun.FireInterval = 0.9f; Shotgun.MagazineSize = 6; Shotgun.StartingReserve = 30; Shotgun.MaxReserve = 60;
    Shotgun.ReloadTime = 2.6f; Shotgun.Range = 3000.0f; Shotgun.SpreadDegrees = 6.0f; Shotgun.Pellets = 8; Shotgun.bAutomatic = false;

    Weapons = { Pistol, Smg, Rifle, Shotgun };
}

void USCWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    MagAmmo.Reset();
    ReserveAmmo.Reset();
    for (const FSCWeaponData& Data : Weapons)
    {
        MagAmmo.Add(Data.MagazineSize);
        ReserveAmmo.Add(Data.StartingReserve);
    }
    CurrentIndex = 0;

    if (ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(GetOwner()))
    {
        Character->OnWeaponEquipped(CurrentIndex);
    }
}

const FSCWeaponData& USCWeaponComponent::GetCurrentWeapon() const
{
    static const FSCWeaponData Fallback;
    return Weapons.IsValidIndex(CurrentIndex) ? Weapons[CurrentIndex] : Fallback;
}

int32 USCWeaponComponent::GetMagAmmo() const { return IsValidIndex(CurrentIndex) ? MagAmmo[CurrentIndex] : 0; }
int32 USCWeaponComponent::GetReserveAmmo() const { return IsValidIndex(CurrentIndex) ? ReserveAmmo[CurrentIndex] : 0; }

float USCWeaponComponent::GetSecondsSinceLastShot() const
{
    return GetWorld() ? GetWorld()->GetTimeSeconds() - LastShotTime : 1000.0f;
}

void USCWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bTriggerHeld && Weapons.IsValidIndex(CurrentIndex) && Weapons[CurrentIndex].bAutomatic)
    {
        TryFire();
    }
}

void USCWeaponComponent::StartFire()
{
    bTriggerHeld = true;
    TryFire();
}

void USCWeaponComponent::StopFire()
{
    bTriggerHeld = false;
}

void USCWeaponComponent::TryFire()
{
    if (!IsValidIndex(CurrentIndex) || bReloading || !GetWorld()) return;

    if (GetWorld()->GetTimeSeconds() < NextFireTime) return;

    if (MagAmmo[CurrentIndex] <= 0)
    {
        Reload();
        return;
    }

    FireOnce();
}

void USCWeaponComponent::FireOnce()
{
    ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(GetOwner());
    UWorld* World = GetWorld();
    if (!Character || !World || Character->IsDead()) return;

    const FSCWeaponData& Data = Weapons[CurrentIndex];
    --MagAmmo[CurrentIndex];
    NextFireTime = World->GetTimeSeconds() + Data.FireInterval;
    LastShotTime = World->GetTimeSeconds();

    // Aim from the camera through the crosshair, but start the trace in front of the character
    // so shots never hit things behind the player.
    FVector ViewLocation = Character->GetActorLocation();
    FRotator ViewRotation = Character->GetActorRotation();
    if (AController* Controller = Character->GetController())
    {
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }
    const FVector ViewDirection = ViewRotation.Vector();
    const float AlongView = FVector::DotProduct(Character->GetActorLocation() - ViewLocation, ViewDirection);
    const FVector TraceStart = ViewLocation + ViewDirection * FMath::Max(0.0f, AlongView);
    const FVector Muzzle = Character->GetMuzzleLocation();

    FCollisionQueryParams Params(SCENE_QUERY_STAT(SCWeaponTrace), true, Character);
    Params.bReturnPhysicalMaterial = false;

    const float SpreadRadians = FMath::DegreesToRadians(Data.SpreadDegrees);
    for (int32 Pellet = 0; Pellet < FMath::Max(1, Data.Pellets); ++Pellet)
    {
        const FVector Direction = FMath::VRandCone(ViewDirection, SpreadRadians);
        const FVector TraceEnd = TraceStart + Direction * Data.Range;

        FHitResult Hit;
        const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Pawn, Params);
        const FVector ImpactPoint = bHit ? Hit.ImpactPoint : TraceEnd;

        if (bHit && Hit.GetActor())
        {
            UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Data.Damage, Direction, Hit, Character->GetController(), Character, UDamageType::StaticClass());
        }

#if ENABLE_DRAW_DEBUG
        // Placeholder tracer / impact marker until real VFX (Niagara) are authored.
        DrawDebugLine(World, Muzzle, ImpactPoint, FColor(255, 220, 90), false, 0.06f, 0, 2.0f);
        if (bHit)
        {
            DrawDebugPoint(World, ImpactPoint, 10.0f, FColor::Orange, false, 0.4f);
        }
#endif
    }

    if (MagAmmo[CurrentIndex] <= 0 && ReserveAmmo[CurrentIndex] > 0)
    {
        Reload();
    }
}

void USCWeaponComponent::Reload()
{
    if (!IsValidIndex(CurrentIndex) || bReloading || !GetWorld()) return;

    const FSCWeaponData& Data = Weapons[CurrentIndex];
    if (MagAmmo[CurrentIndex] >= Data.MagazineSize || ReserveAmmo[CurrentIndex] <= 0) return;

    bReloading = true;
    GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &USCWeaponComponent::FinishReload, Data.ReloadTime, false);
}

void USCWeaponComponent::FinishReload()
{
    bReloading = false;
    if (!IsValidIndex(CurrentIndex)) return;

    const int32 Needed = Weapons[CurrentIndex].MagazineSize - MagAmmo[CurrentIndex];
    const int32 Taken = FMath::Min(Needed, ReserveAmmo[CurrentIndex]);
    MagAmmo[CurrentIndex] += Taken;
    ReserveAmmo[CurrentIndex] -= Taken;
}

void USCWeaponComponent::EquipWeapon(int32 Index)
{
    if (!IsValidIndex(Index) || Index == CurrentIndex) return;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
    }
    bReloading = false;
    CurrentIndex = Index;
    NextFireTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.25f : 0.0f;

    if (ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(GetOwner()))
    {
        Character->OnWeaponEquipped(CurrentIndex);
    }
}

void USCWeaponComponent::NextWeapon()
{
    if (Weapons.Num() > 0)
    {
        EquipWeapon((CurrentIndex + 1) % Weapons.Num());
    }
}

void USCWeaponComponent::AddAmmo(int32 WeaponIndex, int32 Amount)
{
    if (IsValidIndex(WeaponIndex))
    {
        ReserveAmmo[WeaponIndex] = FMath::Clamp(ReserveAmmo[WeaponIndex] + Amount, 0, Weapons[WeaponIndex].MaxReserve);
    }
}
