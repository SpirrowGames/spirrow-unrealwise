#include "Commands/SpirrowBridgeBlueprintNodeCoreCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_MacroInstance.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpirrowBridgeNodeCore, Log, All);

FSpirrowBridgeBlueprintNodeCoreCommands::FSpirrowBridgeBlueprintNodeCoreCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("connect_blueprint_nodes"))
    {
        return HandleConnectBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("disconnect_blueprint_nodes"))
    {
        return HandleDisconnectBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("find_blueprint_nodes"))
    {
        return HandleFindBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("set_node_pin_value"))
    {
        return HandleSetNodePinValue(Params);
    }
    else if (CommandType == TEXT("delete_node"))
    {
        return HandleDeleteNode(Params);
    }
    else if (CommandType == TEXT("move_node"))
    {
        return HandleMoveNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_event_node"))
    {
        return HandleAddBlueprintEvent(Params);
    }
    else if (CommandType == TEXT("add_blueprint_function_node"))
    {
        return HandleAddBlueprintFunctionCall(Params);
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, SourceNodeId, TargetNodeId, SourcePinName, TargetPinName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("source_node_id"), SourceNodeId))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("target_node_id"), TargetNodeId))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("source_pin"), SourcePinName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("target_pin"), TargetPinName))
    {
        return Error;
    }

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    UEdGraphNode* SourceNode = nullptr;
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == SourceNodeId) SourceNode = Node;
        else if (Node->NodeGuid.ToString() == TargetNodeId) TargetNode = Node;
    }

    if (!SourceNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeNotFound,
            FString::Printf(TEXT("Source node not found: %s"), *SourceNodeId));
    }
    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeNotFound,
            FString::Printf(TEXT("Target node not found: %s"), *TargetNodeId));
    }

    if (FSpirrowBridgeCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
        return ResultObj;
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::ConnectionFailed,
        TEXT("Failed to connect nodes"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleDisconnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters - at minimum need blueprint and one node
    FString BlueprintName, NodeId, PinName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("node_id"), NodeId))
    {
        return Error;
    }

    // Optional: specific pin to disconnect
    Params->TryGetStringField(TEXT("pin_name"), PinName);

    // Optional: target node/pin for breaking specific connection
    FString TargetNodeId, TargetPinName;
    Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId);
    Params->TryGetStringField(TEXT("target_pin"), TargetPinName);

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    // Find the source node
    UEdGraphNode* SourceNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId)
        {
            SourceNode = Node;
            break;
        }
    }

    if (!SourceNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeNotFound,
            FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    int32 DisconnectedCount = 0;
    TArray<FString> DisconnectedPins;

    // Case 1: Disconnect specific pin-to-pin connection
    if (!PinName.IsEmpty() && !TargetNodeId.IsEmpty() && !TargetPinName.IsEmpty())
    {
        UEdGraphNode* TargetNode = nullptr;
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (Node->NodeGuid.ToString() == TargetNodeId)
            {
                TargetNode = Node;
                break;
            }
        }

        if (!TargetNode)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::NodeNotFound,
                FString::Printf(TEXT("Target node not found: %s"), *TargetNodeId));
        }

        UEdGraphPin* SourcePin = FSpirrowBridgeCommonUtils::FindPin(SourceNode, PinName, EGPD_MAX);
        UEdGraphPin* TargetPin = FSpirrowBridgeCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_MAX);

        if (!SourcePin)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::PinNotFound,
                FString::Printf(TEXT("Source pin not found: %s"), *PinName));
        }
        if (!TargetPin)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::PinNotFound,
                FString::Printf(TEXT("Target pin not found: %s"), *TargetPinName));
        }

        if (SourcePin->LinkedTo.Contains(TargetPin))
        {
            SourcePin->BreakLinkTo(TargetPin);
            DisconnectedCount = 1;
            DisconnectedPins.Add(FString::Printf(TEXT("%s -> %s"), *PinName, *TargetPinName));
        }
    }
    // Case 2: Disconnect all connections from a specific pin
    else if (!PinName.IsEmpty())
    {
        UEdGraphPin* SourcePin = FSpirrowBridgeCommonUtils::FindPin(SourceNode, PinName, EGPD_MAX);
        if (!SourcePin)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::PinNotFound,
                FString::Printf(TEXT("Pin not found: %s"), *PinName));
        }

        DisconnectedCount = SourcePin->LinkedTo.Num();
        for (UEdGraphPin* LinkedPin : SourcePin->LinkedTo)
        {
            DisconnectedPins.Add(FString::Printf(TEXT("%s -> %s.%s"),
                *PinName,
                *LinkedPin->GetOwningNode()->GetName(),
                *LinkedPin->PinName.ToString()));
        }
        SourcePin->BreakAllPinLinks();
    }
    // Case 3: Disconnect all connections from the entire node
    else
    {
        for (UEdGraphPin* Pin : SourceNode->Pins)
        {
            if (Pin->LinkedTo.Num() > 0)
            {
                DisconnectedCount += Pin->LinkedTo.Num();
                DisconnectedPins.Add(FString::Printf(TEXT("%s (%d connections)"),
                    *Pin->PinName.ToString(), Pin->LinkedTo.Num()));
                Pin->BreakAllPinLinks();
            }
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetNumberField(TEXT("disconnected_count"), DisconnectedCount);

    TArray<TSharedPtr<FJsonValue>> DisconnectedArray;
    for (const FString& PinInfo : DisconnectedPins)
    {
        DisconnectedArray.Add(MakeShared<FJsonValueString>(PinInfo));
    }
    ResultObj->SetArrayField(TEXT("disconnected"), DisconnectedArray);

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }

    // Get optional parameters
    FString Path, NodeType, EventType, FunctionNameFilter, VariableNameFilter;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));
    Params->TryGetStringField(TEXT("node_type"), NodeType);
    Params->TryGetStringField(TEXT("event_type"), EventType);
    Params->TryGetStringField(TEXT("function_name"), FunctionNameFilter);
    Params->TryGetStringField(TEXT("variable_name"), VariableNameFilter);

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    TArray<TSharedPtr<FJsonValue>> NodesArray;

    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (!Node) continue;

        FString DetectedType = TEXT("Other");
        FString NodeName;

        if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
        {
            DetectedType = TEXT("Event");
            NodeName = EventNode->EventReference.GetMemberName().ToString();
            if (!EventType.IsEmpty() && NodeName != EventType) continue;
        }
        else if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
        {
            DetectedType = TEXT("Function");
            NodeName = FuncNode->FunctionReference.GetMemberName().ToString();
            if (!FunctionNameFilter.IsEmpty() && NodeName != FunctionNameFilter) continue;
        }
        else if (UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node))
        {
            DetectedType = TEXT("VariableGet");
            NodeName = VarGetNode->VariableReference.GetMemberName().ToString();
            if (!VariableNameFilter.IsEmpty() && NodeName != VariableNameFilter) continue;
        }
        else if (UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node))
        {
            DetectedType = TEXT("VariableSet");
            NodeName = VarSetNode->VariableReference.GetMemberName().ToString();
            if (!VariableNameFilter.IsEmpty() && NodeName != VariableNameFilter) continue;
        }
        else if (Cast<UK2Node_IfThenElse>(Node))
        {
            DetectedType = TEXT("Branch");
            NodeName = TEXT("Branch");
        }
        else if (Cast<UK2Node_ExecutionSequence>(Node))
        {
            DetectedType = TEXT("Sequence");
            NodeName = TEXT("Sequence");
        }
        else if (UK2Node_MacroInstance* MacroNode = Cast<UK2Node_MacroInstance>(Node))
        {
            DetectedType = TEXT("Macro");
            if (MacroNode->GetMacroGraph()) NodeName = MacroNode->GetMacroGraph()->GetName();
        }
        else if (UK2Node_InputAction* InputNode = Cast<UK2Node_InputAction>(Node))
        {
            DetectedType = TEXT("InputAction");
            NodeName = InputNode->InputActionName.ToString();
        }
        else if (Cast<UK2Node_Self>(Node))
        {
            DetectedType = TEXT("Self");
            NodeName = TEXT("Self");
        }

        if (!NodeType.IsEmpty() && NodeType != TEXT("All"))
        {
            if (NodeType == TEXT("Variable"))
            {
                if (DetectedType != TEXT("VariableGet") && DetectedType != TEXT("VariableSet")) continue;
            }
            else if (DetectedType != NodeType) continue;
        }

        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
        NodeObj->SetStringField(TEXT("node_type"), DetectedType);
        NodeObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
        NodeObj->SetStringField(TEXT("name"), NodeName);

        TSharedPtr<FJsonObject> PosObj = MakeShared<FJsonObject>();
        PosObj->SetNumberField(TEXT("x"), Node->NodePosX);
        PosObj->SetNumberField(TEXT("y"), Node->NodePosY);
        NodeObj->SetObjectField(TEXT("position"), PosObj);

        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin) continue;
            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
            PinObj->SetBoolField(TEXT("connected"), Pin->LinkedTo.Num() > 0);
            PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
        }
        NodeObj->SetArrayField(TEXT("pins"), PinsArray);
        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
    ResultObj->SetNumberField(TEXT("count"), NodesArray.Num());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleSetNodePinValue(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, NodeId, PinName, PinValue;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("node_id"), NodeId))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("pin_name"), PinName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("pin_value"), PinValue))
    {
        return Error;
    }

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId) { TargetNode = Node; break; }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeNotFound,
            FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    UEdGraphPin* TargetPin = FSpirrowBridgeCommonUtils::FindPin(TargetNode, PinName, EGPD_Input);
    if (!TargetPin) TargetPin = FSpirrowBridgeCommonUtils::FindPin(TargetNode, PinName, EGPD_Output);

    if (!TargetPin)
    {
        FString AvailablePins;
        for (UEdGraphPin* Pin : TargetNode->Pins)
        {
            AvailablePins += FString::Printf(TEXT("%s (%s), "), *Pin->PinName.ToString(), 
                Pin->Direction == EGPD_Input ? TEXT("In") : TEXT("Out"));
        }
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("available_pins"), AvailablePins);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PinNotFound,
            FString::Printf(TEXT("Pin not found: %s"), *PinName),
            Details);
    }

    if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
        TargetPin->DefaultValue = FString::FromInt(FCString::Atoi(*PinValue));
    else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float || 
             TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
        TargetPin->DefaultValue = FString::SanitizeFloat(FCString::Atof(*PinValue));
    else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
        TargetPin->DefaultValue = PinValue.ToBool() ? TEXT("true") : TEXT("false");
    else
        TargetPin->DefaultValue = PinValue;

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetStringField(TEXT("pin_name"), PinName);
    ResultObj->SetStringField(TEXT("pin_value"), PinValue);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleDeleteNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, NodeId;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("node_id"), NodeId))
    {
        return Error;
    }

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId) { TargetNode = Node; break; }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeNotFound,
            FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        Pin->BreakAllPinLinks();
    }
    EventGraph->RemoveNode(TargetNode);
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetBoolField(TEXT("deleted"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleMoveNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, NodeId;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("node_id"), NodeId))
    {
        return Error;
    }
    if (!Params->HasField(TEXT("position")))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing 'position' parameter"));
    }

    FVector2D NewPosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("position"));

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId) { TargetNode = Node; break; }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeNotFound,
            FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    TargetNode->NodePosX = NewPosition.X;
    TargetNode->NodePosY = NewPosition.Y;
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetNumberField(TEXT("pos_x"), NewPosition.X);
    ResultObj->SetNumberField(TEXT("pos_y"), NewPosition.Y);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, EventName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("event_name"), EventName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    UK2Node_Event* EventNode = nullptr;
    bool bIsOverride = false;

    // ★ Check if event is a BlueprintImplementableEvent in parent class ★
    UClass* ParentClass = Blueprint->ParentClass;
    UFunction* ParentFunction = nullptr;

    while (ParentClass)
    {
        ParentFunction = ParentClass->FindFunctionByName(*EventName);
        if (ParentFunction && ParentFunction->HasAnyFunctionFlags(FUNC_BlueprintEvent))
        {
            break;
        }
        ParentFunction = nullptr;
        ParentClass = ParentClass->GetSuperClass();
    }

    if (ParentFunction)
    {
        // ★ This is a BlueprintImplementableEvent override ★
        bIsOverride = true;

        // Check if already overridden
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (UK2Node_Event* ExistingEvent = Cast<UK2Node_Event>(Node))
            {
                if (ExistingEvent->bOverrideFunction &&
                    ExistingEvent->EventReference.GetMemberName() == ParentFunction->GetFName())
                {
                    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                        ESpirrowErrorCode::NodeAlreadyExists,
                        FString::Printf(TEXT("Event '%s' is already overridden"), *EventName));
                }
            }
        }

        // Create override event node
        EventNode = NewObject<UK2Node_Event>(EventGraph);
        EventGraph->AddNode(EventNode, false, false);

        EventNode->EventReference.SetFromField<UFunction>(ParentFunction, false);
        EventNode->bOverrideFunction = true;
        EventNode->NodePosX = NodePosition.X;
        EventNode->NodePosY = NodePosition.Y;
        EventNode->CreateNewGuid();
        EventNode->AllocateDefaultPins();
        EventNode->PostPlacedNewNode();

        UE_LOG(LogSpirrowBridgeNodeCore, Log, TEXT("Created override event node for BlueprintImplementableEvent: %s"), *EventName);
    }
    else
    {
        // ★ Custom event (not an override) ★
        EventNode = FSpirrowBridgeCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
    }

    if (!EventNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            FString::Printf(TEXT("Failed to create event node: %s"), *EventName));
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
    ResultObj->SetBoolField(TEXT("is_override"), bIsOverride);
    if (bIsOverride && ParentClass)
    {
        ResultObj->SetStringField(TEXT("parent_class"), ParentClass->GetName());
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, FunctionName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("function_name"), FunctionName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get optional parameters
    FString Target, Path;
    Params->TryGetStringField(TEXT("target"), Target);
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Validate and load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Failed to get event graph"));
    }

    UFunction* Function = nullptr;
    UK2Node_CallFunction* FunctionNode = nullptr;
    
    if (!Target.IsEmpty())
    {
        UClass* TargetClass = FindObject<UClass>(nullptr, *Target);
        if (!TargetClass && !Target.StartsWith(TEXT("U")))
        {
            TargetClass = FindObject<UClass>(nullptr, *(TEXT("U") + Target));
        }
        if (!TargetClass)
        {
            TargetClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + Target));
        }
        
        if (TargetClass)
        {
            Function = TargetClass->FindFunctionByName(*FunctionName);
            UClass* CurrentClass = TargetClass;
            while (!Function && CurrentClass)
            {
                Function = CurrentClass->FindFunctionByName(*FunctionName);
                if (!Function)
                {
                    for (TFieldIterator<UFunction> FuncIt(CurrentClass); FuncIt; ++FuncIt)
                    {
                        if ((*FuncIt)->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
                        {
                            Function = *FuncIt;
                            break;
                        }
                    }
                }
                CurrentClass = CurrentClass->GetSuperClass();
            }
        }
    }
    
    if (!Function && !FunctionNode)
    {
        Function = Blueprint->GeneratedClass->FindFunctionByName(*FunctionName);
    }
    
    if (Function && !FunctionNode)
    {
        FunctionNode = FSpirrowBridgeCommonUtils::CreateFunctionCallNode(EventGraph, Function, NodePosition);
    }
    
    if (!FunctionNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::FunctionNotFound,
            FString::Printf(TEXT("Function not found: %s in target %s"), *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target));
    }

    // Set parameters if provided
    if (Params->HasField(TEXT("params")))
    {
        const TSharedPtr<FJsonObject>* ParamsObj;
        if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
        {
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
            {
                UEdGraphPin* ParamPin = FSpirrowBridgeCommonUtils::FindPin(FunctionNode, Param.Key, EGPD_Input);
                if (ParamPin)
                {
                    const TSharedPtr<FJsonValue>& ParamValue = Param.Value;
                    if (ParamValue->Type == EJson::String)
                    {
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
                        {
                            FString ClassName = ParamValue->AsString();
                            UClass* Class = LoadObject<UClass>(nullptr, *ClassName);
                            if (!Class) Class = LoadObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + ClassName));
                            if (Class)
                            {
                                const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
                                if (K2Schema) K2Schema->TrySetDefaultObject(*ParamPin, Class);
                            }
                        }
                        else
                        {
                            ParamPin->DefaultValue = ParamValue->AsString();
                        }
                    }
                    else if (ParamValue->Type == EJson::Number)
                    {
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                            ParamPin->DefaultValue = FString::FromInt(FMath::RoundToInt(ParamValue->AsNumber()));
                        else
                            ParamPin->DefaultValue = FString::SanitizeFloat(ParamValue->AsNumber());
                    }
                    else if (ParamValue->Type == EJson::Boolean)
                    {
                        ParamPin->DefaultValue = ParamValue->AsBool() ? TEXT("true") : TEXT("false");
                    }
                    else if (ParamValue->Type == EJson::Array)
                    {
                        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                        if (ParamValue->TryGetArray(ArrayValue) && ArrayValue->Num() == 3 &&
                            ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
                            ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
                        {
                            float X = (*ArrayValue)[0]->AsNumber();
                            float Y = (*ArrayValue)[1]->AsNumber();
                            float Z = (*ArrayValue)[2]->AsNumber();
                            ParamPin->DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                        }
                    }
                }
            }
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
    return ResultObj;
}
