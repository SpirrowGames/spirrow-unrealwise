#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

/**
 * Handles EQS (Environment Query System) related commands for SpirrowBridge.
 * Includes EQS Query creation, Generator, and Test operations.
 * Phase H-2 implementation.
 */
class SPIRROWBRIDGE_API FSpirrowBridgeEQSCommands
{
public:
	FSpirrowBridgeEQSCommands();
	~FSpirrowBridgeEQSCommands();

	/**
	 * Main command handler that routes to specific handlers.
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// ===== EQS Query Commands =====

	/**
	 * Create a new EQS Query asset.
	 */
	TSharedPtr<FJsonObject> HandleCreateEQSQuery(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a Generator to an EQS Query.
	 */
	TSharedPtr<FJsonObject> HandleAddEQSGenerator(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a Test to an EQS Query Generator.
	 */
	TSharedPtr<FJsonObject> HandleAddEQSTest(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set a property on an EQS Test.
	 */
	TSharedPtr<FJsonObject> HandleSetEQSTestProperty(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List EQS Query assets in the project.
	 */
	TSharedPtr<FJsonObject> HandleListEQSAssets(const TSharedPtr<FJsonObject>& Params);

	// ===== Helper Functions =====

	/**
	 * Find an EQS Query asset by name and path.
	 */
	class UEnvQuery* FindEQSQueryAsset(const FString& Name, const FString& Path);

	/**
	 * Get Generator class from type string.
	 */
	UClass* GetGeneratorClass(const FString& GeneratorType);

	/**
	 * Get Test class from type string.
	 */
	UClass* GetTestClass(const FString& TestType);

	/**
	 * Get scoring equation enum from string.
	 */
	EEnvTestScoreEquation::Type GetScoringEquation(const FString& EquationString);

	/**
	 * Get test purpose enum from string.
	 */
	EEnvTestPurpose::Type GetTestPurpose(const FString& PurposeString);

	/**
	 * Convert an EQS Query to JSON representation.
	 */
	TSharedPtr<FJsonObject> EQSQueryToJson(class UEnvQuery* Query);
};
