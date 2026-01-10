#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint property and project scanning commands
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintPropertyCommands
{
public:
    FSpirrowBridgeBlueprintPropertyCommands();

    // Handle blueprint property commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Property and scanning
    TSharedPtr<FJsonObject> HandleScanProjectClasses(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetBlueprintClassArray(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetStructArrayProperty(const TSharedPtr<FJsonObject>& Params);

    // New property commands (v0.8.8)
    TSharedPtr<FJsonObject> HandleCreateDataAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetClassProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetObjectProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBlueprintProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetStructProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetDataAssetProperty(const TSharedPtr<FJsonObject>& Params);

    // Helper function for property type names
    static FString GetPropertyTypeName(FProperty* Property);
};
