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

	// ===== BT Node Operation Commands (Phase G) =====

	/**
	 * Add a composite node (Selector, Sequence, SimpleParallel) to a BehaviorTree.
	 */
	TSharedPtr<FJsonObject> HandleAddBTCompositeNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a task node to a BehaviorTree.
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a decorator node to a BehaviorTree node.
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a service node to a composite node.
	 */
	TSharedPtr<FJsonObject> HandleAddBTServiceNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Connect nodes in a BehaviorTree (set parent-child relationship).
	 */
	TSharedPtr<FJsonObject> HandleConnectBTNodes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set a property on a BehaviorTree node.
	 */
	TSharedPtr<FJsonObject> HandleSetBTNodeProperty(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Delete a node from a BehaviorTree.
	 */
	TSharedPtr<FJsonObject> HandleDeleteBTNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List available BehaviorTree node types.
	 */
	TSharedPtr<FJsonObject> HandleListBTNodeTypes(const TSharedPtr<FJsonObject>& Params);

	// ===== BT Node Operation Helpers =====

	/**
	 * Get the UClass for a BT composite node type string.
	 */
	UClass* GetBTCompositeNodeClass(const FString& TypeString);

	/**
	 * Get the UClass for a BT task node type string.
	 */
	UClass* GetBTTaskNodeClass(const FString& TypeString);

	/**
	 * Get the UClass for a BT decorator type string.
	 */
	UClass* GetBTDecoratorClass(const FString& TypeString);

	/**
	 * Get the UClass for a BT service type string.
	 */
	UClass* GetBTServiceClass(const FString& TypeString);

	/**
	 * Find a BT node by ID within a BehaviorTree.
	 */
	class UBTNode* FindBTNodeById(class UBehaviorTree* BehaviorTree, const FString& NodeId);

	/**
	 * Convert a BT node to JSON representation.
	 */
	TSharedPtr<FJsonObject> BTNodeToJson(class UBTNode* Node);

	/**
	 * Get description for a composite node type.
	 */
	FString GetCompositeDescription(const FString& Type);

	/**
	 * Get description for a task node type.
	 */
	FString GetTaskDescription(const FString& Type);

	/**
	 * Get description for a decorator node type.
	 */
	FString GetDecoratorDescription(const FString& Type);

	/**
	 * Get description for a service node type.
	 */
	FString GetServiceDescription(const FString& Type);
};
