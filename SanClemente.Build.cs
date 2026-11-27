using UnrealBuildTool;

public class SanClemente : ModuleRules
{
    public SanClemente(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "GameplayTags", "AIModule", "NavigationSystem", "UMG", "Slate", "SlateCore"
        });
    }
}
