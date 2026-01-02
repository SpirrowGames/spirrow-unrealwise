#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for core Blueprint commands (creation, compilation, spawn, properties)
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintCoreCommands
{
public:
    FSpirrowBridgeBlueprintCoreCommands();

    // Handle blueprint core commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Blueprint creation and management
    TSharedPtr<FJsonObject> HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetBlueprintProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDuplicateBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBlueprintGraph(const TSharedPtr<FJsonObject>& Params);
};
