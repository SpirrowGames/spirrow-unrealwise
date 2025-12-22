#include "Commands/SpirrowBridgeBlueprintCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Controller.h"
#include "Blueprint/UserWidget.h"
#include "Animation/AnimInstance.h"
#include "Misc/App.h"
#include "UObject/UObjectIterator.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
// GAS Components
#include "AbilitySystemComponent.h"
// Widget Blueprint support
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

FSpirrowBridgeBlueprintCommands::FSpirrowBridgeBlueprintCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_blueprint"))
    {
        return HandleCreateBlueprint(Params);
    }
    else if (CommandType == TEXT("add_component_to_blueprint"))
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
    else if (CommandType == TEXT("compile_blueprint"))
    {
        return HandleCompileBlueprint(Params);
    }
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    else if (CommandType == TEXT("set_blueprint_property"))
    {
        return HandleSetBlueprintProperty(Params);
    }
    else if (CommandType == TEXT("set_static_mesh_properties"))
    {
        return HandleSetStaticMeshProperties(Params);
    }
    else if (CommandType == TEXT("set_pawn_properties"))
    {
        return HandleSetPawnProperties(Params);
    }
    else if (CommandType == TEXT("scan_project_classes"))
    {
        return HandleScanProjectClasses(Params);
    }
    else if (CommandType == TEXT("duplicate_blueprint"))
    {
        return HandleDuplicateBlueprint(Params);
    }
    else if (CommandType == TEXT("get_blueprint_graph"))
    {
        return HandleGetBlueprintGraph(Params);
    }
    else if (CommandType == TEXT("set_blueprint_class_array"))
    {
        return HandleSetBlueprintClassArray(Params);
    }
    else if (CommandType == TEXT("set_struct_array_property"))
    {
        return HandleSetStructArrayProperty(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
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

    // Check if blueprint already exists
    FString PackagePath = Path;
    FString AssetName = BlueprintName;
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath + AssetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint already exists: %s"), *BlueprintName));
    }

    // Create the blueprint factory
    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    
    // Handle parent class
    FString ParentClass;
    Params->TryGetStringField(TEXT("parent_class"), ParentClass);

    // Default to Actor if no parent class specified
    UClass* SelectedParentClass = AActor::StaticClass();

    // Try to find the specified parent class
    if (!ParentClass.IsEmpty())
    {
        FString ClassName = ParentClass;
        UClass* FoundClass = nullptr;

        // Method 1: Direct StaticClass lookup for common engine classes
        if (ClassName == TEXT("Pawn") || ClassName == TEXT("APawn"))
        {
            FoundClass = APawn::StaticClass();
            UE_LOG(LogTemp, Log, TEXT("[Method 1] Found parent class via StaticClass: APawn"));
        }
        else if (ClassName == TEXT("Actor") || ClassName == TEXT("AActor"))
        {
            FoundClass = AActor::StaticClass();
            UE_LOG(LogTemp, Log, TEXT("[Method 1] Found parent class via StaticClass: AActor"));
        }
        else if (ClassName == TEXT("Character") || ClassName == TEXT("ACharacter"))
        {
            FoundClass = ACharacter::StaticClass();
            UE_LOG(LogTemp, Log, TEXT("[Method 1] Found parent class via StaticClass: ACharacter"));
        }
        else if (ClassName == TEXT("UserWidget") || ClassName == TEXT("UUserWidget"))
        {
            FoundClass = UUserWidget::StaticClass();
            UE_LOG(LogTemp, Log, TEXT("[Method 1] Found parent class via StaticClass: UUserWidget"));
        }

        // Method 2: Search through all loaded classes using TObjectIterator
        if (!FoundClass)
        {
            // Try with original name
            UE_LOG(LogTemp, Log, TEXT("[Method 2] Searching for class '%s' using TObjectIterator..."), *ClassName);
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (Class->GetName() == ClassName)
                {
                    FoundClass = Class;
                    UE_LOG(LogTemp, Log, TEXT("[Method 2] Found parent class via TObjectIterator: %s (Path: %s)"),
                        *ClassName, *Class->GetPathName());
                    break;
                }
            }
        }

        // Method 2b: Try with 'A' prefix for Actor classes
        if (!FoundClass && !ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
        {
            FString ClassNameWithA = TEXT("A") + ClassName;
            UE_LOG(LogTemp, Log, TEXT("[Method 2b] Searching for class '%s' using TObjectIterator..."), *ClassNameWithA);
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (Class->GetName() == ClassNameWithA)
                {
                    FoundClass = Class;
                    UE_LOG(LogTemp, Log, TEXT("[Method 2b] Found parent class via TObjectIterator with A prefix: %s (Path: %s)"),
                        *ClassNameWithA, *Class->GetPathName());
                    break;
                }
            }
        }

        // Method 2c: Try with 'U' prefix for UObject classes
        if (!FoundClass && !ClassName.StartsWith(TEXT("U")))
        {
            FString ClassNameWithU = TEXT("U") + ClassName;
            UE_LOG(LogTemp, Log, TEXT("[Method 2c] Searching for class '%s' using TObjectIterator..."), *ClassNameWithU);
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (Class->GetName() == ClassNameWithU)
                {
                    FoundClass = Class;
                    UE_LOG(LogTemp, Log, TEXT("[Method 2c] Found parent class via TObjectIterator with U prefix: %s (Path: %s)"),
                        *ClassNameWithU, *Class->GetPathName());
                    break;
                }
            }
        }

        // Method 3: Try LoadObject with various module paths (for UObject, not just AActor)
        if (!FoundClass)
        {
            UE_LOG(LogTemp, Log, TEXT("[Method 3] Attempting LoadObject for '%s'..."), *ClassName);

            // Get project module name
            FString ProjectModuleName = FApp::GetProjectName();

            // Build list of paths to try
            TArray<FString> ModulePaths;

            // Try original name
            ModulePaths.Add(FString::Printf(TEXT("/Script/Engine.%s"), *ClassName));
            ModulePaths.Add(FString::Printf(TEXT("/Script/UMG.%s"), *ClassName));
            ModulePaths.Add(FString::Printf(TEXT("/Script/%s.%s"), *ProjectModuleName, *ClassName));

            for (const FString& ModulePath : ModulePaths)
            {
                UE_LOG(LogTemp, Log, TEXT("[Method 3] Trying LoadObject: %s"), *ModulePath);
                FoundClass = LoadObject<UClass>(nullptr, *ModulePath);
                if (FoundClass)
                {
                    UE_LOG(LogTemp, Log, TEXT("[Method 3] SUCCESS: Loaded class from '%s'"), *ModulePath);
                    break;
                }
            }
        }

        if (FoundClass)
        {
            SelectedParentClass = FoundClass;
            UE_LOG(LogTemp, Log, TEXT("Successfully set parent class to '%s' (Final Path: %s)"),
                *FoundClass->GetName(), *FoundClass->GetPathName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find specified parent class '%s' after trying all methods, defaulting to AActor"),
                *ParentClass);
        }
    }

    // === Handle different Blueprint types based on parent class ===
    if (SelectedParentClass->IsChildOf(UUserWidget::StaticClass()))
    {
        // Create Widget Blueprint
        UWidgetBlueprintFactory* WidgetFactory = NewObject<UWidgetBlueprintFactory>();
        WidgetFactory->ParentClass = SelectedParentClass;

        UPackage* Package = CreatePackage(*(PackagePath + AssetName));
        UBlueprint* NewBlueprint = Cast<UBlueprint>(WidgetFactory->FactoryCreateNew(
            UWidgetBlueprint::StaticClass(),
            Package,
            *AssetName,
            RF_Standalone | RF_Public,
            nullptr,
            GWarn
        ));

        if (NewBlueprint)
        {
            FAssetRegistryModule::AssetCreated(NewBlueprint);
            Package->MarkPackageDirty();

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("name"), AssetName);
            ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
            ResultObj->SetStringField(TEXT("type"), TEXT("WidgetBlueprint"));
            UE_LOG(LogTemp, Log, TEXT("Successfully created Widget Blueprint: %s"), *AssetName);
            return ResultObj;
        }
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Widget Blueprint"));
    }
    else
    {
        // Create regular Blueprint (Actor-based)
        Factory->ParentClass = SelectedParentClass;

        UPackage* Package = CreatePackage(*(PackagePath + AssetName));
        UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn));

        if (NewBlueprint)
        {
            // Notify the asset registry
            FAssetRegistryModule::AssetCreated(NewBlueprint);

            // Mark the package dirty
            Package->MarkPackageDirty();

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("name"), AssetName);
            ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
            ResultObj->SetStringField(TEXT("type"), TEXT("Blueprint"));
            UE_LOG(LogTemp, Log, TEXT("Successfully created Blueprint: %s"), *AssetName);
            return ResultObj;
        }

        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create blueprint"));
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params)
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

    // ==================== GAS Components - Explicit Support ====================
    // Explicit support for AbilitySystemComponent (ASC)
    if (ComponentType == TEXT("AbilitySystemComponent") || ComponentType == TEXT("ASC"))
    {
        ComponentClass = UAbilitySystemComponent::StaticClass();
        UE_LOG(LogTemp, Log, TEXT("Using AbilitySystemComponent via explicit StaticClass"));
    }
    // ===========================================================================

    // Try to find the class with exact name first
    if (!ComponentClass)
    {
        ComponentClass = FindObject<UClass>(nullptr, *ComponentType);
    }

    // If not found, try with "Component" suffix
    if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
    {
        FString ComponentTypeWithSuffix = ComponentType + TEXT("Component");
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithSuffix);
    }

    // If still not found, try with "U" prefix
    if (!ComponentClass && !ComponentType.StartsWith(TEXT("U")))
    {
        FString ComponentTypeWithPrefix = TEXT("U") + ComponentType;
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithPrefix);

        // Try with both prefix and suffix
        if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
        {
            FString ComponentTypeWithBoth = TEXT("U") + ComponentType + TEXT("Component");
            ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithBoth);
        }
    }
    
    // Verify that the class is a valid component type
    if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown component type: %s. Common types: StaticMeshComponent, BoxComponent, SphereComponent, CapsuleComponent, SkeletalMeshComponent, CameraComponent, SpringArmComponent, PointLightComponent, SpotLightComponent, AudioComponent, ParticleSystemComponent, SceneComponent, ArrowComponent, BillboardComponent, AbilitySystemComponent (ASC)"),
            *ComponentType));
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

        // Add to root if no parent specified
        Blueprint->SimpleConstructionScript->AddNode(NewNode);

        // Compile the blueprint
        FKismetEditorUtilities::CompileBlueprint(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("component_name"), ComponentName);
        ResultObj->SetStringField(TEXT("component_type"), ComponentType);
        return ResultObj;
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to add component to blueprint"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params)
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

    // Log all input parameters for debugging
    UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - Blueprint: %s, Component: %s, Property: %s"), 
        *BlueprintName, *ComponentName, *PropertyName);
    
    // Log property_value if available
    if (Params->HasField(TEXT("property_value")))
    {
        TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));
        FString ValueType;
        
        switch(JsonValue->Type)
        {
            case EJson::Boolean: ValueType = FString::Printf(TEXT("Boolean: %s"), JsonValue->AsBool() ? TEXT("true") : TEXT("false")); break;
            case EJson::Number: ValueType = FString::Printf(TEXT("Number: %f"), JsonValue->AsNumber()); break;
            case EJson::String: ValueType = FString::Printf(TEXT("String: %s"), *JsonValue->AsString()); break;
            case EJson::Array: ValueType = TEXT("Array"); break;
            case EJson::Object: ValueType = TEXT("Object"); break;
            default: ValueType = TEXT("Unknown"); break;
        }
        
        UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - Value Type: %s"), *ValueType);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - No property_value provided"));
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
        UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Blueprint not found: %s"), *BlueprintName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Blueprint found: %s (Class: %s)"), 
            *BlueprintName, 
            Blueprint->GeneratedClass ? *Blueprint->GeneratedClass->GetName() : TEXT("NULL"));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Searching for component %s in blueprint nodes"), *ComponentName);
    
    if (!Blueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - SimpleConstructionScript is NULL for blueprint %s"), *BlueprintName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid blueprint construction script"));
    }
    
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node)
        {
            UE_LOG(LogTemp, Verbose, TEXT("SetComponentProperty - Found node: %s"), *Node->GetVariableName().ToString());
            if (Node->GetVariableName().ToString() == ComponentName)
            {
                ComponentNode = Node;
                break;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - Found NULL node in blueprint"));
        }
    }

    if (!ComponentNode)
    {
        UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Component not found: %s"), *ComponentName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Component found: %s (Class: %s)"), 
            *ComponentName, 
            ComponentNode->ComponentTemplate ? *ComponentNode->ComponentTemplate->GetClass()->GetName() : TEXT("NULL"));
    }

    // Get the component template
    UObject* ComponentTemplate = ComponentNode->ComponentTemplate;
    if (!ComponentTemplate)
    {
        UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Component template is NULL for %s"), *ComponentName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid component template"));
    }

    // Check if this is a Spring Arm component and log special debug info
    if (ComponentTemplate->GetClass()->GetName().Contains(TEXT("SpringArm")))
    {
        UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - SpringArm component detected! Class: %s"), 
            *ComponentTemplate->GetClass()->GetPathName());
            
        // Log all properties of the SpringArm component class
        UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - SpringArm properties:"));
        for (TFieldIterator<FProperty> PropIt(ComponentTemplate->GetClass()); PropIt; ++PropIt)
        {
            FProperty* Prop = *PropIt;
            UE_LOG(LogTemp, Warning, TEXT("  - %s (%s)"), *Prop->GetName(), *Prop->GetCPPType());
        }

        // Special handling for Spring Arm properties
        if (Params->HasField(TEXT("property_value")))
        {
            TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));
            
            // Get the property using the new FField system
            FProperty* Property = FindFProperty<FProperty>(ComponentTemplate->GetClass(), *PropertyName);
            if (!Property)
            {
                UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Property %s not found on SpringArm component"), *PropertyName);
                return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                    FString::Printf(TEXT("Property %s not found on SpringArm component"), *PropertyName));
            }

            // Create a scope guard to ensure property cleanup
            struct FScopeGuard
            {
                UObject* Object;
                FScopeGuard(UObject* InObject) : Object(InObject) 
                {
                    if (Object)
                    {
                        Object->Modify();
                    }
                }
                ~FScopeGuard()
                {
                    if (Object)
                    {
                        Object->PostEditChange();
                    }
                }
            } ScopeGuard(ComponentTemplate);

            bool bSuccess = false;
            FString ErrorMessage;

            // Handle specific Spring Arm property types
            if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
            {
                if (JsonValue->Type == EJson::Number)
                {
                    const float Value = JsonValue->AsNumber();
                    UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Setting float property %s to %f"), *PropertyName, Value);
                    FloatProp->SetPropertyValue_InContainer(ComponentTemplate, Value);
                    bSuccess = true;
                }
            }
            else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
            {
                if (JsonValue->Type == EJson::Boolean)
                {
                    const bool Value = JsonValue->AsBool();
                    UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Setting bool property %s to %d"), *PropertyName, Value);
                    BoolProp->SetPropertyValue_InContainer(ComponentTemplate, Value);
                    bSuccess = true;
                }
            }
            else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
            {
                UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Handling struct property %s of type %s"), 
                    *PropertyName, *StructProp->Struct->GetName());
                
                // Special handling for common Spring Arm struct properties
                if (StructProp->Struct == TBaseStructure<FVector>::Get())
                {
                    if (JsonValue->Type == EJson::Array)
                    {
                        const TArray<TSharedPtr<FJsonValue>>& Arr = JsonValue->AsArray();
                        if (Arr.Num() == 3)
                        {
                            FVector Vec(
                                Arr[0]->AsNumber(),
                                Arr[1]->AsNumber(),
                                Arr[2]->AsNumber()
                            );
                            void* PropertyAddr = StructProp->ContainerPtrToValuePtr<void>(ComponentTemplate);
                            StructProp->CopySingleValue(PropertyAddr, &Vec);
                            bSuccess = true;
                        }
                    }
                }
                else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
                {
                    if (JsonValue->Type == EJson::Array)
                    {
                        const TArray<TSharedPtr<FJsonValue>>& Arr = JsonValue->AsArray();
                        if (Arr.Num() == 3)
                        {
                            FRotator Rot(
                                Arr[0]->AsNumber(),
                                Arr[1]->AsNumber(),
                                Arr[2]->AsNumber()
                            );
                            void* PropertyAddr = StructProp->ContainerPtrToValuePtr<void>(ComponentTemplate);
                            StructProp->CopySingleValue(PropertyAddr, &Rot);
                            bSuccess = true;
                        }
                    }
                }
            }

            if (bSuccess)
            {
                // Mark the blueprint as modified
                UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Successfully set SpringArm property %s"), *PropertyName);
                FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

                TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                ResultObj->SetStringField(TEXT("component"), ComponentName);
                ResultObj->SetStringField(TEXT("property"), PropertyName);
                ResultObj->SetBoolField(TEXT("success"), true);
                return ResultObj;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Failed to set SpringArm property %s"), *PropertyName);
                return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                    FString::Printf(TEXT("Failed to set SpringArm property %s"), *PropertyName));
            }
        }
    }

    // Regular property handling for non-Spring Arm components continues...

    // Set the property value
    if (Params->HasField(TEXT("property_value")))
    {
        TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));
        
        // Get the property
        FProperty* Property = FindFProperty<FProperty>(ComponentTemplate->GetClass(), *PropertyName);
        if (!Property)
        {
            UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Property %s not found on component %s"), 
                *PropertyName, *ComponentName);
            
            // List all available properties for this component
            UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - Available properties for %s:"), *ComponentName);
            for (TFieldIterator<FProperty> PropIt(ComponentTemplate->GetClass()); PropIt; ++PropIt)
            {
                FProperty* Prop = *PropIt;
                UE_LOG(LogTemp, Warning, TEXT("  - %s (%s)"), *Prop->GetName(), *Prop->GetCPPType());
            }
            
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Property %s not found on component %s"), *PropertyName, *ComponentName));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Property found: %s (Type: %s)"), 
                *PropertyName, *Property->GetCPPType());
        }

        bool bSuccess = false;
        FString ErrorMessage;

        // Handle different property types
        UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Attempting to set property %s"), *PropertyName);
        
        // Add try-catch block to catch and log any crashes
        try
        {
            if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
            {
                // Handle vector properties
                UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Property is a struct: %s"), 
                    StructProp->Struct ? *StructProp->Struct->GetName() : TEXT("NULL"));
                    
                if (StructProp->Struct == TBaseStructure<FVector>::Get())
                {
                    if (JsonValue->Type == EJson::Array)
                    {
                        // Handle array input [x, y, z]
                        const TArray<TSharedPtr<FJsonValue>>& Arr = JsonValue->AsArray();
                        if (Arr.Num() == 3)
                        {
                            FVector Vec(
                                Arr[0]->AsNumber(),
                                Arr[1]->AsNumber(),
                                Arr[2]->AsNumber()
                            );
                            void* PropertyAddr = StructProp->ContainerPtrToValuePtr<void>(ComponentTemplate);
                            UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Setting Vector(%f, %f, %f)"), 
                                Vec.X, Vec.Y, Vec.Z);
                            StructProp->CopySingleValue(PropertyAddr, &Vec);
                            bSuccess = true;
                        }
                        else
                        {
                            ErrorMessage = FString::Printf(TEXT("Vector property requires 3 values, got %d"), Arr.Num());
                            UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - %s"), *ErrorMessage);
                        }
                    }
                    else if (JsonValue->Type == EJson::Number)
                    {
                        // Handle scalar input (sets all components to same value)
                        float Value = JsonValue->AsNumber();
                        FVector Vec(Value, Value, Value);
                        void* PropertyAddr = StructProp->ContainerPtrToValuePtr<void>(ComponentTemplate);
                        UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Setting Vector(%f, %f, %f) from scalar"), 
                            Vec.X, Vec.Y, Vec.Z);
                        StructProp->CopySingleValue(PropertyAddr, &Vec);
                        bSuccess = true;
                    }
                    else
                    {
                        ErrorMessage = TEXT("Vector property requires either a single number or array of 3 numbers");
                        UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - %s"), *ErrorMessage);
                    }
                }
                else
                {
                    // Handle other struct properties using default handler
                    UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Using generic struct handler for %s"), 
                        *PropertyName);
                    bSuccess = FSpirrowBridgeCommonUtils::SetObjectProperty(ComponentTemplate, PropertyName, JsonValue, ErrorMessage);
                    if (!bSuccess)
                    {
                        UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Failed to set struct property: %s"), *ErrorMessage);
                    }
                }
            }
            else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
            {
                // Handle enum properties
                UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Property is an enum"));
                if (JsonValue->Type == EJson::String)
                {
                    FString EnumValueName = JsonValue->AsString();
                    UEnum* Enum = EnumProp->GetEnum();
                    UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Setting enum from string: %s"), *EnumValueName);
                    
                    if (Enum)
                    {
                        int64 EnumValue = Enum->GetValueByNameString(EnumValueName);
                        
                        if (EnumValue != INDEX_NONE)
                        {
                            UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Found enum value: %lld"), EnumValue);
                            EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
                                ComponentTemplate, 
                                EnumValue
                            );
                            bSuccess = true;
                        }
                        else
                        {
                            // List all possible enum values
                            UE_LOG(LogTemp, Warning, TEXT("SetComponentProperty - Available enum values for %s:"), 
                                *Enum->GetName());
                            for (int32 i = 0; i < Enum->NumEnums(); i++)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("  - %s (%lld)"), 
                                    *Enum->GetNameStringByIndex(i),
                                    Enum->GetValueByIndex(i));
                            }
                            
                            ErrorMessage = FString::Printf(TEXT("Invalid enum value '%s' for property %s"), 
                                *EnumValueName, *PropertyName);
                            UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - %s"), *ErrorMessage);
                        }
                    }
                    else
                    {
                        ErrorMessage = TEXT("Enum object is NULL");
                        UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - %s"), *ErrorMessage);
                    }
                }
                else if (JsonValue->Type == EJson::Number)
                {
                    // Allow setting enum by integer value
                    int64 EnumValue = JsonValue->AsNumber();
                    UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Setting enum from number: %lld"), EnumValue);
                    EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
                        ComponentTemplate, 
                        EnumValue
                    );
                    bSuccess = true;
                }
                else
                {
                    ErrorMessage = TEXT("Enum property requires either a string name or integer value");
                    UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - %s"), *ErrorMessage);
                }
            }
            else if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
            {
                // Handle numeric properties
                UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Property is numeric: IsInteger=%d, IsFloat=%d"), 
                    NumericProp->IsInteger(), NumericProp->IsFloatingPoint());
                    
                if (JsonValue->Type == EJson::Number)
                {
                    double Value = JsonValue->AsNumber();
                    UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Setting numeric value: %f"), Value);
                    
                    if (NumericProp->IsInteger())
                    {
                        NumericProp->SetIntPropertyValue(ComponentTemplate, (int64)Value);
                        UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Set integer value: %lld"), (int64)Value);
                        bSuccess = true;
                    }
                    else if (NumericProp->IsFloatingPoint())
                    {
                        NumericProp->SetFloatingPointPropertyValue(ComponentTemplate, Value);
                        UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Set float value: %f"), Value);
                        bSuccess = true;
                    }
                }
                else
                {
                    ErrorMessage = TEXT("Numeric property requires a number value");
                    UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - %s"), *ErrorMessage);
                }
            }
            else
            {
                // Handle all other property types using default handler
                UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Using generic property handler for %s (Type: %s)"), 
                    *PropertyName, *Property->GetCPPType());
                bSuccess = FSpirrowBridgeCommonUtils::SetObjectProperty(ComponentTemplate, PropertyName, JsonValue, ErrorMessage);
                if (!bSuccess)
                {
                    UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Failed to set property: %s"), *ErrorMessage);
                }
            }
        }
        catch (const std::exception& Ex)
        {
            UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - EXCEPTION: %s"), ANSI_TO_TCHAR(Ex.what()));
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Exception while setting property %s: %s"), *PropertyName, ANSI_TO_TCHAR(Ex.what())));
        }
        catch (...)
        {
            UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - UNKNOWN EXCEPTION occurred while setting property %s"), *PropertyName);
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Unknown exception while setting property %s"), *PropertyName));
        }

        if (bSuccess)
        {
            // Mark the blueprint as modified
            UE_LOG(LogTemp, Log, TEXT("SetComponentProperty - Successfully set property %s on component %s"), 
                *PropertyName, *ComponentName);
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("component"), ComponentName);
            ResultObj->SetStringField(TEXT("property"), PropertyName);
            ResultObj->SetBoolField(TEXT("success"), true);
            return ResultObj;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Failed to set property %s: %s"), 
                *PropertyName, *ErrorMessage);
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrorMessage);
        }
    }

    UE_LOG(LogTemp, Error, TEXT("SetComponentProperty - Missing 'property_value' parameter"));
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params)
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
        // In UE5.5, use proper overrideMass instead of just scaling
        PrimComponent->SetMassOverrideInKg(NAME_None, Mass);
        UE_LOG(LogTemp, Display, TEXT("Set mass for component %s to %f kg"), *ComponentName, Mass);
    }

    if (Params->HasField(TEXT("linear_damping")))
    {
        PrimComponent->SetLinearDamping(Params->GetNumberField(TEXT("linear_damping")));
    }

    if (Params->HasField(TEXT("angular_damping")))
    {
        PrimComponent->SetAngularDamping(Params->GetNumberField(TEXT("angular_damping")));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params)
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

    // Compile the blueprint
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), BlueprintName);
    ResultObj->SetStringField(TEXT("path"), Path + BlueprintName);
    ResultObj->SetBoolField(TEXT("compiled"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
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

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FSpirrowBridgeCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform);
    if (NewActor)
    {
        NewActor->SetActorLabel(*ActorName);
        return FSpirrowBridgeCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetBlueprintProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
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

    // === DEBUG: Log class hierarchy ===
    UE_LOG(LogTemp, Warning, TEXT("=== SetBlueprintProperty Debug ==="));
    UE_LOG(LogTemp, Warning, TEXT("Blueprint: %s"), *BlueprintName);
    UE_LOG(LogTemp, Warning, TEXT("GeneratedClass: %s"), Blueprint->GeneratedClass ? *Blueprint->GeneratedClass->GetName() : TEXT("NULL"));

    if (Blueprint->GeneratedClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Class Hierarchy:"));
        UClass* CurrentClass = Blueprint->GeneratedClass;
        int32 Depth = 0;
        while (CurrentClass && Depth < 10)
        {
            UE_LOG(LogTemp, Warning, TEXT("  [%d] %s"), Depth, *CurrentClass->GetName());
            CurrentClass = CurrentClass->GetSuperClass();
            Depth++;
        }
    }
    // === END DEBUG ===

    // Get the default object
    UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
    if (!DefaultObject)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get default object"));
    }

    // === DEBUG: Log DefaultObject info ===
    UE_LOG(LogTemp, Warning, TEXT("DefaultObject Class: %s"), *DefaultObject->GetClass()->GetName());
    UE_LOG(LogTemp, Warning, TEXT("Looking for property: %s"), *PropertyName);

    // List ALL properties including inherited ones
    UE_LOG(LogTemp, Warning, TEXT("All properties (including inherited):"));
    for (TFieldIterator<FProperty> PropIt(DefaultObject->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
    {
        FProperty* Prop = *PropIt;
        UE_LOG(LogTemp, Warning, TEXT("  - %s (%s) [Owner: %s]"),
            *Prop->GetName(),
            *Prop->GetCPPType(),
            *Prop->GetOwnerClass()->GetName());
    }
    // === END DEBUG ===

    // Set the property value
    if (Params->HasField(TEXT("property_value")))
    {
        TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));
        
        FString ErrorMessage;
        if (FSpirrowBridgeCommonUtils::SetObjectProperty(DefaultObject, PropertyName, JsonValue, ErrorMessage))
        {
            // Mark the blueprint as modified
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("property"), PropertyName);
            ResultObj->SetBoolField(TEXT("success"), true);
            return ResultObj;
        }
        else
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrorMessage);
        }
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params)
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

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetPawnProperties(const TSharedPtr<FJsonObject>& Params)
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

    // Mark the blueprint as modified if any properties were set
    if (bAnyPropertiesSet)
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }
    else if (ResultsObj->Values.Num() == 0)
    {
        // No properties were specified
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No properties specified to set"));
    }

    TSharedPtr<FJsonObject> ResponseObj = MakeShared<FJsonObject>();
    ResponseObj->SetStringField(TEXT("blueprint"), BlueprintName);
    ResponseObj->SetBoolField(TEXT("success"), bAnyPropertiesSet);
    ResponseObj->SetObjectField(TEXT("results"), ResultsObj);
    return ResponseObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleScanProjectClasses(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters
    FString ClassType = TEXT("all");
    Params->TryGetStringField(TEXT("class_type"), ClassType);

    FString ParentClassFilter;
    Params->TryGetStringField(TEXT("parent_class"), ParentClassFilter);

    FString ModuleFilter;
    Params->TryGetStringField(TEXT("module_filter"), ModuleFilter);

    FString PathFilter;
    Params->TryGetStringField(TEXT("path_filter"), PathFilter);

    bool bIncludeEngine = false;
    if (Params->HasField(TEXT("include_engine")))
    {
        bIncludeEngine = Params->GetBoolField(TEXT("include_engine"));
    }

    bool bExcludeReinst = true;  // Default: exclude REINST classes
    if (Params->HasField(TEXT("exclude_reinst")))
    {
        bExcludeReinst = Params->GetBoolField(TEXT("exclude_reinst"));
    }

    FString BlueprintTypeFilter;
    Params->TryGetStringField(TEXT("blueprint_type"), BlueprintTypeFilter);

    // Result arrays
    TArray<TSharedPtr<FJsonValue>> CppClassesArray;
    TArray<TSharedPtr<FJsonValue>> BlueprintsArray;

    // === Scan C++ classes ===
    if (ClassType == TEXT("all") || ClassType == TEXT("cpp"))
    {
        for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
        {
            UClass* TestClass = *ClassIt;
            if (!TestClass || !TestClass->IsChildOf(AActor::StaticClass()))
            {
                continue;
            }

            FString ClassPath = TestClass->GetPathName();
            FString ClassName = TestClass->GetName();

            // Filter Engine classes
            if (!bIncludeEngine)
            {
                if (ClassPath.Contains(TEXT("/Script/Engine.")) ||
                    ClassPath.Contains(TEXT("/Script/CoreUObject.")) ||
                    ClassPath.Contains(TEXT("/Script/UMG.")) ||
                    ClassPath.Contains(TEXT("/Script/AIModule.")) ||
                    ClassPath.Contains(TEXT("/Script/NavigationSystem.")) ||
                    ClassPath.Contains(TEXT("/Script/PhysicsCore.")) ||
                    ClassPath.Contains(TEXT("/Script/EnhancedInput.")) ||
                    ClassPath.Contains(TEXT("/Script/InputCore.")))
                {
                    continue;
                }
            }

            // Skip Blueprint-generated classes (_C) - handled in Blueprint section
            if (ClassName.EndsWith(TEXT("_C")))
            {
                continue;
            }

            // Filter REINST_* classes (Live Coding temporary classes)
            if (bExcludeReinst)
            {
                if (ClassName.StartsWith(TEXT("REINST_")) ||
                    ClassPath.Contains(TEXT("/Engine/Transient.")))
                {
                    continue;
                }
            }

            // Module filter
            if (!ModuleFilter.IsEmpty())
            {
                if (!ClassPath.Contains(FString::Printf(TEXT("/Script/%s."), *ModuleFilter)))
                {
                    continue;
                }
            }

            // Parent class filter
            if (!ParentClassFilter.IsEmpty())
            {
                UClass* ParentClass = TestClass->GetSuperClass();
                bool bMatchesParent = false;
                while (ParentClass)
                {
                    if (ParentClass->GetName() == ParentClassFilter ||
                        ParentClass->GetName() == TEXT("A") + ParentClassFilter ||
                        ParentClass->GetName() == ParentClassFilter.Mid(1)) // Remove 'A' prefix if present
                    {
                        bMatchesParent = true;
                        break;
                    }
                    ParentClass = ParentClass->GetSuperClass();
                }
                if (!bMatchesParent)
                {
                    continue;
                }
            }

            // Extract module name
            FString ModuleName;
            if (ClassPath.Contains(TEXT("/Script/")))
            {
                int32 ScriptIdx = ClassPath.Find(TEXT("/Script/"));
                int32 DotIdx = ClassPath.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromStart, ScriptIdx + 8);
                if (DotIdx != INDEX_NONE)
                {
                    ModuleName = ClassPath.Mid(ScriptIdx + 8, DotIdx - ScriptIdx - 8);
                }
            }

            // Get parent class name
            FString ParentClassName;
            if (TestClass->GetSuperClass())
            {
                ParentClassName = TestClass->GetSuperClass()->GetName();
            }

            // Create JSON object
            TSharedPtr<FJsonObject> ClassObj = MakeShared<FJsonObject>();
            ClassObj->SetStringField(TEXT("name"), ClassName);
            ClassObj->SetStringField(TEXT("path"), ClassPath);
            ClassObj->SetStringField(TEXT("parent"), ParentClassName);
            ClassObj->SetStringField(TEXT("module"), ModuleName);

            CppClassesArray.Add(MakeShared<FJsonValueObject>(ClassObj));
        }
    }

    // === Scan Blueprint assets ===
    if (ClassType == TEXT("all") || ClassType == TEXT("blueprint"))
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        FARFilter Filter;
        Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        Filter.bRecursivePaths = true;

        // Path filter
        if (!PathFilter.IsEmpty())
        {
            Filter.PackagePaths.Add(FName(*PathFilter));
        }
        else
        {
            Filter.PackagePaths.Add(FName(TEXT("/Game")));
        }

        TArray<FAssetData> AssetList;
        AssetRegistry.GetAssets(Filter, AssetList);

        for (const FAssetData& Asset : AssetList)
        {
            // Load Blueprint
            UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
            if (!Blueprint)
            {
                continue;
            }

            // Parent class filter
            if (!ParentClassFilter.IsEmpty() && Blueprint->ParentClass)
            {
                UClass* ParentClass = Blueprint->ParentClass;
                bool bMatchesParent = false;
                while (ParentClass)
                {
                    if (ParentClass->GetName() == ParentClassFilter ||
                        ParentClass->GetName() == TEXT("A") + ParentClassFilter ||
                        ParentClass->GetName() == ParentClassFilter.Mid(1))
                    {
                        bMatchesParent = true;
                        break;
                    }
                    ParentClass = ParentClass->GetSuperClass();
                }
                if (!bMatchesParent)
                {
                    continue;
                }
            }

            // Blueprint type filter
            if (!BlueprintTypeFilter.IsEmpty() && Blueprint->ParentClass)
            {
                bool bMatchesType = false;
                UClass* ParentClass = Blueprint->ParentClass;

                if (BlueprintTypeFilter == TEXT("actor"))
                {
                    // Actor-derived but exclude Widget/Anim/ControlRig
                    bMatchesType = ParentClass->IsChildOf(AActor::StaticClass()) &&
                                   !ParentClass->IsChildOf(UUserWidget::StaticClass());
                }
                else if (BlueprintTypeFilter == TEXT("widget"))
                {
                    bMatchesType = ParentClass->IsChildOf(UUserWidget::StaticClass());
                }
                else if (BlueprintTypeFilter == TEXT("anim"))
                {
                    bMatchesType = ParentClass->IsChildOf(UAnimInstance::StaticClass());
                }
                else if (BlueprintTypeFilter == TEXT("controlrig"))
                {
                    // ControlRig is a plugin, so check dynamically
                    static UClass* ControlRigClass = FindObject<UClass>(nullptr, TEXT("/Script/ControlRig.ControlRig"));
                    if (ControlRigClass)
                    {
                        bMatchesType = ParentClass->IsChildOf(ControlRigClass);
                    }
                }
                else if (BlueprintTypeFilter == TEXT("interface"))
                {
                    // Interface: parent is UInterface or name starts with BPI_
                    bMatchesType = ParentClass->IsChildOf(UInterface::StaticClass()) ||
                                   Asset.AssetName.ToString().StartsWith(TEXT("BPI_"));
                }
                else if (BlueprintTypeFilter == TEXT("gamemode"))
                {
                    bMatchesType = ParentClass->IsChildOf(AGameModeBase::StaticClass());
                }
                else if (BlueprintTypeFilter == TEXT("controller"))
                {
                    bMatchesType = ParentClass->IsChildOf(AController::StaticClass());
                }
                else if (BlueprintTypeFilter == TEXT("character"))
                {
                    bMatchesType = ParentClass->IsChildOf(ACharacter::StaticClass());
                }
                else if (BlueprintTypeFilter == TEXT("pawn"))
                {
                    bMatchesType = ParentClass->IsChildOf(APawn::StaticClass());
                }

                if (!bMatchesType)
                {
                    continue;
                }
            }

            // Parent class name
            FString ParentClassName;
            if (Blueprint->ParentClass)
            {
                ParentClassName = Blueprint->ParentClass->GetName();
            }

            // Create JSON object
            TSharedPtr<FJsonObject> BPObj = MakeShared<FJsonObject>();
            BPObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
            BPObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
            BPObj->SetStringField(TEXT("parent"), ParentClassName);

            BlueprintsArray.Add(MakeShared<FJsonValueObject>(BPObj));
        }
    }

    // Build result
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("cpp_classes"), CppClassesArray);
    ResultObj->SetArrayField(TEXT("blueprints"), BlueprintsArray);
    ResultObj->SetNumberField(TEXT("total_cpp"), CppClassesArray.Num());
    ResultObj->SetNumberField(TEXT("total_blueprints"), BlueprintsArray.Num());

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleDuplicateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString SourceName;
    if (!Params->TryGetStringField(TEXT("source_name"), SourceName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'source_name' parameter"));
    }

    FString NewName;
    if (!Params->TryGetStringField(TEXT("new_name"), NewName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
    }

    // Get optional parameters
    FString SourcePath = TEXT("/Game/Blueprints");
    Params->TryGetStringField(TEXT("source_path"), SourcePath);

    FString DestinationPath = SourcePath; // Default to source path
    Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

    // Build full source asset path
    FString SourceAssetPath = SourcePath + TEXT("/") + SourceName + TEXT(".") + SourceName;

    // Check if source Blueprint exists
    if (!UEditorAssetLibrary::DoesAssetExist(SourceAssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source Blueprint does not exist: %s"), *SourceAssetPath)
        );
    }

    // Build full destination asset path
    FString DestinationAssetPath = DestinationPath + TEXT("/") + NewName;

    // Check if destination already exists
    if (UEditorAssetLibrary::DoesAssetExist(DestinationAssetPath + TEXT(".") + NewName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Destination Blueprint already exists: %s"), *DestinationAssetPath)
        );
    }

    // Get AssetTools module
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    // Duplicate the asset
    UObject* SourceAsset = UEditorAssetLibrary::LoadAsset(SourceAssetPath);
    if (!SourceAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load source Blueprint: %s"), *SourceAssetPath)
        );
    }

    UObject* DuplicatedAsset = AssetTools.DuplicateAsset(NewName, DestinationPath, SourceAsset);
    if (!DuplicatedAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to duplicate Blueprint from %s to %s"), *SourceAssetPath, *DestinationAssetPath)
        );
    }

    // Build success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Blueprint duplicated successfully: %s"), *NewName));
    ResultObj->SetStringField(TEXT("source_path"), SourceAssetPath);
    ResultObj->SetStringField(TEXT("new_path"), DuplicatedAsset->GetPathName());

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleGetBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString Path = Params->GetStringField(TEXT("path"));

    if (BlueprintName.IsEmpty())
    {
        ResultJson->SetStringField(TEXT("status"), TEXT("error"));
        ResultJson->SetStringField(TEXT("error"), TEXT("blueprint_name is required"));
        return ResultJson;
    }

    // Find the Blueprint
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprintByName(BlueprintName, Path);
    if (!Blueprint)
    {
        ResultJson->SetStringField(TEXT("status"), TEXT("error"));
        ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Blueprint '%s' not found at path '%s'"), *BlueprintName, *Path));
        return ResultJson;
    }

    TSharedPtr<FJsonObject> ResultData = MakeShareable(new FJsonObject());
    ResultData->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultData->SetStringField(TEXT("parent_class"), Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None"));

    // Get Event Graph nodes
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (!Graph) continue;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;

            TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject());
            NodeObj->SetStringField(TEXT("id"), Node->NodeGuid.ToString());
            NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
            NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
            NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
            NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);

            // Get node type info
            if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
            {
                NodeObj->SetStringField(TEXT("type"), TEXT("Event"));
                NodeObj->SetStringField(TEXT("event_name"), EventNode->GetFunctionName().ToString());
            }
            else if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
            {
                NodeObj->SetStringField(TEXT("type"), TEXT("Function"));
                NodeObj->SetStringField(TEXT("function_name"), FuncNode->GetFunctionName().ToString());
            }
            else if (UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node))
            {
                NodeObj->SetStringField(TEXT("type"), TEXT("VariableGet"));
                NodeObj->SetStringField(TEXT("variable_name"), VarGetNode->GetVarName().ToString());
            }
            else if (UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node))
            {
                NodeObj->SetStringField(TEXT("type"), TEXT("VariableSet"));
                NodeObj->SetStringField(TEXT("variable_name"), VarSetNode->GetVarName().ToString());
            }
            else
            {
                NodeObj->SetStringField(TEXT("type"), TEXT("Other"));
            }

            // Get pin information
            TArray<TSharedPtr<FJsonValue>> PinsArray;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin) continue;

                TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject());
                PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
                PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));

                // Record connections
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;

                    // Only record output -> input connections to avoid duplicates
                    if (Pin->Direction == EGPD_Output)
                    {
                        TSharedPtr<FJsonObject> ConnObj = MakeShareable(new FJsonObject());
                        ConnObj->SetStringField(TEXT("source_node"), Node->NodeGuid.ToString());
                        ConnObj->SetStringField(TEXT("source_pin"), Pin->PinName.ToString());
                        ConnObj->SetStringField(TEXT("target_node"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
                        ConnObj->SetStringField(TEXT("target_pin"), LinkedPin->PinName.ToString());
                        ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnObj)));
                    }
                }
            }
            NodeObj->SetArrayField(TEXT("pins"), PinsArray);

            NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
        }
    }

    ResultData->SetArrayField(TEXT("nodes"), NodesArray);
    ResultData->SetArrayField(TEXT("connections"), ConnectionsArray);

    // Get Variables
    TArray<TSharedPtr<FJsonValue>> VariablesArray;
    for (FBPVariableDescription& Var : Blueprint->NewVariables)
    {
        TSharedPtr<FJsonObject> VarObj = MakeShareable(new FJsonObject());
        VarObj->SetStringField(TEXT("name"), Var.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
        VarObj->SetBoolField(TEXT("is_exposed"), Var.HasMetaData(FBlueprintMetadata::MD_ExposeOnSpawn));
        VariablesArray.Add(MakeShareable(new FJsonValueObject(VarObj)));
    }
    ResultData->SetArrayField(TEXT("variables"), VariablesArray);

    // Get Components
    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    if (Blueprint->SimpleConstructionScript)
    {
        TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
        for (USCS_Node* SCSNode : AllNodes)
        {
            if (!SCSNode || !SCSNode->ComponentTemplate) continue;

            TSharedPtr<FJsonObject> CompObj = MakeShareable(new FJsonObject());
            CompObj->SetStringField(TEXT("name"), SCSNode->GetVariableName().ToString());
            CompObj->SetStringField(TEXT("class"), SCSNode->ComponentTemplate->GetClass()->GetName());
            ComponentsArray.Add(MakeShareable(new FJsonValueObject(CompObj)));
        }
    }
    ResultData->SetArrayField(TEXT("components"), ComponentsArray);

    ResultJson->SetStringField(TEXT("status"), TEXT("success"));
    ResultJson->SetObjectField(TEXT("result"), ResultData);

    return ResultJson;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetBlueprintClassArray(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString PropertyName = Params->GetStringField(TEXT("property_name"));
    const TArray<TSharedPtr<FJsonValue>>* ClassPathsArray = nullptr;
    if (!Params->TryGetArrayField(TEXT("class_paths"), ClassPathsArray))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: class_paths"));
    }

    FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/Blueprints");

    // Load Blueprint
    FString BlueprintPath = Path + TEXT("/") + BlueprintName + TEXT(".") + BlueprintName;
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    // Get CDO
    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get CDO"));
    }

    // Find property
    FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(BPClass, *PropertyName);
    if (!ArrayProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Property not found: %s"), *PropertyName));
    }

    // Check if inner property is a class property
    FClassProperty* ClassProp = CastField<FClassProperty>(ArrayProp->Inner);
    if (!ClassProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Property %s is not a TSubclassOf array"), *PropertyName));
    }

    // Get array helper
    void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(CDO);
    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

    // Clear existing array
    ArrayHelper.EmptyValues();

    // Add new classes
    int32 AddedCount = 0;
    for (const TSharedPtr<FJsonValue>& ClassPathValue : *ClassPathsArray)
    {
        FString ClassPath = ClassPathValue->AsString();

        // Load the class
        UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
        if (!LoadedClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to load class: %s"), *ClassPath);
            continue;
        }

        // Add to array
        int32 NewIndex = ArrayHelper.AddValue();
        UClass** ElementPtr = reinterpret_cast<UClass**>(ArrayHelper.GetRawPtr(NewIndex));
        *ElementPtr = LoadedClass;
        AddedCount++;
    }

    // Mark modified and compile
    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Create success response
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetBoolField(TEXT("success"), true);
    ResultJson->SetStringField(TEXT("property_name"), PropertyName);
    ResultJson->SetNumberField(TEXT("count"), AddedCount);

    return ResultJson;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetStructArrayProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString PropertyName = Params->GetStringField(TEXT("property_name"));

    const TArray<TSharedPtr<FJsonValue>>* ValuesArray = nullptr;
    if (!Params->TryGetArrayField(TEXT("values"), ValuesArray))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: values"));
    }

    FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/Blueprints");

    // Load Blueprint
    FString BlueprintPath = Path + TEXT("/") + BlueprintName + TEXT(".") + BlueprintName;
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    // Get CDO
    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get CDO"));
    }

    // Find array property
    FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(BPClass, *PropertyName);
    if (!ArrayProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Property not found: %s"), *PropertyName));
    }

    // Verify inner property is a struct
    FStructProperty* StructProp = CastField<FStructProperty>(ArrayProp->Inner);
    if (!StructProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Property %s is not a struct array"), *PropertyName));
    }

    UScriptStruct* StructType = StructProp->Struct;
    UE_LOG(LogTemp, Log, TEXT("Setting struct array property %s of type %s"), *PropertyName, *StructType->GetName());

    // Get array helper
    void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(CDO);
    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

    // Clear existing array and resize
    ArrayHelper.EmptyValues();
    ArrayHelper.Resize(ValuesArray->Num());

    int32 SetCount = 0;
    TArray<FString> Errors;

    // Set each element
    for (int32 i = 0; i < ValuesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& ElementValue = (*ValuesArray)[i];
        if (ElementValue->Type != EJson::Object)
        {
            Errors.Add(FString::Printf(TEXT("Element %d is not an object"), i));
            continue;
        }

        const TSharedPtr<FJsonObject>& ElementObj = ElementValue->AsObject();
        void* ElementPtr = ArrayHelper.GetRawPtr(i);

        // Set each field of the struct
        for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
        {
            FProperty* FieldProp = *PropIt;
            FString FieldName = FieldProp->GetName();

            if (!ElementObj->HasField(FieldName))
            {
                continue; // Skip fields not provided in JSON
            }

            TSharedPtr<FJsonValue> FieldValue = ElementObj->TryGetField(FieldName);
            if (!FieldValue.IsValid())
            {
                continue;
            }

            void* FieldPtr = FieldProp->ContainerPtrToValuePtr<void>(ElementPtr);

            // Handle different field types
            if (FClassProperty* ClassFieldProp = CastField<FClassProperty>(FieldProp))
            {
                // TSubclassOf field
                FString ClassPath = FieldValue->AsString();
                UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
                if (LoadedClass)
                {
                    ClassFieldProp->SetObjectPropertyValue(FieldPtr, LoadedClass);
                    UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, *LoadedClass->GetName());
                }
                else
                {
                    Errors.Add(FString::Printf(TEXT("Element %d: Failed to load class for %s: %s"), i, *FieldName, *ClassPath));
                }
            }
            else if (FIntProperty* IntFieldProp = CastField<FIntProperty>(FieldProp))
            {
                int32 IntValue = static_cast<int32>(FieldValue->AsNumber());
                IntFieldProp->SetPropertyValue(FieldPtr, IntValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %d"), i, *FieldName, IntValue);
            }
            else if (FFloatProperty* FloatFieldProp = CastField<FFloatProperty>(FieldProp))
            {
                float FloatValue = static_cast<float>(FieldValue->AsNumber());
                FloatFieldProp->SetPropertyValue(FieldPtr, FloatValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %f"), i, *FieldName, FloatValue);
            }
            else if (FDoubleProperty* DoubleFieldProp = CastField<FDoubleProperty>(FieldProp))
            {
                double DoubleValue = FieldValue->AsNumber();
                DoubleFieldProp->SetPropertyValue(FieldPtr, DoubleValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %f"), i, *FieldName, DoubleValue);
            }
            else if (FBoolProperty* BoolFieldProp = CastField<FBoolProperty>(FieldProp))
            {
                bool BoolValue = FieldValue->AsBool();
                BoolFieldProp->SetPropertyValue(FieldPtr, BoolValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, BoolValue ? TEXT("true") : TEXT("false"));
            }
            else if (FStrProperty* StrFieldProp = CastField<FStrProperty>(FieldProp))
            {
                FString StrValue = FieldValue->AsString();
                StrFieldProp->SetPropertyValue(FieldPtr, StrValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, *StrValue);
            }
            else if (FNameProperty* NameFieldProp = CastField<FNameProperty>(FieldProp))
            {
                FName NameValue = FName(*FieldValue->AsString());
                NameFieldProp->SetPropertyValue(FieldPtr, NameValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, *NameValue.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("  [%d] Unsupported field type for %s: %s"),
                    i, *FieldName, *FieldProp->GetClass()->GetName());
            }
        }

        SetCount++;
    }

    // Mark modified and compile
    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Create response
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetBoolField(TEXT("success"), Errors.Num() == 0);
    ResultJson->SetStringField(TEXT("property_name"), PropertyName);
    ResultJson->SetStringField(TEXT("struct_type"), StructType->GetName());
    ResultJson->SetNumberField(TEXT("count"), SetCount);

    if (Errors.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrorsArray;
        for (const FString& Error : Errors)
        {
            ErrorsArray.Add(MakeShareable(new FJsonValueString(Error)));
        }
        ResultJson->SetArrayField(TEXT("errors"), ErrorsArray);
    }

    return ResultJson;
} 