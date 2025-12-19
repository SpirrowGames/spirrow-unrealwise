#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handler class for Material-related MCP commands
 */
class SPIRROWBRIDGE_API FSpirrowBridgeMaterialCommands
{
public:
    FSpirrowBridgeMaterialCommands();

    /**
     * Handle material commands
     * @param CommandType - The type of command to handle
     * @param Params - JSON parameters for the command
     * @return JSON response with results or error
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    /**
     * Create a simple material with specified properties
     * @param Params - Must include:
     *                "name" - Material name
     *                "path" - Content browser path
     *                "shading_model" - DefaultLit or Unlit
     *                "blend_mode" - Opaque, Translucent, or Masked
     *                "two_sided" - Whether to render both sides
     *                "base_color" - [R, G, B] base color (optional)
     *                "opacity" - Opacity value for Translucent (optional)
     *                "emissive_color" - [R, G, B] emissive color (optional)
     *                "emissive_strength" - Emissive multiplier (optional)
     * @return JSON response with created material details
     */
    TSharedPtr<FJsonObject> HandleCreateSimpleMaterial(const TSharedPtr<FJsonObject>& Params);
};
