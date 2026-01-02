#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handles UMG Widget Animation operations
 * Responsible for creating and configuring widget animations
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGAnimationCommands
{
public:
    FSpirrowBridgeUMGAnimationCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateWidgetAnimation(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddAnimationKeyframe(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params);
};
