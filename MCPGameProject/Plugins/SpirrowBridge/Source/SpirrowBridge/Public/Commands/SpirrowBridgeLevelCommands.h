#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Level (.umap) creation MCP commands.
 */
class SPIRROWBRIDGE_API FSpirrowBridgeLevelCommands
{
public:
    FSpirrowBridgeLevelCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateLevel(const TSharedPtr<FJsonObject>& Params);
};
