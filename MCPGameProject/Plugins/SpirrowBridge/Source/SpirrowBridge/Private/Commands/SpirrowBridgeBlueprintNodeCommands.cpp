#include "Commands/SpirrowBridgeBlueprintNodeCommands.h"
#include "Commands/SpirrowBridgeBlueprintNodeCoreCommands.h"
#include "Commands/SpirrowBridgeBlueprintNodeVariableCommands.h"
#include "Commands/SpirrowBridgeBlueprintNodeControlFlowCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

FSpirrowBridgeBlueprintNodeCommands::FSpirrowBridgeBlueprintNodeCommands()
{
    CoreCommands = MakeShared<FSpirrowBridgeBlueprintNodeCoreCommands>();
    VariableCommands = MakeShared<FSpirrowBridgeBlueprintNodeVariableCommands>();
    ControlFlowCommands = MakeShared<FSpirrowBridgeBlueprintNodeControlFlowCommands>();
}

FSpirrowBridgeBlueprintNodeCommands::~FSpirrowBridgeBlueprintNodeCommands()
{
    CoreCommands.Reset();
    VariableCommands.Reset();
    ControlFlowCommands.Reset();
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result;

    // Try CoreCommands first (connect, find, set_pin_value, delete, move, event, function)
    Result = CoreCommands->HandleCommand(CommandType, Params);
    if (Result.IsValid())
    {
        return Result;
    }

    // Try VariableCommands (variable, get/set nodes, self reference, input action)
    Result = VariableCommands->HandleCommand(CommandType, Params);
    if (Result.IsValid())
    {
        return Result;
    }

    // Try ControlFlowCommands (branch, sequence, delay, loop, print, math, comparison)
    Result = ControlFlowCommands->HandleCommand(CommandType, Params);
    if (Result.IsValid())
    {
        return Result;
    }

    // Unknown command
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint node command: %s"), *CommandType));
}
