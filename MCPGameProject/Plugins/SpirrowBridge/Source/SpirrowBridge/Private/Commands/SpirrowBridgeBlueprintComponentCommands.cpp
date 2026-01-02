#include "Commands/SpirrowBridgeBlueprintComponentCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EditorAssetLibrary.h"
#include "UObject/UObjectIterator.h"
#include "AbilitySystemComponent.h"

FSpirrowBridgeBlueprintComponentCommands::FSpirrowBridgeBlueprintComponentCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintComponentCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("add_component_to_blueprint"))
    {
        return HandleAddComponentToBlueprint(Params);
    }
    else if (CommandType == TEXT("set_component_property"))
    {
        return HandleSetComponentProperty(Params);
    }
    else if (CommandType == TEXT("set_physics_properties"))
    {
        return HandleSetPhysicsProperties(Params);
    }
    else if (CommandType == TEXT("set_static_mesh_properties"))
    {
        return HandleSetStaticMeshProperties(Params);
    }
    else if (CommandType == TEXT("set_pawn_properties"))
    {
        return HandleSetPawnProperties(Params);
    }

    return nullptr; // Not handled by this class
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintComponentCommands::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentType;
    if (!Params->TryGetStringField(TEXT("component_type"), ComponentType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get path parameter (default: /Game/Blueprints)
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Ensure path ends with /
    if (!Path.EndsWith(TEXT("/")))
    {
        Path += TEXT("/");
    }

    // Construct the full asset path
    FString BlueprintPath = FString::Printf(TEXT("%s%s.%s"), *Path, *BlueprintName, *BlueprintName);

    // Load the blueprint
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintPath));
    }

    // Create the component - dynamically find the component class by name
    UClass* ComponentClass = nullptr;

    // ==================== Common Components - Explicit Support ====================
    if (ComponentType == TEXT("StaticMeshComponent") || ComponentType == TEXT("StaticMesh"))
    {
        ComponentClass = UStaticMeshComponent::StaticClass();
    }
    else if (ComponentType == TEXT("BoxComponent") || ComponentType == TEXT("Box"))
    {
        ComponentClass = UBoxComponent::StaticClass();
    }
    else if (ComponentType == TEXT("SphereComponent") || ComponentType == TEXT("Sphere"))
    {
        ComponentClass = USphereComponent::StaticClass();
    }
    else if (ComponentType == TEXT("AbilitySystemComponent") || ComponentType == TEXT("ASC"))
    {
        ComponentClass = UAbilitySystemComponent::StaticClass();
    }

    // Method 1: Try TObjectIterator to search all loaded classes
    if (!ComponentClass)
    {
        TArray<FString> ClassNamesToTry;
        ClassNamesToTry.Add(ComponentType);
        
        if (!ComponentType.StartsWith(TEXT("U")))
        {
            ClassNamesToTry.Add(TEXT("U") + ComponentType);
        }
        if (!ComponentType.EndsWith(TEXT("Component")))
        {
            ClassNamesToTry.Add(ComponentType + TEXT("Component"));
            if (!ComponentType.StartsWith(TEXT("U")))
            {
                ClassNamesToTry.Add(TEXT("U") + ComponentType + TEXT("Component"));
            }
        }

        for (const FString& ClassName : ClassNamesToTry)
        {
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (Class->GetName() == ClassName && Class->IsChildOf(UActorComponent::StaticClass()))
                {
                    ComponentClass = Class;
                    break;
                }
            }
            if (ComponentClass) break;
        }
    }

    // Method 2: Try LoadObject with common module paths
    if (!ComponentClass)
    {
        TArray<FString> ModulePaths;
        
        FString FullClassName = ComponentType;
        if (!FullClassName.StartsWith(TEXT("U")))
        {
            FullClassName = TEXT("U") + FullClassName;
        }
        if (!FullClassName.EndsWith(TEXT("Component")))
        {
            FullClassName = FullClassName + TEXT("Component");
        }
        
        ModulePaths.Add(FString::Printf(TEXT("/Script/Engine.%s"), *FullClassName));
        ModulePaths.Add(FString::Printf(TEXT("/Script/Engine.%s"), *ComponentType));
        ModulePaths.Add(FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *FullClassName));
        
        for (const FString& ModulePath : ModulePaths)
        {
            ComponentClass = LoadObject<UClass>(nullptr, *ModulePath);
            if (ComponentClass && ComponentClass->IsChildOf(UActorComponent::StaticClass()))
            {
                break;
            }
            ComponentClass = nullptr;
        }
    }
    
    // Verify that the class is a valid component type
    if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown component type: %s"), *ComponentType));
    }

    // Add the component to the blueprint
    USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, *ComponentName);
    if (NewNode)
    {
        // Set transform if provided
        USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate);
        if (SceneComponent)
        {
            if (Params->HasField(TEXT("location")))
            {
                SceneComponent->SetRelativeLocation(FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("location")));
            }
            if (Params->HasField(TEXT("rotation")))
            {
                SceneComponent->SetRelativeRotation(FSpirrowBridgeCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")));
            }
            if (Params->HasField(TEXT("scale")))
            {
                SceneComponent->SetRelativeScale3D(FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
            }
        }

        Blueprint->SimpleConstructionScript->AddNode(NewNode);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("component_name"), ComponentName);
        ResultObj->SetStringField(TEXT("component_type"), ComponentType);
        return ResultObj;
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to add component to blueprint"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintComponentCommands::HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params)
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

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
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

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    if (!Blueprint->SimpleConstructionScript)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid blueprint construction script"));
    }
    
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    // Get the component template
    UObject* ComponentTemplate = ComponentNode->ComponentTemplate;
    if (!ComponentTemplate)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid component template"));
    }

    // Set the property value
    if (Params->HasField(TEXT("property_value")))
    {
        TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));
        
        // Get the property
        FProperty* Property = FindFProperty<FProperty>(ComponentTemplate->GetClass(), *PropertyName);
        if (!Property)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Property %s not found on component %s"), *PropertyName, *ComponentName));
        }

        bool bSuccess = false;
        FString ErrorMessage;

        // Handle different property types
        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            if (StructProp->Struct == TBaseStructure<FVector>::Get())
            {
                if (JsonValue->Type == EJson::Array)
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = JsonValue->AsArray();
                    if (Arr.Num() == 3)
                    {
                        FVector Vec(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
                        void* PropertyAddr = StructProp->ContainerPtrToValuePtr<void>(ComponentTemplate);
                        StructProp->CopySingleValue(PropertyAddr, &Vec);
                        bSuccess = true;
                    }
                }
                else if (JsonValue->Type == EJson::Number)
                {
                    float Value = JsonValue->AsNumber();
                    FVector Vec(Value, Value, Value);
                    void* PropertyAddr = StructProp->ContainerPtrToValuePtr<void>(ComponentTemplate);
                    StructProp->CopySingleValue(PropertyAddr, &Vec);
                    bSuccess = true;
                }
            }
            else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
            {
                if (JsonValue->Type == EJson::Array)
                {
                    const TArray<TSharedPtr<FJsonValue>>& Arr = JsonValue->AsArray();
                    if (Arr.Num() == 3)
                    {
                        FRotator Rot(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
                        void* PropertyAddr = StructProp->ContainerPtrToValuePtr<void>(ComponentTemplate);
                        StructProp->CopySingleValue(PropertyAddr, &Rot);
                        bSuccess = true;
                    }
                }
            }
            else
            {
                bSuccess = FSpirrowBridgeCommonUtils::SetObjectProperty(ComponentTemplate, PropertyName, JsonValue, ErrorMessage);
            }
        }
        else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
        {
            if (JsonValue->Type == EJson::String)
            {
                FString EnumValueName = JsonValue->AsString();
                UEnum* Enum = EnumProp->GetEnum();
                if (Enum)
                {
                    int64 EnumValue = Enum->GetValueByNameString(EnumValueName);
                    if (EnumValue != INDEX_NONE)
                    {
                        EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ComponentTemplate, EnumValue);
                        bSuccess = true;
                    }
                }
            }
            else if (JsonValue->Type == EJson::Number)
            {
                int64 EnumValue = JsonValue->AsNumber();
                EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ComponentTemplate, EnumValue);
                bSuccess = true;
            }
        }
        else if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
        {
            if (JsonValue->Type == EJson::Number)
            {
                double Value = JsonValue->AsNumber();
                if (NumericProp->IsInteger())
                {
                    NumericProp->SetIntPropertyValue(ComponentTemplate, (int64)Value);
                    bSuccess = true;
                }
                else if (NumericProp->IsFloatingPoint())
                {
                    NumericProp->SetFloatingPointPropertyValue(ComponentTemplate, Value);
                    bSuccess = true;
                }
            }
        }
        else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
        {
            if (JsonValue->Type == EJson::Boolean)
            {
                BoolProp->SetPropertyValue_InContainer(ComponentTemplate, JsonValue->AsBool());
                bSuccess = true;
            }
        }
        else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
        {
            if (JsonValue->Type == EJson::Number)
            {
                FloatProp->SetPropertyValue_InContainer(ComponentTemplate, JsonValue->AsNumber());
                bSuccess = true;
            }
        }
        else
        {
            bSuccess = FSpirrowBridgeCommonUtils::SetObjectProperty(ComponentTemplate, PropertyName, JsonValue, ErrorMessage);
        }

        if (bSuccess)
        {
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("component"), ComponentName);
            ResultObj->SetStringField(TEXT("property"), PropertyName);
            ResultObj->SetBoolField(TEXT("success"), true);
            return ResultObj;
        }
        else
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrorMessage.IsEmpty() ? TEXT("Failed to set property") : ErrorMessage);
        }
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintComponentCommands::HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params)
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

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Set physics properties
    if (Params->HasField(TEXT("simulate_physics")))
    {
        PrimComponent->SetSimulatePhysics(Params->GetBoolField(TEXT("simulate_physics")));
    }

    if (Params->HasField(TEXT("mass")))
    {
        float Mass = Params->GetNumberField(TEXT("mass"));
        PrimComponent->SetMassOverrideInKg(NAME_None, Mass);
    }

    if (Params->HasField(TEXT("linear_damping")))
    {
        PrimComponent->SetLinearDamping(Params->GetNumberField(TEXT("linear_damping")));
    }

    if (Params->HasField(TEXT("angular_damping")))
    {
        PrimComponent->SetAngularDamping(Params->GetNumberField(TEXT("angular_damping")));
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintComponentCommands::HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params)
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

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
    if (!MeshComponent)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Component is not a static mesh component"));
    }

    // Set static mesh properties
    if (Params->HasField(TEXT("static_mesh")))
    {
        FString MeshPath = Params->GetStringField(TEXT("static_mesh"));
        UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
        if (Mesh)
        {
            MeshComponent->SetStaticMesh(Mesh);
        }
    }

    if (Params->HasField(TEXT("material")))
    {
        FString MaterialPath = Params->GetStringField(TEXT("material"));
        UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (Material)
        {
            MeshComponent->SetMaterial(0, Material);
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintComponentCommands::HandleSetPawnProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
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

    // Get the default object
    UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
    if (!DefaultObject)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get default object"));
    }

    // Track if any properties were set successfully
    bool bAnyPropertiesSet = false;
    TSharedPtr<FJsonObject> ResultsObj = MakeShared<FJsonObject>();
    
    // Set auto possess player if specified
    if (Params->HasField(TEXT("auto_possess_player")))
    {
        TSharedPtr<FJsonValue> AutoPossessValue = Params->Values.FindRef(TEXT("auto_possess_player"));
        
        FString ErrorMessage;
        if (FSpirrowBridgeCommonUtils::SetObjectProperty(DefaultObject, TEXT("AutoPossessPlayer"), AutoPossessValue, ErrorMessage))
        {
            bAnyPropertiesSet = true;
            TSharedPtr<FJsonObject> PropResultObj = MakeShared<FJsonObject>();
            PropResultObj->SetBoolField(TEXT("success"), true);
            ResultsObj->SetObjectField(TEXT("AutoPossessPlayer"), PropResultObj);
        }
        else
        {
            TSharedPtr<FJsonObject> PropResultObj = MakeShared<FJsonObject>();
            PropResultObj->SetBoolField(TEXT("success"), false);
            PropResultObj->SetStringField(TEXT("error"), ErrorMessage);
            ResultsObj->SetObjectField(TEXT("AutoPossessPlayer"), PropResultObj);
        }
    }
    
    // Set controller rotation properties
    const TCHAR* RotationProps[] = {
        TEXT("bUseControllerRotationYaw"),
        TEXT("bUseControllerRotationPitch"),
        TEXT("bUseControllerRotationRoll")
    };
    
    const TCHAR* ParamNames[] = {
        TEXT("use_controller_rotation_yaw"),
        TEXT("use_controller_rotation_pitch"),
        TEXT("use_controller_rotation_roll")
    };
    
    for (int32 i = 0; i < 3; i++)
    {
        if (Params->HasField(ParamNames[i]))
        {
            TSharedPtr<FJsonValue> Value = Params->Values.FindRef(ParamNames[i]);
            
            FString ErrorMessage;
            if (FSpirrowBridgeCommonUtils::SetObjectProperty(DefaultObject, RotationProps[i], Value, ErrorMessage))
            {
                bAnyPropertiesSet = true;
                TSharedPtr<FJsonObject> PropResultObj = MakeShared<FJsonObject>();
                PropResultObj->SetBoolField(TEXT("success"), true);
                ResultsObj->SetObjectField(RotationProps[i], PropResultObj);
            }
            else
            {
                TSharedPtr<FJsonObject> PropResultObj = MakeShared<FJsonObject>();
                PropResultObj->SetBoolField(TEXT("success"), false);
                PropResultObj->SetStringField(TEXT("error"), ErrorMessage);
                ResultsObj->SetObjectField(RotationProps[i], PropResultObj);
            }
        }
    }
    
    // Set can be damaged property
    if (Params->HasField(TEXT("can_be_damaged")))
    {
        TSharedPtr<FJsonValue> Value = Params->Values.FindRef(TEXT("can_be_damaged"));
        
        FString ErrorMessage;
        if (FSpirrowBridgeCommonUtils::SetObjectProperty(DefaultObject, TEXT("bCanBeDamaged"), Value, ErrorMessage))
        {
            bAnyPropertiesSet = true;
            TSharedPtr<FJsonObject> PropResultObj = MakeShared<FJsonObject>();
            PropResultObj->SetBoolField(TEXT("success"), true);
            ResultsObj->SetObjectField(TEXT("bCanBeDamaged"), PropResultObj);
        }
        else
        {
            TSharedPtr<FJsonObject> PropResultObj = MakeShared<FJsonObject>();
            PropResultObj->SetBoolField(TEXT("success"), false);
            PropResultObj->SetStringField(TEXT("error"), ErrorMessage);
            ResultsObj->SetObjectField(TEXT("bCanBeDamaged"), PropResultObj);
        }
    }

    if (bAnyPropertiesSet)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }
    else if (ResultsObj->Values.Num() == 0)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No properties specified to set"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetStringField(TEXT("blueprint"), BlueprintName);
    ResponseObj->SetBoolField(TEXT("success"), bAnyPropertiesSet);
    ResponseObj->SetObjectField(TEXT("results"), ResultsObj);
    return ResponseObj;
}
