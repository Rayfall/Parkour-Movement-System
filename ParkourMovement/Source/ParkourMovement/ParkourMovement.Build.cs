// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ParkourMovement : ModuleRules
{
	public ParkourMovement(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput" });
	}
}
