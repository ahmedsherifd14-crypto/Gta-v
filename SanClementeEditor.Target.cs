using UnrealBuildTool;
using System.Collections.Generic;

public class SanClementeEditorTarget : TargetRules
{
    public SanClementeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("SanClemente");
    }
}
