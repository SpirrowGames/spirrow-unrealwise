#include "Commands/SpirrowBridgeCommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

// Struct types for specialized handling
#include "BehaviorTree/BehaviorTreeTypes.h"      // FBlackboardKeySelector
#include "DataProviders/AIDataProvider.h"        // FAIDataProviderFloatValue, FAIDataProviderIntValue, FAIDataProviderBoolValue

// ============================================
// JSON Response Utilities  
// ============================================

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::CreateErrorResponse(const FString& Message)
{
    return CreateErrorResponse(ESpirrowErrorCode::UnknownError, Message);
}

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::CreateErrorResponse(int32 ErrorCode, const FString& Message)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetNumberField(TEXT("error_code"), ErrorCode);
    ResponseObject->SetStringField(TEXT("error"), Message);
    return ResponseObject;
}

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::CreateErrorResponse(int32 ErrorCode, const FString& Message, const TSharedPtr<FJsonObject>& Details)
{
    TSharedPtr<FJsonObject> ResponseObject = CreateErrorResponse(ErrorCode, Message);
    if (Details.IsValid())
    {
        ResponseObject->SetObjectField(TEXT("details"), Details);
    }
    return ResponseObject;
}

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), true);
    
    if (Data.IsValid())
    {
        ResponseObject->SetObjectField(TEXT("data"), Data);
    }
    
    return ResponseObject;
}

void FSpirrowBridgeCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((int32)Value->AsNumber());
        }
    }
}

void FSpirrowBridgeCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((float)Value->AsNumber());
        }
    }
}

FVector2D FSpirrowBridgeCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector2D Result(0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
    }
    
    return Result;
}

FVector FSpirrowBridgeCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
        Result.Z = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

FRotator FSpirrowBridgeCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FRotator Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
        Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
        Result.Roll = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

// Blueprint Utilities
UBlueprint* FSpirrowBridgeCommonUtils::FindBlueprint(const FString& BlueprintName, const FString& Path)
{
    return FindBlueprintByName(BlueprintName, Path);
}

UBlueprint* FSpirrowBridgeCommonUtils::FindBlueprintByName(const FString& BlueprintName, const FString& Path)
{
    // Normalize path to ensure it ends with /
    FString NormalizedPath = Path;
    if (!NormalizedPath.EndsWith(TEXT("/")))
    {
        NormalizedPath += TEXT("/");
    }

    // Construct asset path: /Path/BlueprintName.BlueprintName
    FString AssetPath = FString::Printf(TEXT("%s%s.%s"), *NormalizedPath, *BlueprintName, *BlueprintName);
    return LoadObject<UBlueprint>(nullptr, *AssetPath);
}

UEdGraph* FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities
UK2Node_Event* FSpirrowBridgeCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"), 
                *EventName, *EventNode->NodeGuid.ToString());
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // Find the function to create the event
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s)"), 
            *EventName, *EventNode->NodeGuid.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s"), *EventName);
    }
    
    return EventNode;
}

UK2Node_CallFunction* FSpirrowBridgeCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

UK2Node_VariableGet* FSpirrowBridgeCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableGet* VariableGetNode = NewObject<UK2Node_VariableGet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableGetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableGetNode->NodePosX = Position.X;
        VariableGetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableGetNode, true);
        VariableGetNode->PostPlacedNewNode();
        VariableGetNode->AllocateDefaultPins();
        
        return VariableGetNode;
    }
    
    return nullptr;
}

UK2Node_VariableSet* FSpirrowBridgeCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableSet* VariableSetNode = NewObject<UK2Node_VariableSet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableSetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableSetNode->NodePosX = Position.X;
        VariableSetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableSetNode, true);
        VariableSetNode->PostPlacedNewNode();
        VariableSetNode->AllocateDefaultPins();
        
        return VariableSetNode;
    }
    
    return nullptr;
}

UK2Node_InputAction* FSpirrowBridgeCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    InputActionNode->InputActionName = FName(*ActionName);
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    Graph->AddNode(InputActionNode, true);
    InputActionNode->CreateNewGuid();
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    return InputActionNode;
}

UK2Node_Self* FSpirrowBridgeCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FSpirrowBridgeCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }
    
    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }
    
    return false;
}

UEdGraphPin* FSpirrowBridgeCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    
    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"), 
           *PinName, (int32)Direction, *Node->GetName());
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"), 
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }
    
    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) && 
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

// Actor utilities
TSharedPtr<FJsonValue> FSpirrowBridgeCommonUtils::ActorToJson(AActor* Actor)
{
    if (!Actor)
    {
        return MakeShared<FJsonValueNull>();
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return MakeShared<FJsonValueObject>(ActorObject);
}

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    if (!Actor)
    {
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return ActorObject;
}

UK2Node_Event* FSpirrowBridgeCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

bool FSpirrowBridgeCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!Object)
    {
        OutErrorMessage = TEXT("Invalid object");
        return false;
    }

    // Search through class hierarchy for the property
    FProperty* Property = nullptr;
    UClass* CurrentClass = Object->GetClass();

    while (CurrentClass && !Property)
    {
        Property = CurrentClass->FindPropertyByName(*PropertyName);
        if (!Property)
        {
            CurrentClass = CurrentClass->GetSuperClass();
        }
    }

    if (!Property)
    {
        // Log available properties for debugging
        UE_LOG(LogTemp, Warning, TEXT("SetObjectProperty - Property '%s' not found. Available properties:"), *PropertyName);
        for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
        {
            UE_LOG(LogTemp, Warning, TEXT("  - %s (%s)"), *PropIt->GetName(), *PropIt->GetCPPType());
        }
        OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return false;
    }

    void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Object);
    
    // Handle different property types
    if (Property->IsA<FBoolProperty>())
    {
        ((FBoolProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    else if (Property->IsA<FIntProperty>())
    {
        int32 IntValue = static_cast<int32>(Value->AsNumber());
        FIntProperty* IntProperty = CastField<FIntProperty>(Property);
        if (IntProperty)
        {
            IntProperty->SetPropertyValue_InContainer(Object, IntValue);
            return true;
        }
    }
    else if (Property->IsA<FFloatProperty>())
    {
        ((FFloatProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (Property->IsA<FStrProperty>())
    {
        ((FStrProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    else if (Property->IsA<FByteProperty>())
    {
        FByteProperty* ByteProp = CastField<FByteProperty>(Property);
        UEnum* EnumDef = ByteProp ? ByteProp->GetIntPropertyEnum() : nullptr;
        
        // If this is a TEnumAsByte property (has associated enum)
        if (EnumDef)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
                ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %d"), 
                      *PropertyName, ByteValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    uint8 ByteValue = FCString::Atoi(*EnumValueName);
                    ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %d"), 
                          *PropertyName, *EnumValueName, ByteValue);
                    return true;
                }
                
                // Handle qualified enum names (e.g., "Player0" or "EAutoReceiveInput::Player0")
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
        else
        {
            // Regular byte property
            uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
            ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
            return true;
        }
    }
    else if (Property->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProp = CastField<FEnumProperty>(Property);
        UEnum* EnumDef = EnumProp ? EnumProp->GetEnum() : nullptr;
        FNumericProperty* UnderlyingNumericProp = EnumProp ? EnumProp->GetUnderlyingProperty() : nullptr;
        
        if (EnumDef && UnderlyingNumericProp)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                int64 EnumValue = static_cast<int64>(Value->AsNumber());
                UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %lld"), 
                      *PropertyName, EnumValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    int64 EnumValue = FCString::Atoi64(*EnumValueName);
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                
                // Handle qualified enum names
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
    }
    // IMPORTANT: Check derived classes BEFORE base classes to avoid incorrect type matching
    // FClassProperty and FSoftObjectProperty are derived from FObjectProperty
    else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        // Handle class reference properties (TSubclassOf<T>) - MUST CHECK BEFORE FObjectProperty
        if (Value->Type == EJson::String)
        {
            FString ClassPath = Value->AsString();

            // ★ Handle None/empty/nullptr as null value ★
            if (ClassPath.IsEmpty() ||
                ClassPath.Equals(TEXT("None"), ESearchCase::IgnoreCase) ||
                ClassPath.Equals(TEXT("nullptr"), ESearchCase::IgnoreCase) ||
                ClassPath.Equals(TEXT("null"), ESearchCase::IgnoreCase))
            {
                ClassProp->SetObjectPropertyValue(PropertyAddr, nullptr);
                UE_LOG(LogTemp, Display, TEXT("Set class property %s to null (value: '%s')"), *PropertyName, *ClassPath);
                return true;
            }

            UClass* LoadedClass = nullptr;

            // Method 1: Try LoadClass for C++ classes (e.g., "/Script/Engine.Character")
            LoadedClass = LoadClass<UObject>(nullptr, *ClassPath);

            // Method 2: If LoadClass failed, try loading as Blueprint asset
            if (!LoadedClass)
            {
                UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(ClassPath);

                if (LoadedAsset)
                {
                    // Check if it's a Blueprint asset
                    UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset);
                    if (Blueprint && Blueprint->GeneratedClass)
                    {
                        LoadedClass = Blueprint->GeneratedClass;
                        UE_LOG(LogTemp, Display, TEXT("Loaded Blueprint class: %s -> %s"),
                               *ClassPath, *LoadedClass->GetName());
                    }
                    // Check if it's already a UClass (loaded via different path)
                    else if (UClass* DirectClass = Cast<UClass>(LoadedAsset))
                    {
                        LoadedClass = DirectClass;
                        UE_LOG(LogTemp, Display, TEXT("Loaded class directly: %s"), *ClassPath);
                    }
                }
            }

            // Method 3: Try alternative path format (without .AssetName suffix)
            if (!LoadedClass && !ClassPath.Contains(TEXT(".")))
            {
                FString AssetName = FPaths::GetCleanFilename(ClassPath);
                FString AlternativePath = ClassPath + TEXT(".") + AssetName;

                UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AlternativePath);
                if (LoadedAsset)
                {
                    UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset);
                    if (Blueprint && Blueprint->GeneratedClass)
                    {
                        LoadedClass = Blueprint->GeneratedClass;
                        UE_LOG(LogTemp, Display, TEXT("Loaded Blueprint class via alternative path: %s -> %s"),
                               *AlternativePath, *LoadedClass->GetName());
                    }
                }
            }

            if (LoadedClass)
            {
                // Verify the class is compatible with the property's meta class
                UClass* MetaClass = ClassProp->MetaClass;
                if (LoadedClass->IsChildOf(MetaClass))
                {
                    ClassProp->SetObjectPropertyValue(PropertyAddr, LoadedClass);
                    UE_LOG(LogTemp, Display, TEXT("Set class property %s to: %s (MetaClass: %s)"),
                           *PropertyName, *LoadedClass->GetName(), *MetaClass->GetName());
                    return true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(
                        TEXT("Class type mismatch for property %s: Expected child of %s, got %s"),
                        *PropertyName,
                        *MetaClass->GetName(),
                        *LoadedClass->GetName()
                    );
                    return false;
                }
            }
            else
            {
                OutErrorMessage = FString::Printf(TEXT("Failed to load class: %s"), *ClassPath);
                return false;
            }
        }
        else if (Value->Type == EJson::Null)
        {
            ClassProp->SetObjectPropertyValue(PropertyAddr, nullptr);
            UE_LOG(LogTemp, Display, TEXT("Set class property %s to null"), *PropertyName);
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(
                TEXT("Class property %s requires a string (class path) or null value"),
                *PropertyName
            );
            return false;
        }
    }
    else if (FSoftObjectProperty* SoftObjectProp = CastField<FSoftObjectProperty>(Property))
    {
        // Handle soft object reference properties (TSoftObjectPtr<T>) - CHECK BEFORE FObjectProperty
        if (Value->Type == EJson::String)
        {
            FString AssetPath = Value->AsString();

            // ★ Handle None/empty/nullptr as null value ★
            if (AssetPath.IsEmpty() ||
                AssetPath.Equals(TEXT("None"), ESearchCase::IgnoreCase) ||
                AssetPath.Equals(TEXT("nullptr"), ESearchCase::IgnoreCase) ||
                AssetPath.Equals(TEXT("null"), ESearchCase::IgnoreCase))
            {
                FSoftObjectPtr NullPtr;
                SoftObjectProp->SetPropertyValue(PropertyAddr, NullPtr);
                UE_LOG(LogTemp, Display, TEXT("Set soft object property %s to null (value: '%s')"), *PropertyName, *AssetPath);
                return true;
            }

            // Create soft object path and set it
            FSoftObjectPath TempPath(AssetPath);
            FSoftObjectPtr SoftPtr(TempPath);
            SoftObjectProp->SetPropertyValue(PropertyAddr, SoftPtr);

            UE_LOG(LogTemp, Display, TEXT("Set soft object property %s to: %s"),
                   *PropertyName, *AssetPath);
            return true;
        }
        else if (Value->Type == EJson::Null)
        {
            FSoftObjectPtr NullPtr;
            SoftObjectProp->SetPropertyValue(PropertyAddr, NullPtr);
            UE_LOG(LogTemp, Display, TEXT("Set soft object property %s to null"), *PropertyName);
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(
                TEXT("Soft object property %s requires a string (asset path) or null value"),
                *PropertyName
            );
            return false;
        }
    }
    else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
    {
        // Handle object reference properties (TObjectPtr<T>) - CHECK LAST (base class)
        if (Value->Type == EJson::String)
        {
            FString AssetPath = Value->AsString();

            // ★ Handle None/empty/nullptr as null value ★
            if (AssetPath.IsEmpty() ||
                AssetPath.Equals(TEXT("None"), ESearchCase::IgnoreCase) ||
                AssetPath.Equals(TEXT("nullptr"), ESearchCase::IgnoreCase) ||
                AssetPath.Equals(TEXT("null"), ESearchCase::IgnoreCase))
            {
                ObjectProp->SetObjectPropertyValue(PropertyAddr, nullptr);
                UE_LOG(LogTemp, Display, TEXT("Set object property %s to null (value: '%s')"), *PropertyName, *AssetPath);
                return true;
            }

            // Try to load the asset
            UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);

            // If loading failed and path doesn't contain a dot, try alternative format
            if (!LoadedAsset && !AssetPath.Contains(TEXT(".")))
            {
                FString AssetName = FPaths::GetCleanFilename(AssetPath);
                FString AlternativePath = AssetPath + TEXT(".") + AssetName;
                LoadedAsset = UEditorAssetLibrary::LoadAsset(AlternativePath);

                if (LoadedAsset)
                {
                    UE_LOG(LogTemp, Display, TEXT("Loaded asset via alternative path: %s"), *AlternativePath);
                }
            }

            if (LoadedAsset)
            {
                // Verify the loaded asset is compatible with the property's expected class
                UClass* ExpectedClass = ObjectProp->PropertyClass;
                if (LoadedAsset->IsA(ExpectedClass))
                {
                    ObjectProp->SetObjectPropertyValue(PropertyAddr, LoadedAsset);
                    UE_LOG(LogTemp, Display, TEXT("Set object property %s to: %s (Class: %s)"),
                           *PropertyName, *LoadedAsset->GetName(), *ExpectedClass->GetName());
                    return true;
                }
                else
                {
                    OutErrorMessage = FString::Printf(
                        TEXT("Asset type mismatch for property %s: Expected %s, got %s"),
                        *PropertyName,
                        *ExpectedClass->GetName(),
                        *LoadedAsset->GetClass()->GetName()
                    );
                    return false;
                }
            }
            else
            {
                OutErrorMessage = FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath);
                return false;
            }
        }
        else if (Value->Type == EJson::Null)
        {
            ObjectProp->SetObjectPropertyValue(PropertyAddr, nullptr);
            UE_LOG(LogTemp, Display, TEXT("Set object property %s to null"), *PropertyName);
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(
                TEXT("Object property %s requires a string (asset path) or null value"),
                *PropertyName
            );
            return false;
        }
    }
    // ============================================
    // FStructProperty handling
    // ============================================
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        return SetStructPropertyValue(PropertyAddr, StructProp, Value, OutErrorMessage);
    }
    // ============================================
    // FNameProperty handling
    // ============================================
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FName NameValue = FName(*Value->AsString());
            NameProp->SetPropertyValue(PropertyAddr, NameValue);
            UE_LOG(LogTemp, Display, TEXT("Set name property %s to: %s"), *PropertyName, *NameValue.ToString());
            return true;
        }
        else
        {
            OutErrorMessage = FString::Printf(TEXT("Name property %s requires a string value"), *PropertyName);
            return false;
        }
    }
    // ============================================
    // FDoubleProperty handling (UE5)
    // ============================================
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        DoubleProp->SetPropertyValue(PropertyAddr, Value->AsNumber());
        UE_LOG(LogTemp, Display, TEXT("Set double property %s to: %f"), *PropertyName, Value->AsNumber());
        return true;
    }

    OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s for property %s"),
                                    *Property->GetClass()->GetName(), *PropertyName);
    return false;
}

// ============================================
// Parameter Validation utilities
// ============================================

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::ValidateRequiredString(
    const TSharedPtr<FJsonObject>& Params, 
    const FString& ParamName, 
    FString& OutValue)
{
    if (!Params.IsValid())
    {
        return CreateErrorResponse(ESpirrowErrorCode::InvalidParams, TEXT("Invalid params object"));
    }
    
    if (!Params->HasField(ParamName))
    {
        return CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam, 
            FString::Printf(TEXT("Missing required parameter: %s"), *ParamName)
        );
    }
    
    if (!Params->TryGetStringField(ParamName, OutValue))
    {
        return CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamType, 
            FString::Printf(TEXT("Parameter '%s' must be a string"), *ParamName)
        );
    }
    
    if (OutValue.IsEmpty())
    {
        return CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamValue, 
            FString::Printf(TEXT("Parameter '%s' cannot be empty"), *ParamName)
        );
    }
    
    return nullptr; // Success - no error
}

void FSpirrowBridgeCommonUtils::GetOptionalString(
    const TSharedPtr<FJsonObject>& Params, 
    const FString& ParamName, 
    FString& OutValue, 
    const FString& DefaultValue)
{
    OutValue = DefaultValue;
    
    if (Params.IsValid() && Params->HasField(ParamName))
    {
        Params->TryGetStringField(ParamName, OutValue);
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::ValidateRequiredNumber(
    const TSharedPtr<FJsonObject>& Params, 
    const FString& ParamName, 
    double& OutValue)
{
    if (!Params.IsValid())
    {
        return CreateErrorResponse(ESpirrowErrorCode::InvalidParams, TEXT("Invalid params object"));
    }
    
    if (!Params->HasField(ParamName))
    {
        return CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam, 
            FString::Printf(TEXT("Missing required parameter: %s"), *ParamName)
        );
    }
    
    if (!Params->TryGetNumberField(ParamName, OutValue))
    {
        return CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamType, 
            FString::Printf(TEXT("Parameter '%s' must be a number"), *ParamName)
        );
    }
    
    return nullptr; // Success
}

void FSpirrowBridgeCommonUtils::GetOptionalNumber(
    const TSharedPtr<FJsonObject>& Params, 
    const FString& ParamName, 
    double& OutValue, 
    double DefaultValue)
{
    OutValue = DefaultValue;
    
    if (Params.IsValid() && Params->HasField(ParamName))
    {
        Params->TryGetNumberField(ParamName, OutValue);
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::ValidateRequiredBool(
    const TSharedPtr<FJsonObject>& Params, 
    const FString& ParamName, 
    bool& OutValue)
{
    if (!Params.IsValid())
    {
        return CreateErrorResponse(ESpirrowErrorCode::InvalidParams, TEXT("Invalid params object"));
    }
    
    if (!Params->HasField(ParamName))
    {
        return CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam, 
            FString::Printf(TEXT("Missing required parameter: %s"), *ParamName)
        );
    }
    
    if (!Params->TryGetBoolField(ParamName, OutValue))
    {
        return CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamType, 
            FString::Printf(TEXT("Parameter '%s' must be a boolean"), *ParamName)
        );
    }
    
    return nullptr; // Success
}

void FSpirrowBridgeCommonUtils::GetOptionalBool(
    const TSharedPtr<FJsonObject>& Params, 
    const FString& ParamName, 
    bool& OutValue, 
    bool DefaultValue)
{
    OutValue = DefaultValue;
    
    if (Params.IsValid() && Params->HasField(ParamName))
    {
        Params->TryGetBoolField(ParamName, OutValue);
    }
}

// ============================================
// Asset Validation utilities
// ============================================

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::ValidateBlueprint(
    const FString& BlueprintName, 
    const FString& Path, 
    UBlueprint*& OutBlueprint)
{
    OutBlueprint = FindBlueprintByName(BlueprintName, Path);
    
    if (!OutBlueprint)
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("blueprint_name"), BlueprintName);
        Details->SetStringField(TEXT("path"), Path);
        Details->SetStringField(TEXT("full_path"), FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName));
        
        return CreateErrorResponse(
            ESpirrowErrorCode::BlueprintNotFound, 
            FString::Printf(TEXT("Blueprint not found: %s at %s"), *BlueprintName, *Path),
            Details
        );
    }
    
    return nullptr; // Success
}

TSharedPtr<FJsonObject> FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(
    const FString& WidgetName, 
    const FString& Path, 
    UWidgetBlueprint*& OutWidget)
{
    // Normalize path
    FString NormalizedPath = Path;
    if (!NormalizedPath.EndsWith(TEXT("/")))
    {
        NormalizedPath += TEXT("/");
    }
    
    FString AssetPath = FString::Printf(TEXT("%s%s.%s"), *NormalizedPath, *WidgetName, *WidgetName);
    OutWidget = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
    
    if (!OutWidget)
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("widget_name"), WidgetName);
        Details->SetStringField(TEXT("path"), Path);
        Details->SetStringField(TEXT("full_path"), AssetPath);
        
        return CreateErrorResponse(
            ESpirrowErrorCode::WidgetNotFound, 
            FString::Printf(TEXT("Widget Blueprint not found: %s at %s"), *WidgetName, *Path),
            Details
        );
    }
    
    return nullptr; // Success
}

bool FSpirrowBridgeCommonUtils::IsValidAssetPath(const FString& Path, FString& OutErrorMessage)
{
    if (Path.IsEmpty())
    {
        OutErrorMessage = TEXT("Asset path cannot be empty");
        return false;
    }
    
    if (!Path.StartsWith(TEXT("/Game/")) && !Path.StartsWith(TEXT("/Engine/")))
    {
        OutErrorMessage = FString::Printf(TEXT("Asset path must start with /Game/ or /Engine/: %s"), *Path);
        return false;
    }
    
    // Check for invalid characters
    if (Path.Contains(TEXT("..")) || Path.Contains(TEXT("//")) || Path.Contains(TEXT(" ")))
    {
        OutErrorMessage = FString::Printf(TEXT("Asset path contains invalid characters: %s"), *Path);
        return false;
    }
    
    return true;
}

// ============================================
// Additional JSON Array utilities
// ============================================

FLinearColor FSpirrowBridgeCommonUtils::GetLinearColorFromJson(
    const TSharedPtr<FJsonObject>& JsonObject, 
    const FString& FieldName, 
    const FLinearColor& DefaultValue)
{
    if (!JsonObject->HasField(FieldName))
    {
        return DefaultValue;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        float R = (float)(*JsonArray)[0]->AsNumber();
        float G = (float)(*JsonArray)[1]->AsNumber();
        float B = (float)(*JsonArray)[2]->AsNumber();
        float A = JsonArray->Num() >= 4 ? (float)(*JsonArray)[3]->AsNumber() : 1.0f;
        return FLinearColor(R, G, B, A);
    }
    
    return DefaultValue;
}

// ============================================
// Logging utilities
// ============================================

// Define log category at top of file or in module
DEFINE_LOG_CATEGORY_STATIC(LogSpirrowBridge, Log, All);

void FSpirrowBridgeCommonUtils::LogCommandError(const FString& CommandName, const FString& Message)
{
    UE_LOG(LogSpirrowBridge, Error, TEXT("[%s] %s"), *CommandName, *Message);
}

void FSpirrowBridgeCommonUtils::LogCommandWarning(const FString& CommandName, const FString& Message)
{
    UE_LOG(LogSpirrowBridge, Warning, TEXT("[%s] %s"), *CommandName, *Message);
}

void FSpirrowBridgeCommonUtils::LogCommandInfo(const FString& CommandName, const FString& Message)
{
    UE_LOG(LogSpirrowBridge, Display, TEXT("[%s] %s"), *CommandName, *Message);
}

// ============================================
// Struct Property utilities
// ============================================

bool FSpirrowBridgeCommonUtils::SetStructPropertyValue(void* StructAddr, FStructProperty* StructProp,
                                                       const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!StructAddr || !StructProp)
    {
        OutErrorMessage = TEXT("Invalid struct address or property");
        return false;
    }

    UScriptStruct* ScriptStruct = StructProp->Struct;
    if (!ScriptStruct)
    {
        OutErrorMessage = TEXT("Invalid script struct");
        return false;
    }

    FString StructName = ScriptStruct->GetName();
    UE_LOG(LogTemp, Display, TEXT("SetStructPropertyValue: Processing struct type '%s'"), *StructName);

    // ============================================
    // Handle FBlackboardKeySelector (BehaviorTree)
    // ============================================
    if (StructName == TEXT("BlackboardKeySelector"))
    {
        FBlackboardKeySelector* KeySelector = static_cast<FBlackboardKeySelector*>(StructAddr);
        
        // Handle string input: just set SelectedKeyName
        if (Value->Type == EJson::String)
        {
            KeySelector->SelectedKeyName = FName(*Value->AsString());
            UE_LOG(LogTemp, Display, TEXT("Set FBlackboardKeySelector.SelectedKeyName to: %s"), 
                   *KeySelector->SelectedKeyName.ToString());
            return true;
        }
        // Handle object input: parse SelectedKeyName field
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            
            // SelectedKeyName (required)
            if (JsonObj->HasField(TEXT("SelectedKeyName")))
            {
                FString KeyName;
                if (JsonObj->TryGetStringField(TEXT("SelectedKeyName"), KeyName))
                {
                    KeySelector->SelectedKeyName = FName(*KeyName);
                    UE_LOG(LogTemp, Display, TEXT("Set FBlackboardKeySelector.SelectedKeyName to: %s"), *KeyName);
                }
            }
            
            return true;
        }
        else
        {
            OutErrorMessage = TEXT("FBlackboardKeySelector requires a string or object with SelectedKeyName");
            return false;
        }
    }
    // ============================================
    // Handle FVector
    // ============================================
    else if (StructName == TEXT("Vector"))
    {
        FVector* VectorPtr = static_cast<FVector*>(StructAddr);
        
        if (Value->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& JsonArray = Value->AsArray();
            if (JsonArray.Num() >= 3)
            {
                VectorPtr->X = JsonArray[0]->AsNumber();
                VectorPtr->Y = JsonArray[1]->AsNumber();
                VectorPtr->Z = JsonArray[2]->AsNumber();
                UE_LOG(LogTemp, Display, TEXT("Set FVector to: (%f, %f, %f)"), VectorPtr->X, VectorPtr->Y, VectorPtr->Z);
                return true;
            }
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("X"))) VectorPtr->X = JsonObj->GetNumberField(TEXT("X"));
            if (JsonObj->HasField(TEXT("Y"))) VectorPtr->Y = JsonObj->GetNumberField(TEXT("Y"));
            if (JsonObj->HasField(TEXT("Z"))) VectorPtr->Z = JsonObj->GetNumberField(TEXT("Z"));
            UE_LOG(LogTemp, Display, TEXT("Set FVector to: (%f, %f, %f)"), VectorPtr->X, VectorPtr->Y, VectorPtr->Z);
            return true;
        }
        OutErrorMessage = TEXT("FVector requires an array [X, Y, Z] or object {X, Y, Z}");
        return false;
    }
    // ============================================
    // Handle FVector2D
    // ============================================
    else if (StructName == TEXT("Vector2D"))
    {
        FVector2D* Vector2DPtr = static_cast<FVector2D*>(StructAddr);
        
        if (Value->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& JsonArray = Value->AsArray();
            if (JsonArray.Num() >= 2)
            {
                Vector2DPtr->X = JsonArray[0]->AsNumber();
                Vector2DPtr->Y = JsonArray[1]->AsNumber();
                UE_LOG(LogTemp, Display, TEXT("Set FVector2D to: (%f, %f)"), Vector2DPtr->X, Vector2DPtr->Y);
                return true;
            }
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("X"))) Vector2DPtr->X = JsonObj->GetNumberField(TEXT("X"));
            if (JsonObj->HasField(TEXT("Y"))) Vector2DPtr->Y = JsonObj->GetNumberField(TEXT("Y"));
            UE_LOG(LogTemp, Display, TEXT("Set FVector2D to: (%f, %f)"), Vector2DPtr->X, Vector2DPtr->Y);
            return true;
        }
        OutErrorMessage = TEXT("FVector2D requires an array [X, Y] or object {X, Y}");
        return false;
    }
    // ============================================
    // Handle FRotator
    // ============================================
    else if (StructName == TEXT("Rotator"))
    {
        FRotator* RotatorPtr = static_cast<FRotator*>(StructAddr);
        
        if (Value->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& JsonArray = Value->AsArray();
            if (JsonArray.Num() >= 3)
            {
                RotatorPtr->Pitch = JsonArray[0]->AsNumber();
                RotatorPtr->Yaw = JsonArray[1]->AsNumber();
                RotatorPtr->Roll = JsonArray[2]->AsNumber();
                UE_LOG(LogTemp, Display, TEXT("Set FRotator to: (Pitch=%f, Yaw=%f, Roll=%f)"), 
                       RotatorPtr->Pitch, RotatorPtr->Yaw, RotatorPtr->Roll);
                return true;
            }
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("Pitch"))) RotatorPtr->Pitch = JsonObj->GetNumberField(TEXT("Pitch"));
            if (JsonObj->HasField(TEXT("Yaw"))) RotatorPtr->Yaw = JsonObj->GetNumberField(TEXT("Yaw"));
            if (JsonObj->HasField(TEXT("Roll"))) RotatorPtr->Roll = JsonObj->GetNumberField(TEXT("Roll"));
            UE_LOG(LogTemp, Display, TEXT("Set FRotator to: (Pitch=%f, Yaw=%f, Roll=%f)"), 
                   RotatorPtr->Pitch, RotatorPtr->Yaw, RotatorPtr->Roll);
            return true;
        }
        OutErrorMessage = TEXT("FRotator requires an array [Pitch, Yaw, Roll] or object {Pitch, Yaw, Roll}");
        return false;
    }
    // ============================================
    // Handle FLinearColor
    // ============================================
    else if (StructName == TEXT("LinearColor"))
    {
        FLinearColor* ColorPtr = static_cast<FLinearColor*>(StructAddr);
        
        if (Value->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& JsonArray = Value->AsArray();
            if (JsonArray.Num() >= 3)
            {
                ColorPtr->R = JsonArray[0]->AsNumber();
                ColorPtr->G = JsonArray[1]->AsNumber();
                ColorPtr->B = JsonArray[2]->AsNumber();
                ColorPtr->A = JsonArray.Num() >= 4 ? JsonArray[3]->AsNumber() : 1.0f;
                UE_LOG(LogTemp, Display, TEXT("Set FLinearColor to: (R=%f, G=%f, B=%f, A=%f)"), 
                       ColorPtr->R, ColorPtr->G, ColorPtr->B, ColorPtr->A);
                return true;
            }
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("R"))) ColorPtr->R = JsonObj->GetNumberField(TEXT("R"));
            if (JsonObj->HasField(TEXT("G"))) ColorPtr->G = JsonObj->GetNumberField(TEXT("G"));
            if (JsonObj->HasField(TEXT("B"))) ColorPtr->B = JsonObj->GetNumberField(TEXT("B"));
            if (JsonObj->HasField(TEXT("A"))) ColorPtr->A = JsonObj->GetNumberField(TEXT("A"));
            UE_LOG(LogTemp, Display, TEXT("Set FLinearColor to: (R=%f, G=%f, B=%f, A=%f)"), 
                   ColorPtr->R, ColorPtr->G, ColorPtr->B, ColorPtr->A);
            return true;
        }
        OutErrorMessage = TEXT("FLinearColor requires an array [R, G, B, A] or object {R, G, B, A}");
        return false;
    }
    // ============================================
    // Handle FColor
    // ============================================
    else if (StructName == TEXT("Color"))
    {
        FColor* ColorPtr = static_cast<FColor*>(StructAddr);
        
        if (Value->Type == EJson::Array)
        {
            const TArray<TSharedPtr<FJsonValue>>& JsonArray = Value->AsArray();
            if (JsonArray.Num() >= 3)
            {
                ColorPtr->R = static_cast<uint8>(JsonArray[0]->AsNumber());
                ColorPtr->G = static_cast<uint8>(JsonArray[1]->AsNumber());
                ColorPtr->B = static_cast<uint8>(JsonArray[2]->AsNumber());
                ColorPtr->A = JsonArray.Num() >= 4 ? static_cast<uint8>(JsonArray[3]->AsNumber()) : 255;
                UE_LOG(LogTemp, Display, TEXT("Set FColor to: (R=%d, G=%d, B=%d, A=%d)"), 
                       ColorPtr->R, ColorPtr->G, ColorPtr->B, ColorPtr->A);
                return true;
            }
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("R"))) ColorPtr->R = static_cast<uint8>(JsonObj->GetNumberField(TEXT("R")));
            if (JsonObj->HasField(TEXT("G"))) ColorPtr->G = static_cast<uint8>(JsonObj->GetNumberField(TEXT("G")));
            if (JsonObj->HasField(TEXT("B"))) ColorPtr->B = static_cast<uint8>(JsonObj->GetNumberField(TEXT("B")));
            if (JsonObj->HasField(TEXT("A"))) ColorPtr->A = static_cast<uint8>(JsonObj->GetNumberField(TEXT("A")));
            UE_LOG(LogTemp, Display, TEXT("Set FColor to: (R=%d, G=%d, B=%d, A=%d)"), 
                   ColorPtr->R, ColorPtr->G, ColorPtr->B, ColorPtr->A);
            return true;
        }
        OutErrorMessage = TEXT("FColor requires an array [R, G, B, A] (0-255) or object {R, G, B, A}");
        return false;
    }
    // ============================================
    // Handle FAIDataProviderFloatValue (EQS scoring)
    // ============================================
    else if (StructName == TEXT("AIDataProviderFloatValue"))
    {
        FAIDataProviderFloatValue* DataProviderPtr = static_cast<FAIDataProviderFloatValue*>(StructAddr);
        
        if (Value->Type == EJson::Number)
        {
            DataProviderPtr->DefaultValue = Value->AsNumber();
            UE_LOG(LogTemp, Display, TEXT("Set FAIDataProviderFloatValue.DefaultValue to: %f"), 
                   DataProviderPtr->DefaultValue);
            return true;
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("DefaultValue")))
            {
                DataProviderPtr->DefaultValue = JsonObj->GetNumberField(TEXT("DefaultValue"));
            }
            else if (JsonObj->HasField(TEXT("Value")))
            {
                DataProviderPtr->DefaultValue = JsonObj->GetNumberField(TEXT("Value"));
            }
            UE_LOG(LogTemp, Display, TEXT("Set FAIDataProviderFloatValue.DefaultValue to: %f"), 
                   DataProviderPtr->DefaultValue);
            return true;
        }
        OutErrorMessage = TEXT("FAIDataProviderFloatValue requires a number or object with DefaultValue");
        return false;
    }
    // ============================================
    // Handle FAIDataProviderIntValue
    // ============================================
    else if (StructName == TEXT("AIDataProviderIntValue"))
    {
        FAIDataProviderIntValue* DataProviderPtr = static_cast<FAIDataProviderIntValue*>(StructAddr);
        
        if (Value->Type == EJson::Number)
        {
            DataProviderPtr->DefaultValue = static_cast<int32>(Value->AsNumber());
            UE_LOG(LogTemp, Display, TEXT("Set FAIDataProviderIntValue.DefaultValue to: %d"), 
                   DataProviderPtr->DefaultValue);
            return true;
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("DefaultValue")))
            {
                DataProviderPtr->DefaultValue = static_cast<int32>(JsonObj->GetNumberField(TEXT("DefaultValue")));
            }
            else if (JsonObj->HasField(TEXT("Value")))
            {
                DataProviderPtr->DefaultValue = static_cast<int32>(JsonObj->GetNumberField(TEXT("Value")));
            }
            UE_LOG(LogTemp, Display, TEXT("Set FAIDataProviderIntValue.DefaultValue to: %d"), 
                   DataProviderPtr->DefaultValue);
            return true;
        }
        OutErrorMessage = TEXT("FAIDataProviderIntValue requires a number or object with DefaultValue");
        return false;
    }
    // ============================================
    // Handle FAIDataProviderBoolValue
    // ============================================
    else if (StructName == TEXT("AIDataProviderBoolValue"))
    {
        FAIDataProviderBoolValue* DataProviderPtr = static_cast<FAIDataProviderBoolValue*>(StructAddr);
        
        if (Value->Type == EJson::Boolean)
        {
            DataProviderPtr->DefaultValue = Value->AsBool();
            UE_LOG(LogTemp, Display, TEXT("Set FAIDataProviderBoolValue.DefaultValue to: %s"), 
                   DataProviderPtr->DefaultValue ? TEXT("true") : TEXT("false"));
            return true;
        }
        else if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            if (JsonObj->HasField(TEXT("DefaultValue")))
            {
                DataProviderPtr->DefaultValue = JsonObj->GetBoolField(TEXT("DefaultValue"));
            }
            else if (JsonObj->HasField(TEXT("Value")))
            {
                DataProviderPtr->DefaultValue = JsonObj->GetBoolField(TEXT("Value"));
            }
            UE_LOG(LogTemp, Display, TEXT("Set FAIDataProviderBoolValue.DefaultValue to: %s"), 
                   DataProviderPtr->DefaultValue ? TEXT("true") : TEXT("false"));
            return true;
        }
        OutErrorMessage = TEXT("FAIDataProviderBoolValue requires a boolean or object with DefaultValue");
        return false;
    }
    // ============================================
    // Handle FTransform
    // ============================================
    else if (StructName == TEXT("Transform"))
    {
        FTransform* TransformPtr = static_cast<FTransform*>(StructAddr);
        
        if (Value->Type == EJson::Object)
        {
            const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
            
            // Location
            if (JsonObj->HasField(TEXT("Location")) || JsonObj->HasField(TEXT("Translation")))
            {
                const TSharedPtr<FJsonValue>& LocValue = JsonObj->HasField(TEXT("Location")) 
                    ? JsonObj->TryGetField(TEXT("Location")) 
                    : JsonObj->TryGetField(TEXT("Translation"));
                if (LocValue->Type == EJson::Array)
                {
                    const TArray<TSharedPtr<FJsonValue>>& LocArray = LocValue->AsArray();
                    if (LocArray.Num() >= 3)
                    {
                        TransformPtr->SetLocation(FVector(
                            LocArray[0]->AsNumber(), LocArray[1]->AsNumber(), LocArray[2]->AsNumber()));
                    }
                }
            }
            
            // Rotation
            if (JsonObj->HasField(TEXT("Rotation")))
            {
                const TSharedPtr<FJsonValue>& RotValue = JsonObj->TryGetField(TEXT("Rotation"));
                if (RotValue->Type == EJson::Array)
                {
                    const TArray<TSharedPtr<FJsonValue>>& RotArray = RotValue->AsArray();
                    if (RotArray.Num() >= 3)
                    {
                        TransformPtr->SetRotation(FQuat(FRotator(
                            RotArray[0]->AsNumber(), RotArray[1]->AsNumber(), RotArray[2]->AsNumber())));
                    }
                }
            }
            
            // Scale
            if (JsonObj->HasField(TEXT("Scale")) || JsonObj->HasField(TEXT("Scale3D")))
            {
                const TSharedPtr<FJsonValue>& ScaleValue = JsonObj->HasField(TEXT("Scale")) 
                    ? JsonObj->TryGetField(TEXT("Scale")) 
                    : JsonObj->TryGetField(TEXT("Scale3D"));
                if (ScaleValue->Type == EJson::Array)
                {
                    const TArray<TSharedPtr<FJsonValue>>& ScaleArray = ScaleValue->AsArray();
                    if (ScaleArray.Num() >= 3)
                    {
                        TransformPtr->SetScale3D(FVector(
                            ScaleArray[0]->AsNumber(), ScaleArray[1]->AsNumber(), ScaleArray[2]->AsNumber()));
                    }
                }
            }
            
            UE_LOG(LogTemp, Display, TEXT("Set FTransform"));
            return true;
        }
        OutErrorMessage = TEXT("FTransform requires an object with Location/Rotation/Scale arrays");
        return false;
    }
    // ============================================
    // Generic struct handling via reflection (fallback)
    // ============================================
    else if (Value->Type == EJson::Object)
    {
        const TSharedPtr<FJsonObject>& JsonObj = Value->AsObject();
        
        UE_LOG(LogTemp, Display, TEXT("Attempting generic struct handling for '%s'"), *StructName);
        
        // Iterate through JSON fields and set corresponding struct properties
        bool bSuccess = true;
        for (const auto& Pair : JsonObj->Values)
        {
            FString FieldError;
            if (!SetStructFieldValue(StructAddr, ScriptStruct, Pair.Key, Pair.Value, FieldError))
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to set struct field '%s': %s"), *Pair.Key, *FieldError);
                // Continue trying other fields, but mark as partial success
            }
        }
        
        return bSuccess;
    }

    OutErrorMessage = FString::Printf(TEXT("Struct type '%s' requires a JSON object for field mapping"), *StructName);
    return false;
}

bool FSpirrowBridgeCommonUtils::SetStructFieldValue(void* StructAddr, UScriptStruct* ScriptStruct,
                                                    const FString& FieldName, const TSharedPtr<FJsonValue>& Value,
                                                    FString& OutErrorMessage)
{
    if (!StructAddr || !ScriptStruct)
    {
        OutErrorMessage = TEXT("Invalid struct address or script struct");
        return false;
    }

    // Find the property in the struct
    FProperty* Property = ScriptStruct->FindPropertyByName(*FieldName);
    if (!Property)
    {
        OutErrorMessage = FString::Printf(TEXT("Field '%s' not found in struct '%s'"), *FieldName, *ScriptStruct->GetName());
        return false;
    }

    void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(StructAddr);

    // Handle basic types
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        BoolProp->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        IntProp->SetPropertyValue(PropertyAddr, static_cast<int32>(Value->AsNumber()));
        return true;
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        FloatProp->SetPropertyValue(PropertyAddr, static_cast<float>(Value->AsNumber()));
        return true;
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        DoubleProp->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        StrProp->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        NameProp->SetPropertyValue(PropertyAddr, FName(*Value->AsString()));
        return true;
    }
    else if (FStructProperty* NestedStructProp = CastField<FStructProperty>(Property))
    {
        // Recursive struct handling
        return SetStructPropertyValue(PropertyAddr, NestedStructProp, Value, OutErrorMessage);
    }

    OutErrorMessage = FString::Printf(TEXT("Unsupported field type for '%s'"), *FieldName);
    return false;
}