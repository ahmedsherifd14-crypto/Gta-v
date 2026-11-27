#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "SCWeaponComponent.generated.h"

class USkeletalMesh;

USTRUCT(BlueprintType)
struct FSCWeaponData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name = TEXT("Pistol");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 28.0f;          // per pellet
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FireInterval = 0.28f;    // seconds between shots
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MagazineSize = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StartingReserve = 96;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxReserve = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReloadTime = 1.4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Range = 7000.0f;         // cm
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpreadDegrees = 1.0f;    // half-angle cone
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Pellets = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAutomatic = false;

    /** Optional real weapon model. Leave empty to use the placeholder. Attached to the hand socket. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<USkeletalMesh> WeaponMesh;
};

/** Hitscan weapon inventory: 4 weapons, ammo, reload, spread, automatic/semi fire. */
UCLASS(ClassGroup=(SanClemente), meta=(BlueprintSpawnableComponent))
class SANCLEMENTE_API USCWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USCWeaponComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Weapon") void StartFire();
    UFUNCTION(BlueprintCallable, Category="Weapon") void StopFire();
    UFUNCTION(BlueprintCallable, Category="Weapon") void Reload();
    UFUNCTION(BlueprintCallable, Category="Weapon") void EquipWeapon(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Weapon") void NextWeapon();
    UFUNCTION(BlueprintCallable, Category="Weapon") void AddAmmo(int32 WeaponIndex, int32 Amount);

    const FSCWeaponData& GetCurrentWeapon() const;
    UFUNCTION(BlueprintPure, Category="Weapon") int32 GetCurrentIndex() const { return CurrentIndex; }
    UFUNCTION(BlueprintPure, Category="Weapon") int32 GetMagAmmo() const;
    UFUNCTION(BlueprintPure, Category="Weapon") int32 GetReserveAmmo() const;
    UFUNCTION(BlueprintPure, Category="Weapon") bool IsReloading() const { return bReloading; }
    /** Seconds since the last shot was fired (large if never). Used by NPCs to react to gunfire. */
    UFUNCTION(BlueprintPure, Category="Weapon") float GetSecondsSinceLastShot() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    TArray<FSCWeaponData> Weapons;

protected:
    virtual void BeginPlay() override;

private:
    void TryFire();
    void FireOnce();
    void FinishReload();
    bool IsValidIndex(int32 Index) const { return Weapons.IsValidIndex(Index) && MagAmmo.IsValidIndex(Index) && ReserveAmmo.IsValidIndex(Index); }

    UPROPERTY(Transient) TArray<int32> MagAmmo;
    UPROPERTY(Transient) TArray<int32> ReserveAmmo;

    int32 CurrentIndex = 0;
    bool bTriggerHeld = false;
    bool bReloading = false;
    float NextFireTime = 0.0f;
    float LastShotTime = -1000.0f;
    FTimerHandle ReloadTimer;
};
