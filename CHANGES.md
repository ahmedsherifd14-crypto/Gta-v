# CHANGES — San Clemente foundation repair

## Compilation / verification status
- **Not compiled. Not opened in Unreal Editor.** No Unreal Engine or Visual Studio was available in the authoring environment.
- Performed only: manual code review + a static check (balanced braces, local includes exist, `.generated.h` last). That is NOT a compile.

## Problems found in the original
1. `Map_Start` referenced but absent -> missing-map error on open.
2. `Interact` input mapped but never bound; no way to trigger save/load.
3. Money existed only as a parameter; nothing owned a wallet.
4. Character had no mesh (invisible), no PlayerStart, no ground -> nothing playable.
5. `SCSaveSubsystem.h` used `ACharacter` without declaring it; loader rejected any version but 1; control rotation/velocity not restored.
6. Bogus `ActiveGameNameRedirects` line in `DefaultEngine.ini`.
7. Sprint set speed directly (no stamina, stuck sprinting while crouched), crouch had no HUD/visual feedback.

## Modified files
- `Config/DefaultEngine.ini` — startup map -> `/Engine/Maps/Entry`, removed bogus redirect, SM6 + virtual shadows.
- `Config/DefaultInput.ini` — explicit legacy input classes, gamepad, vehicle axes, zoom, save/load keys.
- `Config/DefaultGame.ini` — `bUseGeneratedTestWorld`.
- `Source/SanClemente/SanClemente.Build.cs` — added Slate/SlateCore.
- `SCGameMode.h/.cpp` — HUD/controller classes, day/night clock, fallback PlayerStart, runtime test world.
- `SCPlayerCharacter.h/.cpp` — wallet, health, stamina sprint, crouch visuals, fall damage, death/respawn, interaction scan, camera zoom, gamepad look.
- `SCGameSave.h` — save v2 (health, time of day), `CurrentSaveVersion`.
- `SCSaveSubsystem.h/.cpp` — forward decl fix, v1+v2 loading, restores rotation/controller/health/time, `DeleteSave`.
- `README.md` — rewritten.

## New files
`SCBasicShapes.h`, `SCInteractable.h`, `SCVehicle.h/.cpp`, `SCPlayerController.h/.cpp`, `SCHUD.h/.cpp`, `SCMoneyPickup.h/.cpp`, `SCSavePoint.h/.cpp`, `SCTestWorldBuilder.h/.cpp`, `Tools/CreateStartMap.py`, `.gitignore`, `CHANGES.md`.

## Remaining / unverified
- First real compile may surface version-specific errors (most likely spots: interface `_Implementation ... override`, `SkyLightComponent`/light component properties, `BasicShapeMaterial` "Color" parameter).
- `Tools/CreateStartMap.py` never executed.
- Not implemented: pedestrians/traffic AI, wanted/police, weapons, missions, street lights, vehicle lights/audio, saved pickup state.

## Update 2 — combat, NPCs, wanted level, real-character slots
Still NOT compiled / run. New files: `SCWeaponComponent`, `SCNPC`. Modified: `SCPlayerCharacter` (weapons input, aim camera, strafe stance, real skeletal mesh + weapon socket slots, vehicle-eject on death), `SCGameMode` (wanted level, police spawner), `SCTestWorldBuilder` (civilians, `GetRandomRoadPoint`), `SCHUD` (stars, ammo, crosshair, NPC radar blips), `DefaultInput.ini` (fire/aim/reload/weapons).
Real character/weapon ART is not included (cannot be produced or downloaded here) — see README "Real characters and weapons".
