#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles AI Perception related commands for SpirrowBridge.
 * Includes AIPerceptionComponent and Sense configuration operations.
 * Phase H-1 implementation.
 */
class SPIRROWBRIDGE_API FSpirrowBridgeAIPerceptionCommands
{
public:
	FSpirrowBridgeAIPerceptionCommands();
	~FSpirrowBridgeAIPerceptionCommands();

	/**
	 * Main command handler that routes to specific handlers.
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// ===== AIPerception Component Commands =====

	/**
	 * Add AIPerceptionComponent to a Blueprint (typically AIController).
	 */
	TSharedPtr<FJsonObject> HandleAddAIPerceptionComponent(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure Sight sense on AIPerceptionComponent.
	 */
	TSharedPtr<FJsonObject> HandleConfigureSightSense(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure Hearing sense on AIPerceptionComponent.
	 */
	TSharedPtr<FJsonObject> HandleConfigureHearingSense(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure Damage sense on AIPerceptionComponent.
	 */
	TSharedPtr<FJsonObject> HandleConfigureDamageSense(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set the dominant sense for AIPerceptionComponent.
	 */
	TSharedPtr<FJsonObject> HandleSetPerceptionDominantSense(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add AIPerceptionStimuliSourceComponent to a Blueprint (for detection targets).
	 */
	TSharedPtr<FJsonObject> HandleAddPerceptionStimuliSource(const TSharedPtr<FJsonObject>& Params);

	// ===== Helper Functions =====

	/**
	 * Find AIPerceptionComponent in a Blueprint's component hierarchy.
	 */
	class UAIPerceptionComponent* FindPerceptionComponent(class UBlueprint* Blueprint, const FString& ComponentName);

	/**
	 * Get sense class from type string.
	 */
	UClass* GetSenseClass(const FString& SenseType);

	/**
	 * Parse detection affiliation settings from JSON.
	 */
	void ParseDetectionAffiliation(const TSharedPtr<FJsonObject>& AffiliationJson,
		bool& OutDetectEnemies, bool& OutDetectNeutrals, bool& OutDetectFriendlies);
};
