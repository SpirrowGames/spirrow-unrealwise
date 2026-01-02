#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations for split command handlers
class FSpirrowBridgeBlueprintCoreCommands;
class FSpirrowBridgeBlueprintComponentCommands;
class FSpirrowBridgeBlueprintPropertyCommands;

/**
 * Handler class for Blueprint-related MCP commands
 * This class delegates to specialized sub-handlers for better code organization.
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintCommands
{
public:
    FSpirrowBridgeBlueprintCommands();
    ~FSpirrowBridgeBlueprintCommands();

    // Handle blueprint commands - routes to appropriate sub-handler
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Sub-handler instances
    TSharedPtr<FSpirrowBridgeBlueprintCoreCommands> CoreCommands;
    TSharedPtr<FSpirrowBridgeBlueprintComponentCommands> ComponentCommands;
    TSharedPtr<FSpirrowBridgeBlueprintPropertyCommands> PropertyCommands;
};
