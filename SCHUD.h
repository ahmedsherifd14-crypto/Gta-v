#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SCHUD.generated.h"

/** Canvas-drawn HUD: money, clock, health/stamina, radar, speedometer, prompts and toasts. */
UCLASS()
class SANCLEMENTE_API ASCHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    void PushMessage(const FString& Message, float Duration = 3.0f);

protected:
    UPROPERTY(EditDefaultsOnly, Category="Radar")
    float RadarSize = 220.0f;

    /** World radius (cm) shown on the radar. */
    UPROPERTY(EditDefaultsOnly, Category="Radar")
    float RadarRange = 7000.0f;

private:
    struct FToast
    {
        FString Text;
        float ExpireTime = 0.0f;
    };

    void DrawShadowedText(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale);
    void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill);
    void DrawRadar(const FVector& Center, float ViewYawDegrees, float ActorYawDegrees);

    TArray<FToast> Toasts;
};
