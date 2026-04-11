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

    // External property (UPROPERTY on another class, e.g. Subsystem fields)
    TSharedPtr<FJsonObject> HandleAddExternalPropertySetNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddExternalPropertyGetNode(const TSharedPtr<FJsonObject>& Params);

    // Typed subsystem accessor (UK2Node_GetSubsystem with class baked in)
    TSharedPtr<FJsonObject> HandleAddGetSubsystemNode(const TSharedPtr<FJsonObject>& Params);
};
