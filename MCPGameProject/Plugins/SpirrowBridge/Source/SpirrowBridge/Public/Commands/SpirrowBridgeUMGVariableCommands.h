#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "EdGraphSchema_K2.h"

/**
 * Handles UMG Widget Variable, Function, and Binding operations
 * Responsible for Blueprint-side widget logic
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGVariableCommands
{
public:
    FSpirrowBridgeUMGVariableCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Variables
    TSharedPtr<FJsonObject> HandleAddWidgetVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetArrayVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetWidgetVariableDefault(const TSharedPtr<FJsonObject>& Params);

    // Functions & Events
    TSharedPtr<FJsonObject> HandleAddWidgetFunction(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetEvent(const TSharedPtr<FJsonObject>& Params);

    // Bindings
    TSharedPtr<FJsonObject> HandleBindWidgetToVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleBindWidgetEvent(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params);

    // Helper
    bool SetupPinType(const FString& TypeName, FEdGraphPinType& OutPinType);
};
