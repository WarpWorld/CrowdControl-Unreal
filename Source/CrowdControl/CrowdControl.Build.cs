// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CrowdControl : ModuleRules {
	public CrowdControl(TargetInfo Target) {
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

        // Manually specify the relative path to your 'ThirdParty' folder
        string ThirdPartyPath = Path.GetFullPath(Path.Combine("../../../ThirdParty/WarpWorld/"));
		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));  // Ensure this points to the correct subdirectory

        //PublicAdditionalLibraries.Add("Binaries/Win64/CrowdControl.lib"); 
	}
}
