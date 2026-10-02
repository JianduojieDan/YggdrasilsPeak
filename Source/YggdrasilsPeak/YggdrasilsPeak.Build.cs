// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class YggdrasilsPeak : ModuleRules
{
	public YggdrasilsPeak(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"YggdrasilsPeak",
			"YggdrasilsPeak/Variant_Platforming",
			"YggdrasilsPeak/Variant_Platforming/Animation",
			"YggdrasilsPeak/Variant_Combat",
			"YggdrasilsPeak/Variant_Combat/AI",
			"YggdrasilsPeak/Variant_Combat/Animation",
			"YggdrasilsPeak/Variant_Combat/Gameplay",
			"YggdrasilsPeak/Variant_Combat/Interfaces",
			"YggdrasilsPeak/Variant_Combat/UI",
			"YggdrasilsPeak/Variant_SideScrolling",
			"YggdrasilsPeak/Variant_SideScrolling/AI",
			"YggdrasilsPeak/Variant_SideScrolling/Gameplay",
			"YggdrasilsPeak/Variant_SideScrolling/Interfaces",
			"YggdrasilsPeak/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
