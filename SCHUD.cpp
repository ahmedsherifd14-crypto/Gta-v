#include "SCHUD.h"
#include "SCGameMode.h"
#include "SCMoneyPickup.h"
#include "SCNPC.h"
#include "SCPlayerCharacter.h"
#include "SCWeaponComponent.h"
#include "SCSavePoint.h"
#include "SCVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void ASCHUD::PushMessage(const FString& Message, float Duration)
{
    FToast Toast;
    Toast.Text = Message;
    Toast.ExpireTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) + Duration;
    Toasts.Add(Toast);
    if (Toasts.Num() > 4)
    {
        Toasts.RemoveAt(0);
    }
}

void ASCHUD::DrawShadowedText(const FString& Text, const FLinearColor& Color, float X, float Y, UFont* Font, float Scale)
{
    DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), X + 2.0f, Y + 2.0f, Font, Scale);
    DrawText(Text, Color, X, Y, Font, Scale);
}

void ASCHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill)
{
    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f), X - 2.0f, Y - 2.0f, Width + 4.0f, Height + 4.0f);
    DrawRect(Fill, X, Y, Width * FMath::Clamp(Fraction, 0.0f, 1.0f), Height);
}

void ASCHUD::DrawRadar(const FVector& Center, float ViewYawDegrees, float ActorYawDegrees)
{
    const float Margin = 30.0f;
    const float Left = Margin;
    const float Top = Canvas->ClipY - RadarSize - Margin - 60.0f;
    const float CenterX = Left + RadarSize * 0.5f;
    const float CenterY = Top + RadarSize * 0.5f;
    const float Half = RadarSize * 0.5f;
    const float PixelsPerCm = Half / RadarRange;

    // Background + frame.
    DrawRect(FLinearColor(0.02f, 0.04f, 0.06f, 0.75f), Left, Top, RadarSize, RadarSize);
    const FLinearColor Frame(0.9f, 0.9f, 0.9f, 0.9f);
    DrawRect(Frame, Left - 2.0f, Top - 2.0f, RadarSize + 4.0f, 2.0f);
    DrawRect(Frame, Left - 2.0f, Top + RadarSize, RadarSize + 4.0f, 2.0f);
    DrawRect(Frame, Left - 2.0f, Top, 2.0f, RadarSize);
    DrawRect(Frame, Left + RadarSize, Top, 2.0f, RadarSize);
    DrawLine(CenterX, Top, CenterX, Top + RadarSize, FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
    DrawLine(Left, CenterY, Left + RadarSize, CenterY, FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));

    const float ViewYawRad = FMath::DegreesToRadians(ViewYawDegrees);
    const FVector2D Forward(FMath::Cos(ViewYawRad), FMath::Sin(ViewYawRad));
    const FVector2D Right(-FMath::Sin(ViewYawRad), FMath::Cos(ViewYawRad));

    auto DrawBlip = [&](const FVector& WorldLocation, const FLinearColor& Color, float Size)
    {
        const FVector2D Rel(WorldLocation.X - Center.X, WorldLocation.Y - Center.Y);
        float PX = FVector2D::DotProduct(Rel, Right) * PixelsPerCm;
        float PY = -FVector2D::DotProduct(Rel, Forward) * PixelsPerCm;
        const float Limit = Half - Size - 2.0f;
        PX = FMath::Clamp(PX, -Limit, Limit);
        PY = FMath::Clamp(PY, -Limit, Limit);
        DrawRect(Color, CenterX + PX - Size * 0.5f, CenterY + PY - Size * 0.5f, Size, Size);
    };

    UWorld* World = GetWorld();
    for (TActorIterator<ASCMoneyPickup> It(World); It; ++It)
    {
        DrawBlip(It->GetActorLocation(), FLinearColor(0.2f, 0.9f, 0.3f), 5.0f);
    }
    for (TActorIterator<ASCSavePoint> It(World); It; ++It)
    {
        DrawBlip(It->GetActorLocation(), FLinearColor(0.2f, 0.8f, 1.0f), 8.0f);
    }
    for (TActorIterator<ASCVehicle> It(World); It; ++It)
    {
        DrawBlip(It->GetActorLocation(), FLinearColor(1.0f, 0.8f, 0.1f), 7.0f);
    }
    for (TActorIterator<ASCNPC> It(World); It; ++It)
    {
        if (It->IsDead()) continue;
        if (It->GetRole() == ESCNPCRole::Police)
        {
            DrawBlip(It->GetActorLocation(), FLinearColor(0.2f, 0.35f, 1.0f), 7.0f);
        }
        else
        {
            DrawBlip(It->GetActorLocation(), FLinearColor(0.6f, 0.6f, 0.6f), 3.0f);
        }
    }

    // Player marker: a dot with a facing line (rotated relative to the camera).
    const float RelYawRad = FMath::DegreesToRadians(ActorYawDegrees - ViewYawDegrees);
    const float LineLength = 16.0f;
    DrawLine(CenterX, CenterY, CenterX + FMath::Sin(RelYawRad) * LineLength, CenterY - FMath::Cos(RelYawRad) * LineLength, FLinearColor::White, 3.0f);
    DrawRect(FLinearColor::White, CenterX - 4.0f, CenterY - 4.0f, 8.0f, 8.0f);

    // North marker (world +X) on the radar edge, rotating with the camera.
    const FVector2D North(1.0f, 0.0f);
    const float NorthX = FVector2D::DotProduct(North, Right) * (Half - 14.0f);
    const float NorthY = -FVector2D::DotProduct(North, Forward) * (Half - 14.0f);
    DrawShadowedText(TEXT("N"), FLinearColor(1.0f, 0.3f, 0.3f), CenterX + NorthX - 4.0f, CenterY + NorthY - 8.0f, GEngine->GetSmallFont(), 1.0f);
}

void ASCHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !GetWorld())
    {
        return;
    }

    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    UFont* LargeFont = GEngine->GetLargeFont();
    UFont* MediumFont = GEngine->GetMediumFont();
    UFont* SmallFont = GEngine->GetSmallFont();

    ASCPlayerCharacter* Character = Cast<ASCPlayerCharacter>(PC->GetPawn());
    ASCVehicle* Vehicle = Cast<ASCVehicle>(PC->GetPawn());
    ASCPlayerCharacter* Player = Character;
    if (!Player && Vehicle)
    {
        Player = Vehicle->GetDriver();
    }

    const float RightEdge = Canvas->ClipX - 40.0f;

    // ---- Money (top right) ----
    if (Player)
    {
        const FString MoneyText = FString::Printf(TEXT("$%s"), *FString::FormatAsNumber(Player->GetMoney()));
        float W = 0.0f, H = 0.0f;
        GetTextSize(MoneyText, W, H, LargeFont, 1.6f);
        DrawShadowedText(MoneyText, FLinearColor(0.35f, 0.95f, 0.4f), RightEdge - W, 28.0f, LargeFont, 1.6f);
    }

    // ---- Clock ----
    if (const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
    {
        const FString Clock = GameMode->GetClockString();
        float W = 0.0f, H = 0.0f;
        GetTextSize(Clock, W, H, MediumFont, 1.2f);
        DrawShadowedText(Clock, FLinearColor::White, RightEdge - W, 84.0f, MediumFont, 1.2f);
    }

    // ---- Wanted stars (top right, under the clock) ----
    if (const ASCGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCGameMode>())
    {
        const int32 Wanted = GameMode->GetWantedLevel();
        const float StarSize = 20.0f;
        const float StarsLeft = RightEdge - 5.0f * (StarSize + 6.0f);
        const bool bFlash = Wanted > 0 && FMath::Fmod(Now, 0.8f) < 0.4f;
        for (int32 i = 0; i < 5; ++i)
        {
            const bool bLit = i < Wanted;
            const FLinearColor Color = bLit ? (bFlash ? FLinearColor(1.0f, 1.0f, 1.0f) : FLinearColor(1.0f, 0.85f, 0.1f))
                                            : FLinearColor(0.25f, 0.25f, 0.25f, 0.7f);
            DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f), StarsLeft + i * (StarSize + 6.0f) - 2.0f, 118.0f, StarSize + 4.0f, StarSize + 4.0f);
            DrawRect(Color, StarsLeft + i * (StarSize + 6.0f), 120.0f, StarSize, StarSize);
        }
    }

    // ---- Weapon / ammo (bottom right) and crosshair ----
    if (Character && !Character->IsDead() && Character->GetWeaponComponent())
    {
        const USCWeaponComponent* Weapons = Character->GetWeaponComponent();
        const FString WeaponName = Weapons->GetCurrentWeapon().Name.ToString();
        const FString Ammo = Weapons->IsReloading()
            ? FString(TEXT("RELOADING..."))
            : FString::Printf(TEXT("%d / %d"), Weapons->GetMagAmmo(), Weapons->GetReserveAmmo());
        float W = 0.0f, H = 0.0f;
        GetTextSize(Ammo, W, H, LargeFont, 1.3f);
        DrawShadowedText(Ammo, Weapons->GetMagAmmo() == 0 && !Weapons->IsReloading() ? FLinearColor(1.0f, 0.3f, 0.3f) : FLinearColor::White,
            RightEdge - W, Canvas->ClipY - 100.0f, LargeFont, 1.3f);
        GetTextSize(WeaponName, W, H, MediumFont, 1.0f);
        DrawShadowedText(WeaponName, FLinearColor(0.85f, 0.85f, 0.85f), RightEdge - W, Canvas->ClipY - 56.0f, MediumFont, 1.0f);

        // Crosshair: small dot normally, expanded cross while aiming.
        const float CX = Canvas->ClipX * 0.5f;
        const float CY = Canvas->ClipY * 0.5f;
        const FLinearColor CrossColor(1.0f, 1.0f, 1.0f, 0.9f);
        if (Character->IsAiming())
        {
            const float Gap = 8.0f + Weapons->GetCurrentWeapon().SpreadDegrees * 3.0f;
            DrawRect(CrossColor, CX - Gap - 10.0f, CY - 1.0f, 10.0f, 2.0f);
            DrawRect(CrossColor, CX + Gap, CY - 1.0f, 10.0f, 2.0f);
            DrawRect(CrossColor, CX - 1.0f, CY - Gap - 10.0f, 2.0f, 10.0f);
            DrawRect(CrossColor, CX - 1.0f, CY + Gap, 2.0f, 10.0f);
        }
        else
        {
            DrawRect(CrossColor, CX - 2.0f, CY - 2.0f, 4.0f, 4.0f);
        }
    }

    // ---- Radar + vitals (bottom left) ----
    const FVector PlayerLocation = PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
    const float ViewYaw = PC->GetControlRotation().Yaw;
    const float ActorYaw = PC->GetPawn() ? PC->GetPawn()->GetActorRotation().Yaw : ViewYaw;
    DrawRadar(PlayerLocation, ViewYaw, ActorYaw);

    if (Player)
    {
        const float BarX = 30.0f;
        const float BarY = Canvas->ClipY - 84.0f;
        const float BarW = RadarSize;
        DrawBar(BarX, BarY, BarW, 12.0f, Player->GetHealth() / Player->GetMaxHealth(), FLinearColor(0.85f, 0.12f, 0.12f));
        DrawBar(BarX, BarY + 24.0f, BarW, 8.0f, Player->GetStamina() / Player->GetMaxStamina(), FLinearColor(0.95f, 0.8f, 0.1f));
    }

    // ---- Speedometer (bottom right, in a car) ----
    if (Vehicle)
    {
        const int32 Kmh = FMath::RoundToInt(FMath::Abs(Vehicle->GetForwardSpeed()) * 0.036f);
        const FString Speed = FString::Printf(TEXT("%d km/h"), Kmh);
        float W = 0.0f, H = 0.0f;
        GetTextSize(Speed, W, H, LargeFont, 1.5f);
        DrawShadowedText(Speed, FLinearColor::White, RightEdge - W, Canvas->ClipY - 100.0f, LargeFont, 1.5f);
        DrawShadowedText(TEXT("[E] Exit   [Space] Handbrake"), FLinearColor(0.85f, 0.85f, 0.85f),
            RightEdge - 290.0f, Canvas->ClipY - 56.0f, SmallFont, 1.0f);
    }

    // ---- Interaction prompt (bottom centre) ----
    if (Character && !Character->IsDead())
    {
        const FText Prompt = Character->GetFocusedPrompt();
        if (!Prompt.IsEmpty())
        {
            const FString PromptText = FString::Printf(TEXT("[E]  %s"), *Prompt.ToString());
            float W = 0.0f, H = 0.0f;
            GetTextSize(PromptText, W, H, MediumFont, 1.3f);
            const float X = (Canvas->ClipX - W) * 0.5f;
            const float Y = Canvas->ClipY * 0.72f;
            DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), X - 18.0f, Y - 8.0f, W + 36.0f, H + 16.0f);
            DrawShadowedText(PromptText, FLinearColor::White, X, Y, MediumFont, 1.3f);
        }
    }

    // ---- Toasts (top centre) ----
    Toasts.RemoveAll([Now](const FToast& T) { return T.ExpireTime <= Now; });
    float ToastY = 36.0f;
    for (const FToast& Toast : Toasts)
    {
        float W = 0.0f, H = 0.0f;
        GetTextSize(Toast.Text, W, H, MediumFont, 1.2f);
        DrawShadowedText(Toast.Text, FLinearColor(1.0f, 0.95f, 0.6f), (Canvas->ClipX - W) * 0.5f, ToastY, MediumFont, 1.2f);
        ToastY += H + 8.0f;
    }

    // ---- Controls hint for the first seconds ----
    if (Now < 25.0f && Character)
    {
        DrawShadowedText(TEXT("WASD move | Shift sprint | Ctrl crouch | Space jump | RMB aim | LMB fire | R reload | 1-4/Q weapons | E interact | F5/F9 save/load"),
            FLinearColor(0.9f, 0.9f, 0.9f, 0.9f), 30.0f, Canvas->ClipY - 28.0f, SmallFont, 1.0f);
    }

    // ---- Wasted ----
    if (Character && Character->IsDead())
    {
        DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 0.0f, Canvas->ClipY * 0.4f, Canvas->ClipX, Canvas->ClipY * 0.2f);
        const FString Wasted = TEXT("WASTED");
        float W = 0.0f, H = 0.0f;
        GetTextSize(Wasted, W, H, LargeFont, 4.0f);
        DrawShadowedText(Wasted, FLinearColor(0.85f, 0.1f, 0.1f), (Canvas->ClipX - W) * 0.5f, (Canvas->ClipY - H) * 0.5f, LargeFont, 4.0f);
    }
}
