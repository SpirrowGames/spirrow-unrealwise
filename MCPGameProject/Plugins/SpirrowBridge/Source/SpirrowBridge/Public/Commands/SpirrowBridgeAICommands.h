#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles AI-related commands for SpirrowBridge.
 * Includes BehaviorTree and Blackboard operations.
 */
class SPIRROWBRIDGE_API FSpirrowBridgeAICommands
{
public:
	FSpirrowBridgeAICommands();
	~FSpirrowBridgeAICommands();

	/**
	 * Main command handler that routes to specific handlers.
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// ===== Blackboard Commands =====

	/**
	 * Create a new Blackboard Data Asset.
	 */
	TSharedPtr<FJsonObject> HandleCreateBlackboard(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a key to an existing Blackboard.
	 */
	TSharedPtr<FJsonObject> HandleAddBlackboardKey(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove a key from a Blackboard.
	 */
	TSharedPtr<FJsonObject> HandleRemoveBlackboardKey(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List all keys in a Blackboard.
	 */
	TSharedPtr<FJsonObject> HandleListBlackboardKeys(const TSharedPtr<FJsonObject>& Params);

	// ===== BehaviorTree Commands =====

	/**
	 * Create a new BehaviorTree Asset.
	 */
	TSharedPtr<FJsonObject> HandleCreateBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set the Blackboard asset for a BehaviorTree.
	 */
	TSharedPtr<FJsonObject> HandleSetBehaviorTreeBlackboard(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get the structure of a BehaviorTree (nodes, connections).
	 */
	TSharedPtr<FJsonObject> HandleGetBehaviorTreeStructure(const TSharedPtr<FJsonObject>& Params);

	// ===== Utility Commands =====

	/**
	 * List AI-related assets in the project.
	 */
	TSharedPtr<FJsonObject> HandleListAIAssets(const TSharedPtr<FJsonObject>& Params);

	// ===== Helper Functions =====

	/**
	 * Find a Blackboard asset by name and path.
	 */
	class UBlackboardData* FindBlackboardAsset(const FString& Name, const FString& Path);

	/**
	 * Find a BehaviorTree asset by name and path.
	 */
	class UBehaviorTree* FindBehaviorTreeAsset(const FString& Name, const FString& Path);

	/**
	 * Get the UClass for a Blackboard key type string.
	 */
	UClass* GetBlackboardKeyTypeClass(const FString& TypeString);

	/**
	 * Convert a Blackboard key to JSON representation.
	 */
	TSharedPtr<FJsonObject> BlackboardKeyToJson(const struct FBlackboardEntry& Entry);
};
