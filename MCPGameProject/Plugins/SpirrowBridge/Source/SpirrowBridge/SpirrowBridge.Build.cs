// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpirrowBridge : ModuleRules
{
	public SpirrowBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		// Use IWYUSupport instead of the deprecated bEnforceIWYU in UE5.5
		IWYUSupport = IWYUSupport.Full;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"Networking",
				"Sockets",
				"HTTP",
				"Json",
				"JsonUtilities",
				"DeveloperSettings",
				"AIModule",
				"NavigationSystem",
				"RenderCore"  // v0.10.0 bug fix - FlushRenderingCommands for take_pie_screenshot render-thread sync
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"EditorScriptingUtilities",
				"EditorSubsystem",
				"Slate",
				"SlateCore",
				"UMG",
				"MovieScene",
				"MovieSceneTracks",
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"Projects",
				"AssetRegistry",
				"AssetTools",
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks",
				// BehaviorTree Graph support (グラフベースノード作成用)
				"AIGraph",
				"BehaviorTreeEditor"
			}
		);
		
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"PropertyEditor",         // For widget property editing
					"ToolMenus",              // For editor UI
					"BlueprintEditorLibrary", // For Blueprint utilities
					"UMGEditor",              // For WidgetBlueprint.h and other UMG editor functionality
					"LevelEditor",            // For FLevelEditorViewportClient (editor camera control, v0.10.0)
					"LiveCoding",             // For ILiveCodingModule (trigger_live_coding, v0.10.0)
					"OutputLog"               // For FOutputLogModule (tail_editor_output_log, v0.10.0)
				}
			);
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
} 