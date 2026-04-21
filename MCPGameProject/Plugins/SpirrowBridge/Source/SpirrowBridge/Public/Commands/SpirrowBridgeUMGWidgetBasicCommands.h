#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "Widgets/Layout/Anchors.h"

/**
 * Handles UMG basic widget commands
 * Responsible for Text, Image, ProgressBar
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGWidgetBasicCommands
{
public:
    FSpirrowBridgeUMGWidgetBasicCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleAddTextToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddTextBlockToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddBorderToWidget(const TSharedPtr<FJsonObject>& Params);
};
