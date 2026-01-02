#include "Commands/SpirrowBridgeBlueprintNodeControlFlowCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_MacroInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"

FSpirrowBridgeBlueprintNodeControlFlowCommands::FSpirrowBridgeBlueprintNodeControlFlowCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("add_branch_node"))
    {
        return HandleAddBranchNode(Params);
    }
    else if (CommandType == TEXT("add_sequence_node"))
    {
        return HandleAddSequenceNode(Params);
    }
    else if (CommandType == TEXT("add_delay_node"))
    {
        return HandleAddDelayNode(Params);
    }
    else if (CommandType == TEXT("add_foreach_loop_node"))
    {
        return HandleAddForEachLoopNode(Params);
    }
    else if (CommandType == TEXT("add_forloop_with_break_node"))
    {
        return HandleAddForLoopWithBreakNode(Params);
    }
    else if (CommandType == TEXT("add_print_string_node"))
    {
        return HandleAddPrintStringNode(Params);
    }
    else if (CommandType == TEXT("add_math_node"))
    {
        return HandleAddMathNode(Params);
    }
    else if (CommandType == TEXT("add_comparison_node"))
    {
        return HandleAddComparisonNode(Params);
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddBranchNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(EventGraph);
    if (!BranchNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create branch node"));
    }

    BranchNode->NodePosX = NodePosition.X;
    BranchNode->NodePosY = NodePosition.Y;

    EventGraph->AddNode(BranchNode);
    BranchNode->CreateNewGuid();
    BranchNode->PostPlacedNewNode();
    BranchNode->AllocateDefaultPins();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), BranchNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddSequenceNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    int32 NumOutputs = 2;
    if (Params->HasField(TEXT("num_outputs")))
    {
        NumOutputs = Params->GetIntegerField(TEXT("num_outputs"));
        NumOutputs = FMath::Clamp(NumOutputs, 2, 10);
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    UK2Node_ExecutionSequence* SequenceNode = NewObject<UK2Node_ExecutionSequence>(EventGraph);
    if (!SequenceNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create sequence node"));
    }

    SequenceNode->NodePosX = NodePosition.X;
    SequenceNode->NodePosY = NodePosition.Y;

    EventGraph->AddNode(SequenceNode);
    SequenceNode->CreateNewGuid();
    SequenceNode->PostPlacedNewNode();
    SequenceNode->AllocateDefaultPins();

    for (int32 i = 2; i < NumOutputs; ++i)
    {
        SequenceNode->AddInputPin();
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SequenceNode->NodeGuid.ToString());
    ResultObj->SetNumberField(TEXT("num_outputs"), NumOutputs);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddDelayNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    float Duration = 1.0f;
    if (Params->HasField(TEXT("duration")))
    {
        Duration = Params->GetNumberField(TEXT("duration"));
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    UFunction* DelayFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("Delay"));
    if (!DelayFunction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find Delay function"));
    }

    UK2Node_CallFunction* DelayNode = FSpirrowBridgeCommonUtils::CreateFunctionCallNode(EventGraph, DelayFunction, NodePosition);
    if (!DelayNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create delay node"));
    }

    UEdGraphPin* DurationPin = FSpirrowBridgeCommonUtils::FindPin(DelayNode, TEXT("Duration"), EGPD_Input);
    if (DurationPin)
    {
        DurationPin->DefaultValue = FString::SanitizeFloat(Duration);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), DelayNode->NodeGuid.ToString());
    ResultObj->SetNumberField(TEXT("duration"), Duration);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddForEachLoopNode(const TSharedPtr<FJsonObject>& Params)
{
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("ForEach loop node is not yet supported. Use add_forloop_with_break_node instead."));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddForLoopWithBreakNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    int32 FirstIndex = 0;
    int32 LastIndex = 10;
    if (Params->HasField(TEXT("first_index")))
    {
        FirstIndex = static_cast<int32>(Params->GetNumberField(TEXT("first_index")));
    }
    if (Params->HasField(TEXT("last_index")))
    {
        LastIndex = static_cast<int32>(Params->GetNumberField(TEXT("last_index")));
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    UBlueprint* MacroLibrary = LoadObject<UBlueprint>(nullptr,
        TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
    if (!MacroLibrary)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to load StandardMacros library"));
    }

    UEdGraph* MacroGraph = nullptr;
    for (UEdGraph* Graph : MacroLibrary->MacroGraphs)
    {
        if (Graph && Graph->GetFName() == FName(TEXT("ForLoopWithBreak")))
        {
            MacroGraph = Graph;
            break;
        }
    }

    if (!MacroGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find ForLoopWithBreak macro"));
    }

    UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(EventGraph);
    MacroNode->SetMacroGraph(MacroGraph);
    MacroNode->NodePosX = NodePosition.X;
    MacroNode->NodePosY = NodePosition.Y;
    EventGraph->AddNode(MacroNode, false, false);
    MacroNode->CreateNewGuid();
    MacroNode->PostPlacedNewNode();
    MacroNode->AllocateDefaultPins();

    UEdGraphPin* FirstIndexPin = MacroNode->FindPin(TEXT("FirstIndex"));
    if (FirstIndexPin)
    {
        FirstIndexPin->DefaultValue = FString::FromInt(FirstIndex);
    }

    UEdGraphPin* LastIndexPin = MacroNode->FindPin(TEXT("LastIndex"));
    if (LastIndexPin)
    {
        LastIndexPin->DefaultValue = FString::FromInt(LastIndex);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetStringField(TEXT("node_id"), MacroNode->NodeGuid.ToString());
    ResultJson->SetNumberField(TEXT("first_index"), FirstIndex);
    ResultJson->SetNumberField(TEXT("last_index"), LastIndex);

    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(ResultJson);
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddPrintStringNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString Message = TEXT("Hello");
    Params->TryGetStringField(TEXT("message"), Message);

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    UFunction* PrintStringFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString"));
    if (!PrintStringFunction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find PrintString function"));
    }

    UK2Node_CallFunction* PrintStringNode = FSpirrowBridgeCommonUtils::CreateFunctionCallNode(EventGraph, PrintStringFunction, NodePosition);
    if (!PrintStringNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create PrintString node"));
    }

    UEdGraphPin* InStringPin = FSpirrowBridgeCommonUtils::FindPin(PrintStringNode, TEXT("InString"), EGPD_Input);
    if (InStringPin)
    {
        InStringPin->DefaultValue = Message;
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), PrintStringNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("message"), Message);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddMathNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, Operation;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("operation"), Operation))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }

    FString ValueType = TEXT("Float");
    Params->TryGetStringField(TEXT("value_type"), ValueType);

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    FString FunctionName;
    if (ValueType == TEXT("Float"))
    {
        if (Operation == TEXT("Add")) FunctionName = TEXT("Add_DoubleDouble");
        else if (Operation == TEXT("Subtract")) FunctionName = TEXT("Subtract_DoubleDouble");
        else if (Operation == TEXT("Multiply")) FunctionName = TEXT("Multiply_DoubleDouble");
        else if (Operation == TEXT("Divide")) FunctionName = TEXT("Divide_DoubleDouble");
    }
    else if (ValueType == TEXT("Int"))
    {
        if (Operation == TEXT("Add")) FunctionName = TEXT("Add_IntInt");
        else if (Operation == TEXT("Subtract")) FunctionName = TEXT("Subtract_IntInt");
        else if (Operation == TEXT("Multiply")) FunctionName = TEXT("Multiply_IntInt");
        else if (Operation == TEXT("Divide")) FunctionName = TEXT("Divide_IntInt");
    }

    if (FunctionName.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported operation/type: %s/%s"), *Operation, *ValueType));
    }

    UFunction* MathFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(*FunctionName);
    if (!MathFunction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to find math function: %s"), *FunctionName));
    }

    UK2Node_CallFunction* MathNode = NewObject<UK2Node_CallFunction>(EventGraph);
    MathNode->FunctionReference.SetExternalMember(MathFunction->GetFName(), UKismetMathLibrary::StaticClass());
    MathNode->NodePosX = NodePosition.X;
    MathNode->NodePosY = NodePosition.Y;
    EventGraph->AddNode(MathNode, false, false);
    MathNode->CreateNewGuid();
    MathNode->PostPlacedNewNode();
    MathNode->AllocateDefaultPins();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetStringField(TEXT("node_id"), MathNode->NodeGuid.ToString());
    ResultJson->SetStringField(TEXT("operation"), Operation);
    ResultJson->SetStringField(TEXT("value_type"), ValueType);

    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(ResultJson);
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeControlFlowCommands::HandleAddComparisonNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, Operation;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("operation"), Operation))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }

    FString ValueType = TEXT("Float");
    Params->TryGetStringField(TEXT("value_type"), ValueType);

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    FString FunctionName;
    if (ValueType == TEXT("Float"))
    {
        if (Operation == TEXT("Greater")) FunctionName = TEXT("Greater_DoubleDouble");
        else if (Operation == TEXT("GreaterEqual")) FunctionName = TEXT("GreaterEqual_DoubleDouble");
        else if (Operation == TEXT("Less")) FunctionName = TEXT("Less_DoubleDouble");
        else if (Operation == TEXT("LessEqual")) FunctionName = TEXT("LessEqual_DoubleDouble");
        else if (Operation == TEXT("Equal")) FunctionName = TEXT("EqualEqual_DoubleDouble");
        else if (Operation == TEXT("NotEqual")) FunctionName = TEXT("NotEqual_DoubleDouble");
    }
    else if (ValueType == TEXT("Int"))
    {
        if (Operation == TEXT("Greater")) FunctionName = TEXT("Greater_IntInt");
        else if (Operation == TEXT("GreaterEqual")) FunctionName = TEXT("GreaterEqual_IntInt");
        else if (Operation == TEXT("Less")) FunctionName = TEXT("Less_IntInt");
        else if (Operation == TEXT("LessEqual")) FunctionName = TEXT("LessEqual_IntInt");
        else if (Operation == TEXT("Equal")) FunctionName = TEXT("EqualEqual_IntInt");
        else if (Operation == TEXT("NotEqual")) FunctionName = TEXT("NotEqual_IntInt");
    }

    if (FunctionName.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported comparison/type: %s/%s"), *Operation, *ValueType));
    }

    UFunction* ComparisonFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(*FunctionName);
    if (!ComparisonFunction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to find comparison function: %s"), *FunctionName));
    }

    UK2Node_CallFunction* CompareNode = NewObject<UK2Node_CallFunction>(EventGraph);
    CompareNode->FunctionReference.SetExternalMember(ComparisonFunction->GetFName(), UKismetMathLibrary::StaticClass());
    CompareNode->NodePosX = NodePosition.X;
    CompareNode->NodePosY = NodePosition.Y;
    EventGraph->AddNode(CompareNode, false, false);
    CompareNode->CreateNewGuid();
    CompareNode->PostPlacedNewNode();
    CompareNode->AllocateDefaultPins();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetStringField(TEXT("node_id"), CompareNode->NodeGuid.ToString());
    ResultJson->SetStringField(TEXT("operation"), Operation);
    ResultJson->SetStringField(TEXT("value_type"), ValueType);

    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(ResultJson);
}
