#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint control flow and utility node commands
 */
class SPIRROWBRIDGE_API FSpirrowBridgeBlueprintNodeControlFlowCommands
{
public:
    FSpirrowBridgeBlueprintNodeControlFlowCommands();

    // Handle blueprint node control flow commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Control flow nodes
    TSharedPtr<FJsonObject> HandleAddBranchNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddSequenceNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddDelayNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddForEachLoopNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddForLoopWithBreakNode(const TSharedPtr<FJsonObject>& Params);
    
    // Utility nodes
    TSharedPtr<FJsonObject> HandleAddPrintStringNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddMathNode(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddComparisonNode(const TSharedPtr<FJsonObject>& Params);
};
