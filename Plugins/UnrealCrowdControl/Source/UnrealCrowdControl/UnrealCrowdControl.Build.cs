// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class UnrealCrowdControl : ModuleRules
{
	public UnrealCrowdControl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities" });
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"DeveloperSettings"
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string BaseDirectory = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
			string BinariesDirectory = Path.Combine(BaseDirectory, "Binaries", "Win64");
			
			PublicDelayLoadDLLs.Add("libssl-3-x64.dll");
			PublicDelayLoadDLLs.Add("libcrypto-3-x64.dll");
			PublicDelayLoadDLLs.Add("cpprest_2_10d.dll");
		}
	}
}
