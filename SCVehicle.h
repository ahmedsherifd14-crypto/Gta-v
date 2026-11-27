#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SCInteractable.h"
#include "SCVehicle.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class ASCPlayerCharacter;

/**
 * Drivable placeholder car with arcade (non-Chaos) physics: no vehicle assets required.
 * Walk up and press Interact to get in; press Interact again to get out.
 */
UCLASS()
class SANCLEMENTE_API ASCVehicle : public APawn, public ISCInteractable
{
    GENERATED_BODY()

public:
    ASCVehicle();

    virtual void Tick(float DeltaSeconds) override;
    virtual void UnPossessed() override;

    // ISCInteractable
    virtual bool CanInteract_Implementation(APawn* InteractingPawn) const override;
    virtual FText GetInteractionPrompt_Implementation(APawn* InteractingPawn) const override;
    virtual void Interact_Implementation(APawn* InteractingPawn) override;

    void EnterVehicle(ASCPlayerCharacter* Character);
    void ExitVehicle();

    UFUNCTION(BlueprintPure, Category="Vehicle") ASCPlayerCharacter* GetDriver() const { return Driver; }
    UFUNCTION(BlueprintPure, Category="Vehicle") bool HasDriver() const { return Driver != nullptr; }
    /** Signed speed along the car's forward axis in cm/s. */
    UFUNCTION(BlueprintPure, Category="Vehicle") float GetForwardSpeed() const { return ForwardSpeed; }

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void SetBodyColor(const FLinearColor& Color);

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void ThrottleInputAxis(float Value) { ThrottleInput = FMath::Clamp(Value, -1.0f, 1.0f); }
    void SteerInputAxis(float Value) { SteerInput = FMath::Clamp(Value, -1.0f, 1.0f); }
    void HandbrakePressed() { bHandbrake = true; }
    void HandbrakeReleased() { bHandbrake = false; }
    void LookTurn(float Value);
    void LookUp(float Value);
    void LookTurnRate(float Value);
    void LookUpRate(float Value);
    void CameraZoom(float Value);
    void ExitPressed() { ExitVehicle(); }

    bool FindExitLocation(const ASCPlayerCharacter* Character, FVector& OutLocation) const;

    // ---- Components ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<UBoxComponent> CollisionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> CabinMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TArray<TObjectPtr<UStaticMeshComponent>> WheelMeshes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    // ---- Tuning (cm, cm/s, cm/s^2) ----
    UPROPERTY(EditAnywhere, Category="Driving") float MaxForwardSpeed = 4500.0f;   // ~162 km/h
    UPROPERTY(EditAnywhere, Category="Driving") float MaxReverseSpeed = 1200.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float Acceleration = 2400.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float ReverseAcceleration = 1200.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float BrakeDeceleration = 4200.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float HandbrakeDeceleration = 3000.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float CoastDeceleration = 450.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float ParkedBrakeDeceleration = 1800.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float MaxSteerRateDegrees = 85.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float GravityZ = 1960.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float RideHeight = 2.0f;
    UPROPERTY(EditAnywhere, Category="Driving") float GroundSnapDistance = 25.0f;

    UPROPERTY(EditAnywhere, Category="Camera") float AutoCenterDelay = 1.2f;
    UPROPERTY(EditAnywhere, Category="Camera") float AutoCenterSpeed = 2.5f;

private:
    UPROPERTY(Transient)
    TObjectPtr<ASCPlayerCharacter> Driver;

    float ThrottleInput = 0.0f;
    float SteerInput = 0.0f;
    bool bHandbrake = false;
    float ForwardSpeed = 0.0f;
    float VerticalVelocity = 0.0f;
    float LastLookInputTime = -100.0f;
    FVector InitialLocation = FVector::ZeroVector;
};
