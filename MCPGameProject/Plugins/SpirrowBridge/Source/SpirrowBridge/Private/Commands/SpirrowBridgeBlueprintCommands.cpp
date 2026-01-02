#include "Commands/SpirrowBridgeBlueprintCommands.h"
#include "Commands/SpirrowBridgeBlueprintCoreCommands.h"
#include "Commands/SpirrowBridgeBlueprintComponentCommands.h"
#include "Commands/SpirrowBridgeBlueprintPropertyCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

FSpirrowBridgeBlueprintCommands::FSpirrowBridgeBlueprintCommands()
{
    CoreCommands = MakeShared<FSpirrowBridgeBlueprintCoreCommands>();
    ComponentCommands = MakeShared<FSpirrowBridgeBlueprintComponentCommands>();
    PropertyCommands = MakeShared<FSpirrowBridgeBlueprintPropertyCommands>();
}

FSpirrowBridgeBlueprintCommands::~FSpirrowBridgeBlueprintCommands()
{
    CoreCommands.Reset();
    ComponentCommands.Reset();
    PropertyCommands.Reset();
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result;

    // Try CoreCommands first (create, compile, spawn, duplicate, get_graph, set_property)
    Result = CoreCommands->HandleCommand(CommandType, Params);
    if (Result.IsValid())
    {
        return Result;
    }

    // Try ComponentCommands (add_component, set_component_property, physics, mesh, pawn)
    Result = ComponentCommands->HandleCommand(CommandType, Params);
    if (Result.IsValid())
    {
        return Result;
    }

    // Try PropertyCommands (scan_classes, class_array, struct_array)
    Result = PropertyCommands->HandleCommand(CommandType, Params);
    if (Result.IsValid())
    {
        return Result;
    }

    // Unknown command
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint command: %s"), *CommandType));
}
