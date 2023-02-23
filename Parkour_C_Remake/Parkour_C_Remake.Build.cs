// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Parkour_C_Remake : ModuleRules
{
	public Parkour_C_Remake(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
