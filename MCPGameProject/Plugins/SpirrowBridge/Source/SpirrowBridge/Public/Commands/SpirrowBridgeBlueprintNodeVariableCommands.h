#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint variable and reference node commands
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintNodeVariableCommands
{
public:
    FSpirrowBridgeBlueprintNodeVariableCommands();

    // Handle blueprint node variable commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Variable nodes
    TSharedPtr<FJsonObject> HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddVariableGetNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddVariableSetNode(const TSharedPtr<FJsonObject>& Params);
    
    // Reference nodes
    TSharedPtr<FJsonObject> HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params);
};
