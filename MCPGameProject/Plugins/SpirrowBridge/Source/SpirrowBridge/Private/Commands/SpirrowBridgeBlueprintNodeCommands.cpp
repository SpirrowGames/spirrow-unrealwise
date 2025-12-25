#include "Commands/SpirrowBridgeBlueprintNodeCommands.h"
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
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GameFramework/InputSettings.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "EdGraphSchema_K2.h"

// Declare the log category
DEFINE_LOG_CATEGORY_STATIC(LogSpirrowBridge, Log, All);

FSpirrowBridgeBlueprintNodeCommands::FSpirrowBridgeBlueprintNodeCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("connect_blueprint_nodes"))
    {
        return HandleConnectBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("add_blueprint_get_self_component_reference"))
    {
        return HandleAddBlueprintGetSelfComponentReference(Params);
    }
    else if (CommandType == TEXT("add_blueprint_event_node"))
    {
        return HandleAddBlueprintEvent(Params);
    }
    else if (CommandType == TEXT("add_blueprint_function_node"))
    {
        return HandleAddBlueprintFunctionCall(Params);
    }
    else if (CommandType == TEXT("add_blueprint_variable"))
    {
        return HandleAddBlueprintVariable(Params);
    }
    else if (CommandType == TEXT("add_blueprint_input_action_node"))
    {
        return HandleAddBlueprintInputActionNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_self_reference"))
    {
        return HandleAddBlueprintSelfReference(Params);
    }
    else if (CommandType == TEXT("find_blueprint_nodes"))
    {
        return HandleFindBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("set_node_pin_value"))
    {
        return HandleSetNodePinValue(Params);
    }
    else if (CommandType == TEXT("add_variable_get_node"))
    {
        return HandleAddVariableGetNode(Params);
    }
    else if (CommandType == TEXT("add_variable_set_node"))
    {
        return HandleAddVariableSetNode(Params);
    }
    else if (CommandType == TEXT("add_branch_node"))
    {
        return HandleAddBranchNode(Params);
    }
    else if (CommandType == TEXT("delete_node"))
    {
        return HandleDeleteNode(Params);
    }
    else if (CommandType == TEXT("move_node"))
    {
        return HandleMoveNode(Params);
    }
    // Control flow nodes
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
    // Debug & utility nodes
    else if (CommandType == TEXT("add_print_string_node"))
    {
        return HandleAddPrintStringNode(Params);
    }
    // Math & comparison nodes
    else if (CommandType == TEXT("add_math_node"))
    {
        return HandleAddMathNode(Params);
    }
    else if (CommandType == TEXT("add_comparison_node"))
    {
        return HandleAddComparisonNode(Params);
    }
    
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint node command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeId;
    if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
    }

    FString TargetNodeId;
    if (!Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
    }

    FString SourcePinName;
    if (!Params->TryGetStringField(TEXT("source_pin"), SourcePinName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin' parameter"));
    }

    FString TargetPinName;
    if (!Params->TryGetStringField(TEXT("target_pin"), TargetPinName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin' parameter"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the nodes
    UEdGraphNode* SourceNode = nullptr;
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == SourceNodeId)
        {
            SourceNode = Node;
        }
        else if (Node->NodeGuid.ToString() == TargetNodeId)
        {
            TargetNode = Node;
        }
    }

    if (!SourceNode || !TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    // Connect the nodes
    if (FSpirrowBridgeCommonUtils::ConnectGraphNodes(EventGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
    {
        // Mark the blueprint as modified
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
        return ResultObj;
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to connect nodes"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }
    
    // We'll skip component verification since the GetAllNodes API may have changed in UE5.5
    
    // Create the variable get node directly
    UK2Node_VariableGet* GetComponentNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetComponentNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create get component node"));
    }
    
    // Set up the variable reference properly for UE5.5
    FMemberReference& VarRef = GetComponentNode->VariableReference;
    VarRef.SetSelfMember(FName(*ComponentName));
    
    // Set node position
    GetComponentNode->NodePosX = NodePosition.X;
    GetComponentNode->NodePosY = NodePosition.Y;
    
    // Add to graph
    EventGraph->AddNode(GetComponentNode);
    GetComponentNode->CreateNewGuid();
    GetComponentNode->PostPlacedNewNode();
    GetComponentNode->AllocateDefaultPins();
    
    // Explicitly reconstruct node for UE5.5
    GetComponentNode->ReconstructNode();
    
    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), GetComponentNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the event node
    UK2Node_Event* EventNode = FSpirrowBridgeCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
    if (!EventNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create event node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Check for target parameter (optional)
    FString Target;
    Params->TryGetStringField(TEXT("target"), Target);

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the function
    UFunction* Function = nullptr;
    UK2Node_CallFunction* FunctionNode = nullptr;
    
    // Add extensive logging for debugging
    UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in target '%s'"), 
           *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target);
    
    // Check if we have a target class specified
    if (!Target.IsEmpty())
    {
        // Try to find the target class
        UClass* TargetClass = nullptr;

        // First try without a prefix
        TargetClass = FindObject<UClass>(nullptr, *Target);
        UE_LOG(LogTemp, Display, TEXT("Tried to find class '%s': %s"),
               *Target, TargetClass ? TEXT("Found") : TEXT("Not found"));

        // If not found, try with U prefix (common convention for UE classes)
        if (!TargetClass && !Target.StartsWith(TEXT("U")))
        {
            FString TargetWithPrefix = FString(TEXT("U")) + Target;
            TargetClass = FindObject<UClass>(nullptr, *TargetWithPrefix);
            UE_LOG(LogTemp, Display, TEXT("Tried to find class '%s': %s"), 
                   *TargetWithPrefix, TargetClass ? TEXT("Found") : TEXT("Not found"));
        }
        
        // If still not found, try with common component names
        if (!TargetClass)
        {
            // Try some common component class names
            TArray<FString> PossibleClassNames;
            PossibleClassNames.Add(FString(TEXT("U")) + Target + TEXT("Component"));
            PossibleClassNames.Add(Target + TEXT("Component"));
            
            for (const FString& ClassName : PossibleClassNames)
            {
                TargetClass = FindObject<UClass>(nullptr, *ClassName);
                if (TargetClass)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found class using alternative name '%s'"), *ClassName);
                    break;
                }
            }
        }
        
        // Special case handling for common classes like UGameplayStatics
        if (!TargetClass && Target == TEXT("UGameplayStatics"))
        {
            // For UGameplayStatics, use a direct reference to known class
            TargetClass = FindObject<UClass>(nullptr, TEXT("UGameplayStatics"));
            if (!TargetClass)
            {
                // Try loading it from its known package
                TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.GameplayStatics"));
                UE_LOG(LogTemp, Display, TEXT("Explicitly loading GameplayStatics: %s"), 
                       TargetClass ? TEXT("Success") : TEXT("Failed"));
            }
        }
        
        // If we found a target class, look for the function there
        if (TargetClass)
        {
            UE_LOG(LogTemp, Display, TEXT("Looking for function '%s' in class '%s'"), 
                   *FunctionName, *TargetClass->GetName());
                   
            // First try exact name
            Function = TargetClass->FindFunctionByName(*FunctionName);
            
            // If not found, try class hierarchy
            UClass* CurrentClass = TargetClass;
            while (!Function && CurrentClass)
            {
                UE_LOG(LogTemp, Display, TEXT("Searching in class: %s"), *CurrentClass->GetName());
                
                // Try exact match
                Function = CurrentClass->FindFunctionByName(*FunctionName);
                
                // Try case-insensitive match
                if (!Function)
                {
                    for (TFieldIterator<UFunction> FuncIt(CurrentClass); FuncIt; ++FuncIt)
                    {
                        UFunction* AvailableFunc = *FuncIt;
                        UE_LOG(LogTemp, Display, TEXT("  - Available function: %s"), *AvailableFunc->GetName());
                        
                        if (AvailableFunc->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive match: %s"), *AvailableFunc->GetName());
                            Function = AvailableFunc;
                            break;
                        }
                    }
                }
                
                // Move to parent class
                CurrentClass = CurrentClass->GetSuperClass();
            }
            
            // Special handling for known functions
            if (!Function)
            {
                if (TargetClass->GetName() == TEXT("GameplayStatics") && 
                    (FunctionName == TEXT("GetActorOfClass") || FunctionName.Equals(TEXT("GetActorOfClass"), ESearchCase::IgnoreCase)))
                {
                    UE_LOG(LogTemp, Display, TEXT("Using special case handling for GameplayStatics::GetActorOfClass"));
                    
                    // Create the function node directly
                    FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
                    if (FunctionNode)
                    {
                        // Direct setup for known function
                        FunctionNode->FunctionReference.SetExternalMember(
                            FName(TEXT("GetActorOfClass")), 
                            TargetClass
                        );
                        
                        FunctionNode->NodePosX = NodePosition.X;
                        FunctionNode->NodePosY = NodePosition.Y;
                        EventGraph->AddNode(FunctionNode);
                        FunctionNode->CreateNewGuid();
                        FunctionNode->PostPlacedNewNode();
                        FunctionNode->AllocateDefaultPins();
                        
                        UE_LOG(LogTemp, Display, TEXT("Created GetActorOfClass node directly"));
                        
                        // List all pins
                        for (UEdGraphPin* Pin : FunctionNode->Pins)
                        {
                            UE_LOG(LogTemp, Display, TEXT("  - Pin: %s, Direction: %d, Category: %s"), 
                                   *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
                        }
                    }
                }
            }
        }
    }
    
    // If we still haven't found the function, try in the blueprint's class
    if (!Function && !FunctionNode)
    {
        UE_LOG(LogTemp, Display, TEXT("Trying to find function in blueprint class"));
        Function = Blueprint->GeneratedClass->FindFunctionByName(*FunctionName);
    }
    
    // Create the function call node if we found the function
    if (Function && !FunctionNode)
    {
        FunctionNode = FSpirrowBridgeCommonUtils::CreateFunctionCallNode(EventGraph, Function, NodePosition);
    }
    
    if (!FunctionNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s in target %s"), *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target));
    }

    // Set parameters if provided
    if (Params->HasField(TEXT("params")))
    {
        const TSharedPtr<FJsonObject>* ParamsObj;
        if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
        {
            // Process parameters
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
            {
                const FString& ParamName = Param.Key;
                const TSharedPtr<FJsonValue>& ParamValue = Param.Value;
                
                // Find the parameter pin
                UEdGraphPin* ParamPin = FSpirrowBridgeCommonUtils::FindPin(FunctionNode, ParamName, EGPD_Input);
                if (ParamPin)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found parameter pin '%s' of category '%s'"), 
                           *ParamName, *ParamPin->PinType.PinCategory.ToString());
                    UE_LOG(LogTemp, Display, TEXT("  Current default value: '%s'"), *ParamPin->DefaultValue);
                    if (ParamPin->PinType.PinSubCategoryObject.IsValid())
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Pin subcategory: '%s'"), 
                               *ParamPin->PinType.PinSubCategoryObject->GetName());
                    }
                    
                    // Set parameter based on type
                    if (ParamValue->Type == EJson::String)
                    {
                        FString StringVal = ParamValue->AsString();
                        UE_LOG(LogTemp, Display, TEXT("  Setting string parameter '%s' to: '%s'"), 
                               *ParamName, *StringVal);
                        
                        // Handle class reference parameters (e.g., ActorClass in GetActorOfClass)
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
                        {
                            // For class references, we require the exact class name with proper prefix
                            // - Actor classes must start with 'A' (e.g., ACameraActor)
                            // - Non-actor classes must start with 'U' (e.g., UObject)
                            const FString& ClassName = StringVal;

                            // TODO: This likely won't work in UE5.5+, so don't rely on it.
                            UClass* Class = FindObject<UClass>(nullptr, *ClassName);

                            if (!Class)
                            {
                                Class = LoadObject<UClass>(nullptr, *ClassName);
                                UE_LOG(LogSpirrowBridge, Display, TEXT("FindObject<UClass> failed. Assuming soft path  path: %s"), *ClassName);
                            }
                            
                            // If not found, try with Engine module path
                            if (!Class)
                            {
                                FString EngineClassName = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
                                Class = LoadObject<UClass>(nullptr, *EngineClassName);
                                UE_LOG(LogSpirrowBridge, Display, TEXT("Trying Engine module path: %s"), *EngineClassName);
                            }
                            
                            if (!Class)
                            {
                                UE_LOG(LogSpirrowBridge, Error, TEXT("Failed to find class '%s'. Make sure to use the exact class name with proper prefix (A for actors, U for non-actors)"), *ClassName);
                                return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to find class '%s'"), *ClassName));
                            }

                            const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
                            if (!K2Schema)
                            {
                                UE_LOG(LogSpirrowBridge, Error, TEXT("Failed to get K2Schema"));
                                return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get K2Schema"));
                            }

                            K2Schema->TrySetDefaultObject(*ParamPin, Class);
                            if (ParamPin->DefaultObject != Class)
                            {
                                UE_LOG(LogSpirrowBridge, Error, TEXT("Failed to set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                                return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to set class reference for pin '%s'"), *ParamPin->PinName.ToString()));
                            }

                            UE_LOG(LogSpirrowBridge, Log, TEXT("Successfully set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                            continue;
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                        {
                            bool BoolValue = ParamValue->AsBool();
                            ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                            UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                                   *ParamName, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
                        {
                            // Handle array parameters - like Vector parameters
                            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                            if (ParamValue->TryGetArray(ArrayValue))
                            {
                                // Check if this could be a vector (array of 3 numbers)
                                if (ArrayValue->Num() == 3)
                                {
                                    // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                    float X = (*ArrayValue)[0]->AsNumber();
                                    float Y = (*ArrayValue)[1]->AsNumber();
                                    float Z = (*ArrayValue)[2]->AsNumber();
                                    
                                    FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                    ParamPin->DefaultValue = VectorString;
                                    
                                    UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                           *ParamName, *VectorString);
                                    UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                           *ParamPin->DefaultValue);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                                }
                            }
                        }
                    }
                    else if (ParamValue->Type == EJson::Number)
                    {
                        // Handle integer vs float parameters correctly
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                    }
                    else if (ParamValue->Type == EJson::Boolean)
                    {
                        bool BoolValue = ParamValue->AsBool();
                        ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                        UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                               *ParamName, *ParamPin->DefaultValue);
                    }
                    else if (ParamValue->Type == EJson::Array)
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Processing array parameter '%s'"), *ParamName);
                        // Handle array parameters - like Vector parameters
                        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                        if (ParamValue->TryGetArray(ArrayValue))
                        {
                            // Check if this could be a vector (array of 3 numbers)
                            if (ArrayValue->Num() == 3 && 
                                (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct) &&
                                (ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get()))
                            {
                                // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                float X = (*ArrayValue)[0]->AsNumber();
                                float Y = (*ArrayValue)[1]->AsNumber();
                                float Z = (*ArrayValue)[2]->AsNumber();
                                
                                FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                ParamPin->DefaultValue = VectorString;
                                
                                UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                       *ParamName, *VectorString);
                                UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                       *ParamPin->DefaultValue);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                            }
                        }
                    }
                    // Add handling for other types as needed
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Parameter pin '%s' not found"), *ParamName);
                }
            }
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), FunctionNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    // Get optional parameters
    bool IsExposed = false;
    if (Params->HasField(TEXT("is_exposed")))
    {
        IsExposed = Params->GetBoolField(TEXT("is_exposed"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create variable based on type
    FEdGraphPinType PinType;
    
    // Set up pin type based on variable_type string
    if (VariableType == TEXT("Boolean"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (VariableType == TEXT("Integer") || VariableType == TEXT("Int"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (VariableType == TEXT("Float"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
    }
    else if (VariableType == TEXT("String"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (VariableType == TEXT("Vector"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
    }

    // Create the variable
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    // Set variable properties
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        // Set exposure in editor
        if (IsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit;
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the input action node
    UK2Node_InputAction* InputActionNode = FSpirrowBridgeCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
    if (!InputActionNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create input action node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the self node
    UK2Node_Self* SelfNode = FSpirrowBridgeCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
    if (!SelfNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create self node"));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeType;
    if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create a JSON array for the node GUIDs
    TArray<TSharedPtr<FJsonValue>> NodeGuidArray;
    
    // Filter nodes by the exact requested type
    if (NodeType == TEXT("Event"))
    {
        FString EventName;
        if (!Params->TryGetStringField(TEXT("event_type"), EventName))
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'event_type' parameter for Event node search"));
        }
        
        // Look for nodes with exact event name (e.g., ReceiveBeginPlay)
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
            {
                UE_LOG(LogTemp, Display, TEXT("Found event node with name %s: %s"), *EventName, *EventNode->NodeGuid.ToString());
                NodeGuidArray.Add(MakeShared<FJsonValueString>(EventNode->NodeGuid.ToString()));
            }
        }
    }
    // Add other node types as needed (InputAction, etc.)
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("node_guids"), NodeGuidArray);
    
    return ResultObj;
}

// ==================== NEW NODE MANIPULATION COMMANDS ====================

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleSetNodePinValue(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeId;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    FString PinName;
    if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
    }

    FString PinValue;
    if (!Params->TryGetStringField(TEXT("pin_value"), PinValue))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_value' parameter"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the node by GUID
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId)
        {
            TargetNode = Node;
            break;
        }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    // Find the pin
    UEdGraphPin* TargetPin = FSpirrowBridgeCommonUtils::FindPin(TargetNode, PinName, EGPD_Input);
    if (!TargetPin)
    {
        // Try output pin as fallback
        TargetPin = FSpirrowBridgeCommonUtils::FindPin(TargetNode, PinName, EGPD_Output);
    }

    if (!TargetPin)
    {
        // List available pins for debugging
        FString AvailablePins;
        for (UEdGraphPin* Pin : TargetNode->Pins)
        {
            AvailablePins += FString::Printf(TEXT("%s (%s), "), *Pin->PinName.ToString(), 
                Pin->Direction == EGPD_Input ? TEXT("In") : TEXT("Out"));
        }
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Pin not found: %s. Available pins: %s"), *PinName, *AvailablePins));
    }

    // Set the pin value based on pin type
    const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
    
    if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_String || 
        TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
    {
        TargetPin->DefaultValue = PinValue;
    }
    else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
    {
        TargetPin->DefaultValue = FString::FromInt(FCString::Atoi(*PinValue));
    }
    else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float ||
             TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
    {
        TargetPin->DefaultValue = FString::SanitizeFloat(FCString::Atof(*PinValue));
    }
    else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
    {
        TargetPin->DefaultValue = PinValue.ToBool() ? TEXT("true") : TEXT("false");
    }
    else if (TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
    {
        TargetPin->DefaultValue = PinValue;
    }
    else
    {
        // For other types, try setting directly
        TargetPin->DefaultValue = PinValue;
    }

    UE_LOG(LogTemp, Log, TEXT("Set pin '%s' on node '%s' to value '%s'"), *PinName, *NodeId, *PinValue);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetStringField(TEXT("pin_name"), PinName);
    ResultObj->SetStringField(TEXT("pin_value"), PinValue);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddVariableGetNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Verify variable exists
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

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the variable get node
    UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create variable get node"));
    }

    // Set up the variable reference
    GetNode->VariableReference.SetSelfMember(FName(*VariableName));

    // Set node position
    GetNode->NodePosX = NodePosition.X;
    GetNode->NodePosY = NodePosition.Y;

    // Add to graph
    EventGraph->AddNode(GetNode);
    GetNode->CreateNewGuid();
    GetNode->PostPlacedNewNode();
    GetNode->AllocateDefaultPins();
    GetNode->ReconstructNode();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created variable get node for '%s'"), *VariableName);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), GetNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddVariableSetNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Verify variable exists
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

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the variable set node
    UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(EventGraph);
    if (!SetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create variable set node"));
    }

    // Set up the variable reference
    SetNode->VariableReference.SetSelfMember(FName(*VariableName));

    // Set node position
    SetNode->NodePosX = NodePosition.X;
    SetNode->NodePosY = NodePosition.Y;

    // Add to graph
    EventGraph->AddNode(SetNode);
    SetNode->CreateNewGuid();
    SetNode->PostPlacedNewNode();
    SetNode->AllocateDefaultPins();
    SetNode->ReconstructNode();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created variable set node for '%s'"), *VariableName);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SetNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddBranchNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the Branch function (IfThenElse is the internal name)
    UFunction* BranchFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Conv_BoolToInt"));
    
    // Create a Branch node using UK2Node_IfThenElse
    UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(EventGraph);
    if (!BranchNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create branch node"));
    }

    // Set node position
    BranchNode->NodePosX = NodePosition.X;
    BranchNode->NodePosY = NodePosition.Y;

    // Add to graph
    EventGraph->AddNode(BranchNode);
    BranchNode->CreateNewGuid();
    BranchNode->PostPlacedNewNode();
    BranchNode->AllocateDefaultPins();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created branch node"));

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), BranchNode->NodeGuid.ToString());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleDeleteNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeId;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the node by GUID
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId)
        {
            TargetNode = Node;
            break;
        }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    // Break all pin connections first
    for (UEdGraphPin* Pin : TargetNode->Pins)
    {
        Pin->BreakAllPinLinks();
    }

    // Remove the node from the graph
    EventGraph->RemoveNode(TargetNode);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Deleted node: %s"), *NodeId);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetBoolField(TEXT("deleted"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleMoveNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeId;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    // Get new position
    if (!Params->HasField(TEXT("position")))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'position' parameter"));
    }
    FVector2D NewPosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("position"));

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the node by GUID
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == NodeId)
        {
            TargetNode = Node;
            break;
        }
    }

    if (!TargetNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
    }

    // Set the new position
    TargetNode->NodePosX = NewPosition.X;
    TargetNode->NodePosY = NewPosition.Y;

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Moved node %s to position (%f, %f)"), *NodeId, NewPosition.X, NewPosition.Y);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), NodeId);
    ResultObj->SetNumberField(TEXT("pos_x"), NewPosition.X);
    ResultObj->SetNumberField(TEXT("pos_y"), NewPosition.Y);
    return ResultObj;
}

// ==================== CONTROL FLOW NODES ====================

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddSequenceNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get optional num_outputs parameter (default: 2)
    int32 NumOutputs = 2;
    if (Params->HasField(TEXT("num_outputs")))
    {
        NumOutputs = Params->GetIntegerField(TEXT("num_outputs"));
        if (NumOutputs < 2) NumOutputs = 2;
        if (NumOutputs > 10) NumOutputs = 10; // Reasonable limit
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the sequence node
    UK2Node_ExecutionSequence* SequenceNode = NewObject<UK2Node_ExecutionSequence>(EventGraph);
    if (!SequenceNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create sequence node"));
    }

    // Set node position
    SequenceNode->NodePosX = NodePosition.X;
    SequenceNode->NodePosY = NodePosition.Y;

    // Add to graph
    EventGraph->AddNode(SequenceNode);
    SequenceNode->CreateNewGuid();
    SequenceNode->PostPlacedNewNode();
    SequenceNode->AllocateDefaultPins();

    // Add additional output pins if needed
    for (int32 i = 2; i < NumOutputs; ++i)
    {
        SequenceNode->AddInputPin();
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created sequence node with %d outputs"), NumOutputs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), SequenceNode->NodeGuid.ToString());
    ResultObj->SetNumberField(TEXT("num_outputs"), NumOutputs);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddDelayNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get optional duration parameter (default: 1.0)
    float Duration = 1.0f;
    if (Params->HasField(TEXT("duration")))
    {
        Duration = Params->GetNumberField(TEXT("duration"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the Delay function
    UFunction* DelayFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("Delay"));
    if (!DelayFunction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find Delay function"));
    }

    // Create the function call node
    UK2Node_CallFunction* DelayNode = FSpirrowBridgeCommonUtils::CreateFunctionCallNode(EventGraph, DelayFunction, NodePosition);
    if (!DelayNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create delay node"));
    }

    // Set the duration
    UEdGraphPin* DurationPin = FSpirrowBridgeCommonUtils::FindPin(DelayNode, TEXT("Duration"), EGPD_Input);
    if (DurationPin)
    {
        DurationPin->DefaultValue = FString::SanitizeFloat(Duration);
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created delay node with duration %f"), Duration);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), DelayNode->NodeGuid.ToString());
    ResultObj->SetNumberField(TEXT("duration"), Duration);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddForEachLoopNode(const TSharedPtr<FJsonObject>& Params)
{
    // ForEachLoop is implemented as a Blueprint macro, not a K2Node
    // This requires a different approach using UK2Node_MacroInstance
    // For now, return an error message
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("ForEach loop node is not yet supported. Use ForLoop macro in Blueprint editor instead."));
}

// ==================== DEBUG & UTILITY NODES ====================

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddPrintStringNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get optional message parameter
    FString Message = TEXT("Hello");
    Params->TryGetStringField(TEXT("message"), Message);

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Find the PrintString function
    UFunction* PrintStringFunction = UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString"));
    if (!PrintStringFunction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find PrintString function"));
    }

    // Create the function call node
    UK2Node_CallFunction* PrintStringNode = FSpirrowBridgeCommonUtils::CreateFunctionCallNode(EventGraph, PrintStringFunction, NodePosition);
    if (!PrintStringNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create PrintString node"));
    }

    // Set the message
    UEdGraphPin* InStringPin = FSpirrowBridgeCommonUtils::FindPin(PrintStringNode, TEXT("InString"), EGPD_Input);
    if (InStringPin)
    {
        InStringPin->DefaultValue = Message;
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created PrintString node with message: %s"), *Message);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), PrintStringNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("message"), Message);
    return ResultObj;
}

// ==================== MATH & COMPARISON NODES ====================

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddMathNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString Operation;
    if (!Params->TryGetStringField(TEXT("operation"), Operation))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'operation' parameter (Add, Subtract, Multiply, Divide)"));
    }

    // Get optional value_type parameter (default: Float)
    FString ValueType = TEXT("Float");
    Params->TryGetStringField(TEXT("value_type"), ValueType);

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Map operation to function name
    FName FunctionName;
    bool bIsIntType = ValueType.Equals(TEXT("Int"), ESearchCase::IgnoreCase);

    if (Operation.Equals(TEXT("Add"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_FloatFloat);
    }
    else if (Operation.Equals(TEXT("Subtract"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_FloatFloat);
    }
    else if (Operation.Equals(TEXT("Multiply"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_FloatFloat);
    }
    else if (Operation.Equals(TEXT("Divide"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_FloatFloat);
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown operation: %s. Use Add, Subtract, Multiply, or Divide"), *Operation));
    }

    // Create the function call node using SetExternalMember
    UK2Node_CallFunction* MathNode = NewObject<UK2Node_CallFunction>(EventGraph);
    if (!MathNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create math node"));
    }

    // Set up function reference
    MathNode->FunctionReference.SetExternalMember(FunctionName, UKismetMathLibrary::StaticClass());
    MathNode->NodePosX = NodePosition.X;
    MathNode->NodePosY = NodePosition.Y;

    // Add to graph
    EventGraph->AddNode(MathNode, false, false);
    MathNode->CreateNewGuid();
    MathNode->PostPlacedNewNode();
    MathNode->AllocateDefaultPins();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created math node: %s (%s)"), *Operation, *ValueType);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), MathNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("operation"), Operation);
    ResultObj->SetStringField(TEXT("value_type"), ValueType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddComparisonNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString Operation;
    if (!Params->TryGetStringField(TEXT("operation"), Operation))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'operation' parameter (Greater, GreaterEqual, Less, LessEqual, Equal, NotEqual)"));
    }

    // Get optional value_type parameter (default: Float)
    FString ValueType = TEXT("Float");
    Params->TryGetStringField(TEXT("value_type"), ValueType);

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FSpirrowBridgeCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Find the blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Map operation to function name
    FName FunctionName;
    bool bIsIntType = ValueType.Equals(TEXT("Int"), ESearchCase::IgnoreCase);

    if (Operation.Equals(TEXT("Greater"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_FloatFloat);
    }
    else if (Operation.Equals(TEXT("GreaterEqual"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_FloatFloat);
    }
    else if (Operation.Equals(TEXT("Less"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_FloatFloat);
    }
    else if (Operation.Equals(TEXT("LessEqual"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_FloatFloat);
    }
    else if (Operation.Equals(TEXT("Equal"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_FloatFloat);
    }
    else if (Operation.Equals(TEXT("NotEqual"), ESearchCase::IgnoreCase))
    {
        FunctionName = bIsIntType ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_IntInt) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_FloatFloat);
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown operation: %s. Use Greater, GreaterEqual, Less, LessEqual, Equal, or NotEqual"), *Operation));
    }

    // Create the function call node using SetExternalMember
    UK2Node_CallFunction* ComparisonNode = NewObject<UK2Node_CallFunction>(EventGraph);
    if (!ComparisonNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create comparison node"));
    }

    // Set up function reference
    ComparisonNode->FunctionReference.SetExternalMember(FunctionName, UKismetMathLibrary::StaticClass());
    ComparisonNode->NodePosX = NodePosition.X;
    ComparisonNode->NodePosY = NodePosition.Y;

    // Add to graph
    EventGraph->AddNode(ComparisonNode, false, false);
    ComparisonNode->CreateNewGuid();
    ComparisonNode->PostPlacedNewNode();
    ComparisonNode->AllocateDefaultPins();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Log, TEXT("Created comparison node: %s (%s)"), *Operation, *ValueType);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_id"), ComparisonNode->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("operation"), Operation);
    ResultObj->SetStringField(TEXT("value_type"), ValueType);
    return ResultObj;
} 