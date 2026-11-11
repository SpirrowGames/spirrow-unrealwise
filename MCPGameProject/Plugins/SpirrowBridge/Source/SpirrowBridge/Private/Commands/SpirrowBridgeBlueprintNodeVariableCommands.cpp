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
#include "K2Node_GetSubsystem.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeContainer.h"
#include "K2Node_MakeMap.h"
#include "K2Node_MakeSet.h"
#include "Subsystems/EngineSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
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
    else if (CommandType == TEXT("add_external_property_set_node"))
    {
        return HandleAddExternalPropertySetNode(Params);
    }
    else if (CommandType == TEXT("add_external_property_get_node"))
    {
        return HandleAddExternalPropertyGetNode(Params);
    }
    else if (CommandType == TEXT("add_get_subsystem_node"))
    {
        return HandleAddGetSubsystemNode(Params);
    }
    else if (CommandType == TEXT("add_make_container_node"))
    {
        return HandleAddMakeContainerNode(Params);
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString VariableName, VariableType;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("variable_name"), VariableName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("variable_type"), VariableType))
    {
        return Error;
    }

    // Get optional parameters
    bool IsExposed;
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("is_exposed"), IsExposed, false);

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // Issue #23: the type used to be one of five hard-coded names. Now any
    // built-in type, struct, enum or class, optionally in Array<> / Set<> /
    // Map<,>. See FSpirrowBridgeCommonUtils::ResolvePinTypeFromString.
    FEdGraphPinType PinType;
    FString TypeError;
    if (!FSpirrowBridgeCommonUtils::ResolvePinTypeFromString(VariableType, PinType, TypeError))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParameter,
            FString::Printf(TEXT("Unsupported variable type: %s (%s)"), *VariableType, *TypeError));
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
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    ResultObj->SetStringField(TEXT("resolved_type"), FSpirrowBridgeCommonUtils::DescribePinType(PinType));
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddMakeContainerNode(const TSharedPtr<FJsonObject>& Params)
{
    FString Container;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("container"), Container))
    {
        return Error;
    }

    UClass* NodeClass = nullptr;
    const FString ContainerLower = Container.ToLower();
    if (ContainerLower == TEXT("array")) NodeClass = UK2Node_MakeArray::StaticClass();
    else if (ContainerLower == TEXT("set")) NodeClass = UK2Node_MakeSet::StaticClass();
    else if (ContainerLower == TEXT("map")) NodeClass = UK2Node_MakeMap::StaticClass();
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamValue,
            FString::Printf(TEXT("container must be 'array', 'set' or 'map' (got '%s')"), *Container));
    }
    const bool bIsMap = (NodeClass == UK2Node_MakeMap::StaticClass());

    int32 NumEntries = 1;
    double NumEntriesRaw = 0.0;
    if (Params->TryGetNumberField(TEXT("num_entries"), NumEntriesRaw))
    {
        NumEntries = FMath::RoundToInt32(NumEntriesRaw);
    }
    if (NumEntries < 1 || NumEntries > 64)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamValue,
            FString::Printf(TEXT("num_entries must be between 1 and 64 (got %d)"), NumEntries));
    }

    // Optional explicit type. Without it the pins are wildcards and take their
    // type from the first connection made through connect_blueprint_nodes.
    FString ElementTypeName, KeyTypeName, ValueTypeName;
    Params->TryGetStringField(TEXT("element_type"), ElementTypeName);
    Params->TryGetStringField(TEXT("key_type"), KeyTypeName);
    Params->TryGetStringField(TEXT("value_type"), ValueTypeName);

    bool bHasExplicitType = false;
    FEdGraphPinType ExplicitType;
    {
        FString TypeString;
        if (bIsMap)
        {
            if (KeyTypeName.IsEmpty() != ValueTypeName.IsEmpty())
            {
                return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                    ESpirrowErrorCode::InvalidParams,
                    TEXT("A map needs both key_type and value_type, or neither"));
            }
            if (!KeyTypeName.IsEmpty())
            {
                TypeString = FString::Printf(TEXT("Map<%s,%s>"), *KeyTypeName, *ValueTypeName);
            }
        }
        else if (!ElementTypeName.IsEmpty())
        {
            TypeString = FString::Printf(TEXT("%s<%s>"), ContainerLower == TEXT("array") ? TEXT("Array") : TEXT("Set"), *ElementTypeName);
        }

        if (!TypeString.IsEmpty())
        {
            FString TypeError;
            if (!FSpirrowBridgeCommonUtils::ResolvePinTypeFromString(TypeString, ExplicitType, TypeError))
            {
                return FSpirrowBridgeCommonUtils::CreateErrorResponse(ESpirrowErrorCode::InvalidParamValue, TypeError);
            }
            bHasExplicitType = true;
        }
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    UK2Node_MakeContainer* Node = NewObject<UK2Node_MakeContainer>(EventGraph, NodeClass);
    Node->NodePosX = NodePosition.X;
    Node->NodePosY = NodePosition.Y;
    EventGraph->AddNode(Node, true);
    Node->CreateNewGuid();
    Node->PostPlacedNewNode();
    Node->AllocateDefaultPins();
    for (int32 i = 1; i < NumEntries; ++i)
    {
        Node->AddInputPin();
    }

    if (bHasExplicitType)
    {
        if (UEdGraphPin* OutputPin = Node->GetOutputPin())
        {
            OutputPin->PinType = ExplicitType;

            // Same result as UK2Node_MakeContainer::PropagatePinType (which is
            // protected): key / element pins take the container's key type,
            // map value pins take its value type.
            TArray<UEdGraphPin*> KeyPins;
            TArray<UEdGraphPin*> ValuePins;
            Node->GetKeyAndValuePins(KeyPins, ValuePins);
            for (UEdGraphPin* KeyPin : KeyPins)
            {
                KeyPin->PinType.PinCategory = ExplicitType.PinCategory;
                KeyPin->PinType.PinSubCategory = ExplicitType.PinSubCategory;
                KeyPin->PinType.PinSubCategoryObject = ExplicitType.PinSubCategoryObject;
            }
            for (UEdGraphPin* ValuePin : ValuePins)
            {
                ValuePin->PinType.PinCategory = ExplicitType.PinValueType.TerminalCategory;
                ValuePin->PinType.PinSubCategory = ExplicitType.PinValueType.TerminalSubCategory;
                ValuePin->PinType.PinSubCategoryObject = ExplicitType.PinValueType.TerminalSubCategoryObject;
            }
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
    ResultObj->SetStringField(TEXT("container"), ContainerLower);
    ResultObj->SetBoolField(TEXT("typed"), bHasExplicitType);

    TArray<TSharedPtr<FJsonValue>> InputPins;
    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Input)
        {
            InputPins.Add(MakeShared<FJsonValueString>(Pin->PinName.ToString()));
        }
    }
    ResultObj->SetArrayField(TEXT("input_pins"), InputPins);
    if (const UEdGraphPin* OutputPin = Node->GetOutputPin())
    {
        ResultObj->SetStringField(TEXT("output_pin"), OutputPin->PinName.ToString());
        ResultObj->SetStringField(TEXT("output_type"), FSpirrowBridgeCommonUtils::DescribePinType(OutputPin->PinType));
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddVariableGetNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString VariableName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("variable_name"), VariableName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
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
            ESpirrowErrorCode::VariableNotFound,
            FString::Printf(TEXT("Variable not found in blueprint: %s"), *VariableName));
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            TEXT("Failed to create variable get node"));
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
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), GetNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddVariableSetNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString VariableName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("variable_name"), VariableName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
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
            ESpirrowErrorCode::VariableNotFound,
            FString::Printf(TEXT("Variable not found in blueprint: %s"), *VariableName));
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(EventGraph);
    if (!SetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            TEXT("Failed to create variable set node"));
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
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), SetNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ComponentName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("component_name"), ComponentName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    UK2Node_VariableGet* GetComponentNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetComponentNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            TEXT("Failed to create get component node"));
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
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), GetComponentNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
{
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    UK2Node_Self* SelfNode = FSpirrowBridgeCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
    if (!SelfNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            TEXT("Failed to create self node"));
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ActionName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("action_name"), ActionName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
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

    UK2Node_InputAction* InputActionNode = FSpirrowBridgeCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
    if (!InputActionNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            TEXT("Failed to create input action node"));
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
    return ResultObj;
}

// -----------------------------------------------------------------------------
// External property Set/Get nodes (UPROPERTY on another class, e.g. Subsystems)
// -----------------------------------------------------------------------------

static TSharedPtr<FJsonObject> BuildExternalPropertyNodeResponse(
    UEdGraphNode* Node,
    const FString& ClassName,
    const FString& PropertyName)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("target_class"), ClassName);
    ResultObj->SetStringField(TEXT("property_name"), PropertyName);

    TArray<TSharedPtr<FJsonValue>> InputPins;
    TArray<TSharedPtr<FJsonValue>> OutputPins;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin) continue;
        TSharedPtr<FJsonValue> PinName = MakeShared<FJsonValueString>(Pin->PinName.ToString());
        if (Pin->Direction == EGPD_Input) InputPins.Add(PinName);
        else OutputPins.Add(PinName);
    }
    ResultObj->SetArrayField(TEXT("input_pins"), InputPins);
    ResultObj->SetArrayField(TEXT("output_pins"), OutputPins);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddExternalPropertySetNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString TargetClassName, PropertyName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("target_class"), TargetClassName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    UClass* TargetClass = FSpirrowBridgeCommonUtils::FindClassByNameAnywhere(TargetClassName);
    if (!TargetClass)
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("target_class"), TargetClassName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ClassNotFound,
            FString::Printf(TEXT("Class not found: %s"), *TargetClassName),
            Details);
    }

    FString SpawnError;
    UK2Node_VariableSet* SetNode = FSpirrowBridgeCommonUtils::SpawnExternalPropertySetNode(
        EventGraph, TargetClass, FName(*PropertyName), NodePosition, SpawnError);
    if (!SetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            SpawnError);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    return BuildExternalPropertyNodeResponse(SetNode, TargetClass->GetName(), PropertyName);
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddExternalPropertyGetNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString TargetClassName, PropertyName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("target_class"), TargetClassName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
    {
        return Error;
    }

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    UClass* TargetClass = FSpirrowBridgeCommonUtils::FindClassByNameAnywhere(TargetClassName);
    if (!TargetClass)
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("target_class"), TargetClassName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ClassNotFound,
            FString::Printf(TEXT("Class not found: %s"), *TargetClassName),
            Details);
    }

    FString SpawnError;
    UK2Node_VariableGet* GetNode = FSpirrowBridgeCommonUtils::SpawnExternalPropertyGetNode(
        EventGraph, TargetClass, FName(*PropertyName), NodePosition, SpawnError);
    if (!GetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            SpawnError);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    return BuildExternalPropertyNodeResponse(GetNode, TargetClass->GetName(), PropertyName);
}

// -----------------------------------------------------------------------------
// Get Subsystem node (typed UK2Node_GetSubsystem variants)
// -----------------------------------------------------------------------------
//
// UE provides a dedicated K2 node for subsystem access that bakes the target
// class into the node at construction time. This avoids the Class-pin problem
// entirely (the Class pin does not exist on a typed node) and produces a
// strongly-typed ReturnValue pin so downstream Cast nodes are unnecessary.
//
// Kind -> concrete node class mapping:
//   "GameInstance" -> UK2Node_GetSubsystem       (base, default)
//   "World"        -> UK2Node_GetSubsystem       (base class; detects WorldSubsystem from CustomClass)
//   "Engine"       -> UK2Node_GetEngineSubsystem
//   "LocalPlayer"  -> UK2Node_GetSubsystemFromPC
//   "Editor"       -> UK2Node_GetEditorSubsystem (bonus; useful for editor tooling BPs)

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeVariableCommands::HandleAddGetSubsystemNode(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString SubsystemClassName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("subsystem_class"), SubsystemClassName))
    {
        return Error;
    }

    FString SubsystemKind;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("subsystem_kind"), SubsystemKind, TEXT("GameInstance"));

    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Resolve target Blueprint (regular BP or Level Blueprint via target_type)
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // graph_name selects a function graph (Issue #21); default is the event graph.
    UEdGraph* EventGraph = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetGraph(Params, Blueprint, EventGraph))
    {
        return Error;
    }

    // Resolve the subsystem class and validate it's a subsystem of the correct kind
    UClass* SubsystemClass = FSpirrowBridgeCommonUtils::FindClassByNameAnywhere(SubsystemClassName);
    if (!SubsystemClass)
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("subsystem_class"), SubsystemClassName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ClassNotFound,
            FString::Printf(TEXT("Class not found: %s"), *SubsystemClassName),
            Details);
    }

    UClass* ExpectedBase = nullptr;
    UClass* NodeClassToSpawn = nullptr;
    const FString NormalizedKind = SubsystemKind.ToLower();
    if (NormalizedKind == TEXT("gameinstance"))
    {
        ExpectedBase = UGameInstanceSubsystem::StaticClass();
        NodeClassToSpawn = UK2Node_GetSubsystem::StaticClass();
    }
    else if (NormalizedKind == TEXT("world"))
    {
        ExpectedBase = UWorldSubsystem::StaticClass();
        NodeClassToSpawn = UK2Node_GetSubsystem::StaticClass();
    }
    else if (NormalizedKind == TEXT("engine"))
    {
        ExpectedBase = UEngineSubsystem::StaticClass();
        NodeClassToSpawn = UK2Node_GetEngineSubsystem::StaticClass();
    }
    else if (NormalizedKind == TEXT("localplayer"))
    {
        ExpectedBase = ULocalPlayerSubsystem::StaticClass();
        NodeClassToSpawn = UK2Node_GetSubsystemFromPC::StaticClass();
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamValue,
            FString::Printf(TEXT("Unknown subsystem_kind: %s (expected Engine/GameInstance/World/LocalPlayer)"), *SubsystemKind));
    }

    if (!SubsystemClass->IsChildOf(ExpectedBase))
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("subsystem_class"), SubsystemClass->GetName());
        Details->SetStringField(TEXT("subsystem_kind"), SubsystemKind);
        Details->SetStringField(TEXT("expected_base"), ExpectedBase->GetName());
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertyTypeMismatch,
            FString::Printf(TEXT("Class %s is not a %s (subsystem_kind=%s expects a subclass of %s)"),
                *SubsystemClass->GetName(), *ExpectedBase->GetName(), *SubsystemKind, *ExpectedBase->GetName()),
            Details);
    }

    // Spawn the typed K2 node
    UK2Node_GetSubsystem* GetSubsystemNode = NewObject<UK2Node_GetSubsystem>(EventGraph, NodeClassToSpawn);
    if (!GetSubsystemNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            TEXT("Failed to instantiate K2Node_GetSubsystem"));
    }

    // Initialize MUST be called before AllocateDefaultPins so the Class pin
    // is omitted and the ReturnValue pin gets the typed output.
    GetSubsystemNode->Initialize(SubsystemClass);
    GetSubsystemNode->NodePosX = NodePosition.X;
    GetSubsystemNode->NodePosY = NodePosition.Y;
    EventGraph->AddNode(GetSubsystemNode, false, false);
    GetSubsystemNode->CreateNewGuid();
    GetSubsystemNode->PostPlacedNewNode();
    GetSubsystemNode->AllocateDefaultPins();

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), GetSubsystemNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("subsystem_class"), SubsystemClass->GetName());
    ResultObj->SetStringField(TEXT("subsystem_kind"), SubsystemKind);
    ResultObj->SetStringField(TEXT("node_class"), NodeClassToSpawn->GetName());

    TArray<TSharedPtr<FJsonValue>> InputPins;
    TArray<TSharedPtr<FJsonValue>> OutputPins;
    for (UEdGraphPin* Pin : GetSubsystemNode->Pins)
    {
        if (!Pin) continue;
        TSharedPtr<FJsonValue> PinName = MakeShared<FJsonValueString>(Pin->PinName.ToString());
        if (Pin->Direction == EGPD_Input) InputPins.Add(PinName);
        else OutputPins.Add(PinName);
    }
    ResultObj->SetArrayField(TEXT("input_pins"), InputPins);
    ResultObj->SetArrayField(TEXT("output_pins"), OutputPins);
    return ResultObj;
}
