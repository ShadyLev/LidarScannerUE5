// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LidarScanner : ModuleRules
{
	public LidarScanner(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			// Data interface dependencies
			"Niagara", "NiagaraCore", "VectorVM", "RenderCore", "RHI"
		});
	}
}
