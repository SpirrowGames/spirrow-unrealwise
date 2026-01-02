#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations for split command handlers
class FSpirrowBridgeBlueprintNodeCoreCommands;
class FSpirrowBridgeBlueprintNodeVariableCommands;
class FSpirrowBridgeBlueprintNodeControlFlowCommands;

/**
 * Handler class for Blueprint Node-related MCP commands
 * This class delegates to specialized sub-handlers for better code organization.
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintNodeCommands
{
public:
    FSpirrowBridgeBlueprintNodeCommands();
    ~FSpirrowBridgeBlueprintNodeCommands();

    // Handle blueprint node commands - routes to appropriate sub-handler
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Sub-handler instances
    TSharedPtr<FSpirrowBridgeBlueprintNodeCoreCommands> CoreCommands;
    TSharedPtr<FSpirrowBridgeBlueprintNodeVariableCommands> VariableCommands;
    TSharedPtr<FSpirrowBridgeBlueprintNodeControlFlowCommands> ControlFlowCommands;
};
