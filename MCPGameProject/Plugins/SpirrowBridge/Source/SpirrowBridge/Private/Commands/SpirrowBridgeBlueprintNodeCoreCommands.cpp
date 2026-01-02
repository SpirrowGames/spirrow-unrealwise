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
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeId, TargetNodeId, SourcePinName, TargetPinName;
    if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId) ||
        !Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId) ||
        !Params->TryGetStringField(TEXT("source_pin"), SourcePinName) ||
        !Params->TryGetStringField(TEXT("target_pin"), TargetPinName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required node/pin parameters"));
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

    UEdGraphNode* SourceNode = nullptr;
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == SourceNodeId) SourceNode = Node;
        else if (Node->NodeGuid.ToString() == TargetNodeId) TargetNode = Node;
    }

    if (!SourceNode || !TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    if (FSpirrowBridgeCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
        return ResultObj;
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to connect nodes"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeType, EventType, FunctionNameFilter, VariableNameFilter;
    Params->TryGetStringField(TEXT("node_type"), NodeType);
    Params->TryGetStringField(TEXT("event_type"), EventType);
    Params->TryGetStringField(TEXT("function_name"), FunctionNameFilter);
    Params->TryGetStringField(TEXT("variable_name"), VariableNameFilter);

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
    FString BlueprintName, NodeId, PinName, PinValue;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("node_id"), NodeId) ||
        !Params->TryGetStringField(TEXT("pin_name"), PinName) ||
        !Params->TryGetStringField(TEXT("pin_value"), PinValue))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
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

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId) { TargetNode = Node; break; }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
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
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Pin not found: %s. Available: %s"), *PinName, *AvailablePins));
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
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetStringField(TEXT("pin_name"), PinName);
    ResultObj->SetStringField(TEXT("pin_value"), PinValue);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleDeleteNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, NodeId;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
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

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId) { TargetNode = Node; break; }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        Pin->BreakAllPinLinks();
    }
    EventGraph->RemoveNode(TargetNode);
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetBoolField(TEXT("deleted"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleMoveNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, NodeId;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("node_id"), NodeId) ||
        !Params->HasField(TEXT("position")))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }

    FVector2D NewPosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("position"));

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

    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId) { TargetNode = Node; break; }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    TargetNode->NodePosX = NewPosition.X;
    TargetNode->NodePosY = NewPosition.Y;
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetNumberField(TEXT("pos_x"), NewPosition.X);
    ResultObj->SetNumberField(TEXT("pos_y"), NewPosition.Y);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, EventName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
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

    UK2Node_Event* EventNode = FSpirrowBridgeCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
    if (!EventNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create event node"));
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, FunctionName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    FString Target;
    Params->TryGetStringField(TEXT("target"), Target);

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
    ResultObj->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
    return ResultObj;
}
