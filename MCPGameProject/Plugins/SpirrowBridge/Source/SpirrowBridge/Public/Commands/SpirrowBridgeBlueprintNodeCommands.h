#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint Node-related MCP commands
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintNodeCommands
{
public:
    FSpirrowBridgeBlueprintNodeCommands();

    // Handle blueprint node commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Specific blueprint node command handlers
    TSharedPtr<FJsonObject> HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params);
    
    // New node manipulation commands
    TSharedPtr<FJsonObject> HandleSetNodePinValue(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddVariableGetNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddVariableSetNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBranchNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleMoveNode(const TSharedPtr<FJsonObject>& Params);
}; 