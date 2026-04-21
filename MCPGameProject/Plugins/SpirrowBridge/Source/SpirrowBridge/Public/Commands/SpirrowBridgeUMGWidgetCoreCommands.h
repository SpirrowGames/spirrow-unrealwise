#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "Widgets/Layout/Anchors.h"

class UWidgetTree;
class UPanelWidget;

/**
 * Handles UMG Widget core commands
 * Responsible for widget creation, viewport management, and utilities
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGWidgetCoreCommands
{
public:
    FSpirrowBridgeUMGWidgetCoreCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

    // Utility - shared with other UMG command handlers
    static FAnchors ParseAnchorPreset(const FString& AnchorStr);

    /**
     * Resolves the parent panel for an add_*_to_widget command.
     * If Params contains "parent_name" and it resolves to a UPanelWidget, returns that.
     * Otherwise returns the root CanvasPanel (creating one if missing).
     * Sets OutError with an error response (and returns nullptr) if parent_name is
     * provided but cannot be resolved.
     */
    static UPanelWidget* ResolveAddTarget(
        UWidgetTree* WidgetTree,
        const TSharedPtr<FJsonObject>& Params,
        TSharedPtr<FJsonObject>& OutError);

private:
    TSharedPtr<FJsonObject> HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params);
};
