# San Clemente — Unreal Engine 5 playable foundation

An original open-world crime-action starter, in the *spirit* of modern open-world action games (on-foot + driving, a living city, day/night, radar HUD). It uses **no third-party or Rockstar assets, names or IP**.

> **Status:** source-complete foundation, **not yet compiled or run** — the authoring environment had no Unreal Engine installed. Build it locally (steps below) and treat the first compile as the real verification. It is **not** a finished open-world game.

## What you get
- **Player (third-person):** walk, sprint with stamina, jump, crouch (body shrinks with the capsule), over-the-shoulder camera with lag, mouse-wheel zoom, gamepad support.
- **Vitals:** health, fall damage, "WASTED" and hospital respawn (fee deducted).
- **Interaction framework:** `ISCInteractable` interface + focus scanning + HUD prompt. Used by cars and save points.
- **Drivable cars:** arcade physics, enter/exit (`E`), handbrake, chase camera that re-centres, collision slide/bounce. Placeholder meshes from engine primitives.
- **Save / load:** `USCSaveSubsystem` saves location, rotation, money, health and time of day (versioned; v1 saves still load). `F5`/`F9` quick save/load, or interact with the cyan save pillar.
- **HUD:** money, clock, health/stamina, rotating radar (cars, cash, save point), speedometer, prompts, toasts.
- **Test environment (runtime generated):** 7×7 block city with road grid, lane marks, towers, plaza with fountain/trees, 8 parked cars, ~34 cash bags, save point, sun + moon + sky atmosphere on a 48-minute day.

## Combat, police and wanted level (added in update 2)
- **4 hitscan weapons** (generic originals): Pistol, SMG, Assault Rifle, Shotgun. Ammo, reload, spread, auto/semi fire, aim-down-sights camera, crosshair. Tune everything in `USCWeaponComponent::Weapons` (editable in the editor).
- **NPCs (`ASCNPC`)**: 24 civilians wander the roads and flee from gunfire; they drop cash when killed.
- **Wanted level (0–5 stars)**: hitting/killing civilians or cops raises it; police spawn on nearby roads, chase and shoot you; it cools down after ~20 s without crimes; dying clears it.
- Tracers/impacts are debug-draw placeholders (no VFX/sound yet).

## Real characters and weapons (art you must add — this repo contains no art)
Real character models, animations and weapon models are binary art assets. They cannot be generated from code, and none could be downloaded in the authoring environment. The code is ready to use them:
1. **Fastest:** in the editor use **Add > Add Feature or Content Pack > Third Person**. This creates `/Game/Characters/Mannequins/...`. The player (Manny) and NPCs (Quinn) pick the mesh and animation blueprint up automatically on the next Play — no code change.
2. **Better quality:** MetaHuman (Quixel Bridge), Fab/Marketplace characters, or Mixamo. Retarget to the UE5 mannequin skeleton, then set `CharacterSkeletalMesh` / `CharacterAnimClass` (and `CharacterMeshYawOffset` if the model faces the wrong way) in the character's class defaults (Blueprint child of `SCPlayerCharacter` / `SCNPC`).
3. **Weapons:** assign a skeletal mesh to `FSCWeaponData::WeaponMesh` for each weapon in `USCWeaponComponent::Weapons`; it attaches to the `hand_r` socket (`WeaponSocketName`). Without a mesh a black placeholder box is shown (and nothing on a real character).
4. Vehicles: replace the placeholder primitives in `ASCVehicle` with a car mesh (a Blueprint child with a Static/Skeletal Mesh and the box sized to fit).

## Controls
| Action | Keyboard / Mouse | Gamepad |
|---|---|---|
| Move | W A S D | Left stick |
| Camera | Mouse | Right stick |
| Zoom | Mouse wheel | – |
| Sprint | Left Shift | L3 |
| Jump | Space | A / Cross |
| Crouch (toggle) | Left Ctrl / C | R3 |
| Interact / enter / exit car | E | X / Square |
| Drive: throttle / brake-reverse | W / S | RT / LT |
| Drive: steer | A / D | Left stick |
| Drive: handbrake | Space | A / Cross |
| Quick save / load | F5 / F9 | – |
| Aim / fire | Right / Left mouse | LT / RT |
| Reload | R | Y / Triangle |
| Weapons | 1-4, Q (next) | RB |

## Open and build (Windows)
1. Install **Unreal Engine 5.4** (Epic Games Launcher) and **Visual Studio 2022** with *Game development with C++* and the Windows 10/11 SDK.
2. Extract this folder somewhere with a short path (e.g. `C:\Dev\SanClemente`).
3. Right-click `SanClemente.uproject` → **Generate Visual Studio project files**.
4. Open `SanClemente.sln`, set **Development Editor / Win64**, build target `SanClemente` (the editor target is `SanClementeEditor`).
5. Run, or double-click `SanClemente.uproject` and accept "rebuild modules" if prompted.
6. Press **Play** (Alt+P). You spawn on the plaza; cars are on the surrounding roads.

Command-line alternative (adjust paths):
```
"C:\Program Files\Epic Games\UE_5.4\Engine\Build\BatchFiles\Build.bat" SanClementeEditor Win64 Development -Project="C:\Dev\SanClemente\SanClemente.uproject" -WaitMutex
```

### Other UE5 versions
`EngineAssociation` is `5.4`. For 5.5+ right-click the `.uproject` → *Switch Unreal Engine version*, regenerate project files, and fix any API-drift messages. `IncludeOrderVersion` stays at `Unreal5_4`, which newer engines accept.

## Startup map
`Content/Maps/Map_Start.umap` is binary and cannot be authored without the editor, so the project avoids a missing-map error by starting on the engine's built-in blank map (`/Engine/Maps/Entry`). `ASCGameMode` builds the city at runtime and supplies its own `PlayerStart`.

To get a permanent project map: enable the *Python Editor Script Plugin*, run `Tools/CreateStartMap.py` via **File → Execute Python Script**, restart. For your own authored levels set `bUseGeneratedTestWorld=False` (in `Config/DefaultGame.ini` under `[/Script/SanClemente.SCGameMode]`) and place your own `PlayerStart`.

## Known limitations
- Never compiled/run here: expect to fix a few version-specific compiler messages.
- Car/character meshes are primitives; paint colour relies on the engine `BasicShapeMaterial` exposing a `Color` parameter (if not, objects stay grey — gameplay is unaffected).
- Cars have no suspension, damage, traffic AI, lights or audio. No missions, phone, radio, drive-by shooting, melee, cover system or ragdoll.
- NPC AI is simple (no navmesh/behavior trees): they walk straight and re-pick direction when blocked, so they can get stuck on walls.
- Weapon ammo is not saved; weapons give no ammo pickups yet.
- Cash pickups respawn on level restart (collected state isn't saved).
- Saving is disabled while inside a vehicle.
- Night is dark (no street lamps yet).

## Suggested next slices
Street lamps + vehicle headlights → traffic and pedestrian AI (`AIModule`/`NavigationSystem` are already linked) → weapons + wanted level → mission data assets using `CurrentMissionId` in the save → World Partition once the core loop is verified.
