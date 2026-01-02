#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "Widgets/Layout/Anchors.h"

/**
 * Handles UMG Widget creation commands
 * Responsible for adding widget elements (Text, Image, Button, etc.)
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGWidgetCommands
{
public:
    FSpirrowBridgeUMGWidgetCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Core
    TSharedPtr<FJsonObject> HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params);

    // Basic Widgets
    TSharedPtr<FJsonObject> HandleAddTextToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddTextBlockToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params);

    // Interactive Widgets
    TSharedPtr<FJsonObject> HandleAddButtonToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddButtonToWidgetV2(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddComboBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddEditableTextToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddSpinBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    // Helper
    FAnchors ParseAnchorPreset(const FString& AnchorStr);
};
