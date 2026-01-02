#include "Commands/SpirrowBridgeBlueprintNodeVariableCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"

FSpirrowBridgeBlueprintNodeVariableCommands::FSpirrowBridgeBlueprintNodeVariableCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("add_blueprint_variable"))
    {
        return HandleAddBlueprintVariable(Params);
    }
    else if (CommandType == TEXT("add_variable_get_node"))
    {
        return HandleAddVariableGetNode(Params);
    }
    else if (CommandType == TEXT("add_variable_set_node"))
    {
        return HandleAddVariableSetNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_get_self_component_reference"))
    {
        return HandleAddBlueprintGetSelfComponentReference(Params);
    }
    else if (CommandType == TEXT("add_blueprint_self_reference"))
    {
        return HandleAddBlueprintSelfReference(Params);
    }
    else if (CommandType == TEXT("add_blueprint_input_action_node"))
    {
        return HandleAddBlueprintInputActionNode(Params);
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, VariableName, VariableType;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("variable_name"), VariableName) ||
        !Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }

    bool IsExposed = false;
    Params->TryGetBoolField(TEXT("is_exposed"), IsExposed);

    FString Path = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("path"), Path);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FEdGraphPinType PinType;
    
    if (VariableType == TEXT("Boolean"))
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    else if (VariableType == TEXT("Integer") || VariableType == TEXT("Int"))
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    else if (VariableType == TEXT("Float"))
        PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
    else if (VariableType == TEXT("String"))
        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    else if (VariableType == TEXT("Vector"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
    }

    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar && IsExposed)
    {
        NewVar->PropertyFlags |= CPF_Edit;
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddVariableGetNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, VariableName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("variable_name"), VariableName))
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

    bool bVariableExists = false;
    for (const FBPVariableDescription& Var : Blueprint->NewVariables)
    {
        if (Var.VarName.ToString() == VariableName)
        {
            bVariableExists = true;
            break;
        }
    }

    if (!bVariableExists)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Variable not found in blueprint: %s"), *VariableName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create variable get node"));
    }

    GetNode->VariableReference.SetSelfMember(FName(*VariableName));
    GetNode->NodePosX = NodePosition.X;
    GetNode->NodePosY = NodePosition.Y;

    EventGraph->AddNode(GetNode);
    GetNode->CreateNewGuid();
    GetNode->PostPlacedNewNode();
    GetNode->AllocateDefaultPins();
    GetNode->ReconstructNode();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), GetNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddVariableSetNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, VariableName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("variable_name"), VariableName))
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

    bool bVariableExists = false;
    for (const FBPVariableDescription& Var : Blueprint->NewVariables)
    {
        if (Var.VarName.ToString() == VariableName)
        {
            bVariableExists = true;
            break;
        }
    }

    if (!bVariableExists)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Variable not found in blueprint: %s"), *VariableName));
    }

    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(EventGraph);
    if (!SetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create variable set node"));
    }

    SetNode->VariableReference.SetSelfMember(FName(*VariableName));
    SetNode->NodePosX = NodePosition.X;
    SetNode->NodePosY = NodePosition.Y;

    EventGraph->AddNode(SetNode);
    SetNode->CreateNewGuid();
    SetNode->PostPlacedNewNode();
    SetNode->AllocateDefaultPins();
    SetNode->ReconstructNode();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SetNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, ComponentName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("component_name"), ComponentName))
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

    UK2Node_VariableGet* GetComponentNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetComponentNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create get component node"));
    }

    GetComponentNode->VariableReference.SetSelfMember(FName(*ComponentName));
    GetComponentNode->NodePosX = NodePosition.X;
    GetComponentNode->NodePosY = NodePosition.Y;

    EventGraph->AddNode(GetComponentNode);
    GetComponentNode->CreateNewGuid();
    GetComponentNode->PostPlacedNewNode();
    GetComponentNode->AllocateDefaultPins();
    GetComponentNode->ReconstructNode();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), GetComponentNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
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

    UK2Node_Self* SelfNode = FSpirrowBridgeCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
    if (!SelfNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create self node"));
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, ActionName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) ||
        !Params->TryGetStringField(TEXT("action_name"), ActionName))
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

    UK2Node_InputAction* InputActionNode = FSpirrowBridgeCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
    if (!InputActionNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create input action node"));
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
    return ResultObj;
}
