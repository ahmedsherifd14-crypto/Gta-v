"""
OPTIONAL: creates a permanent, empty /Game/Maps/Map_Start level and points the project at it.

You do NOT need this to play: the project defaults to the engine's blank /Engine/Maps/Entry map and
ASCGameMode builds the test city at runtime.

How to run (inside Unreal Editor, after the C++ module has compiled):
  1. Edit > Plugins > enable "Python Editor Script Plugin" (restart the editor).
  2. File > Execute Python Script... > pick this file.
  3. Restart the editor. Map_Start opens automatically and Play works the same way.

NOTE: written against the UE 5.4 Python API and not executed in the authoring environment.
"""

import os
import re
import unreal

MAP_PATH = "/Game/Maps/Map_Start"


def create_map():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log("Map_Start already exists - leaving it untouched.")
        return True

    unreal.EditorAssetLibrary.make_directory("/Game/Maps")
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.new_level(MAP_PATH):
        unreal.log_error("Could not create " + MAP_PATH)
        return False

    level_subsystem.save_current_level()
    unreal.log("Created " + MAP_PATH)
    return True


def patch_engine_ini():
    ini_path = os.path.join(unreal.Paths.project_config_dir(), "DefaultEngine.ini")
    with open(ini_path, "r", encoding="utf-8") as handle:
        text = handle.read()

    target = "/Game/Maps/Map_Start.Map_Start"
    text = re.sub(r"(?m)^GameDefaultMap=.*$", "GameDefaultMap=" + target, text)
    text = re.sub(r"(?m)^EditorStartupMap=.*$", "EditorStartupMap=" + target, text)

    with open(ini_path, "w", encoding="utf-8") as handle:
        handle.write(text)
    unreal.log("DefaultEngine.ini now points at " + target + " (restart the editor to apply).")


if create_map():
    patch_engine_ini()
