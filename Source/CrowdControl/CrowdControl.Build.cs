// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrowdControl : ModuleRules
{
	public CrowdControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"DeveloperSettings"
				// ... add private dependencies that you statically link with here ...	
			}
		);
	}
}
