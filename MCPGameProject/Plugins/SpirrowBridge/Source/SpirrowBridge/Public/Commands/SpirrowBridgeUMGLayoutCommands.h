#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handles UMG Layout and Designer operations
 * Responsible for layout containers and element manipulation
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGLayoutCommands
{
public:
    FSpirrowBridgeUMGLayoutCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Layout Containers
    TSharedPtr<FJsonObject> HandleAddVerticalBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddHorizontalBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    // Element Operations
    TSharedPtr<FJsonObject> HandleGetWidgetElements(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetWidgetSlotProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetWidgetElementProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleReparentWidgetElement(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRemoveWidgetElement(const TSharedPtr<FJsonObject>& Params);
};
