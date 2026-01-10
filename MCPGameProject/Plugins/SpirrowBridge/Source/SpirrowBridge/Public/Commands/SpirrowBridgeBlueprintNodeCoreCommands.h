#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for core Blueprint node commands (connection, search, events, functions)
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintNodeCoreCommands
{
public:
    FSpirrowBridgeBlueprintNodeCoreCommands();

    // Handle blueprint node core commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Node connection and search
    TSharedPtr<FJsonObject> HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDisconnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetNodePinValue(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleMoveNode(const TSharedPtr<FJsonObject>& Params);
    
    // Event and function nodes
    TSharedPtr<FJsonObject> HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params);
};
