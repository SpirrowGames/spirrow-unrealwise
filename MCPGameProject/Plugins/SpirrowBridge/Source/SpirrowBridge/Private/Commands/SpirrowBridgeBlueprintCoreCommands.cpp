#include "Commands/SpirrowBridgeBlueprintCoreCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Misc/App.h"
#include "UObject/UObjectIterator.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

FSpirrowBridgeBlueprintCoreCommands::FSpirrowBridgeBlueprintCoreCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCoreCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_blueprint"))
    {
        return HandleCreateBlueprint(Params);
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
    else if (CommandType == TEXT("duplicate_blueprint"))
    {
        return HandleDuplicateBlueprint(Params);
    }
    else if (CommandType == TEXT("get_blueprint_graph"))
    {
        return HandleGetBlueprintGraph(Params);
    }

    return nullptr; // Not handled by this class
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCoreCommands::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), BlueprintName))
    {
        return Error;
    }

    // Get optional parameters
    FString Path, ParentClass;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("parent_class"), ParentClass, TEXT(""));

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
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("name"), BlueprintName);
        Details->SetStringField(TEXT("path"), PackagePath + AssetName);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Blueprint already exists: %s"), *BlueprintName),
            Details);
    }

    // Create the blueprint factory
    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    
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
        }
        else if (ClassName == TEXT("Actor") || ClassName == TEXT("AActor"))
        {
            FoundClass = AActor::StaticClass();
        }
        else if (ClassName == TEXT("Character") || ClassName == TEXT("ACharacter"))
        {
            FoundClass = ACharacter::StaticClass();
        }
        else if (ClassName == TEXT("UserWidget") || ClassName == TEXT("UUserWidget"))
        {
            FoundClass = UUserWidget::StaticClass();
        }

        // Method 2: Search through all loaded classes using TObjectIterator
        if (!FoundClass)
        {
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (Class->GetName() == ClassName)
                {
                    FoundClass = Class;
                    break;
                }
            }
        }

        // Method 2b: Try with 'A' prefix for Actor classes
        if (!FoundClass && !ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
        {
            FString ClassNameWithA = TEXT("A") + ClassName;
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (Class->GetName() == ClassNameWithA)
                {
                    FoundClass = Class;
                    break;
                }
            }
        }

        // Method 2c: Try with 'U' prefix for UObject classes
        if (!FoundClass && !ClassName.StartsWith(TEXT("U")))
        {
            FString ClassNameWithU = TEXT("U") + ClassName;
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (Class->GetName() == ClassNameWithU)
                {
                    FoundClass = Class;
                    break;
                }
            }
        }

        // Method 3: Try LoadObject with various module paths
        if (!FoundClass)
        {
            FString ProjectModuleName = FApp::GetProjectName();
            TArray<FString> ModulePaths;
            ModulePaths.Add(FString::Printf(TEXT("/Script/Engine.%s"), *ClassName));
            ModulePaths.Add(FString::Printf(TEXT("/Script/UMG.%s"), *ClassName));
            ModulePaths.Add(FString::Printf(TEXT("/Script/%s.%s"), *ProjectModuleName, *ClassName));

            for (const FString& ModulePath : ModulePaths)
            {
                FoundClass = LoadObject<UClass>(nullptr, *ModulePath);
                if (FoundClass)
                {
                    break;
                }
            }
        }

        if (FoundClass)
        {
            SelectedParentClass = FoundClass;
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
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("name"), AssetName);
            ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
            ResultObj->SetStringField(TEXT("type"), TEXT("WidgetBlueprint"));
            return ResultObj;
        }
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create Widget Blueprint"));
    }
    else
    {
        // Create regular Blueprint (Actor-based)
        Factory->ParentClass = SelectedParentClass;

        UPackage* Package = CreatePackage(*(PackagePath + AssetName));
        UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn));

        if (NewBlueprint)
        {
            FAssetRegistryModule::AssetCreated(NewBlueprint);
            Package->MarkPackageDirty();

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("name"), AssetName);
            ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
            ResultObj->SetStringField(TEXT("type"), TEXT("Blueprint"));
            return ResultObj;
        }

        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create blueprint"));
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCoreCommands::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
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

    // Compile the blueprint
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    if (!Blueprint->GeneratedClass)
    {
        FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None);
    }
    else
    {
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("name"), BlueprintName);
    ResultObj->SetStringField(TEXT("path"), Path + TEXT("/") + BlueprintName);
    ResultObj->SetBoolField(TEXT("compiled"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCoreCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, ActorName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("actor_name"), ActorName))
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
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::UnknownError,
            TEXT("Failed to get editor world"));
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

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::ActorSpawnFailed,
        FString::Printf(TEXT("Failed to spawn blueprint actor '%s'"), *BlueprintName));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCoreCommands::HandleSetBlueprintProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, PropertyName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
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

    // Get the default object
    if (!Blueprint->GeneratedClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidBlueprint,
            TEXT("Blueprint has no GeneratedClass. Try compiling first."));
    }
    UObject* DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
    if (!DefaultObject)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertyNotFound,
            TEXT("Failed to get default object"));
    }

    // Set the property value
    if (Params->HasField(TEXT("property_value")))
    {
        TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));
        
        FString ErrorMessage;
        if (FSpirrowBridgeCommonUtils::SetObjectProperty(DefaultObject, PropertyName, JsonValue, ErrorMessage))
        {
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetBoolField(TEXT("success"), true);
            ResultObj->SetStringField(TEXT("property"), PropertyName);
            return ResultObj;
        }
        else
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::PropertySetFailed,
                ErrorMessage);
        }
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::MissingRequiredParam,
        TEXT("Missing 'property_value' parameter"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCoreCommands::HandleDuplicateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString SourceName, NewName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("source_name"), SourceName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("new_name"), NewName))
    {
        return Error;
    }

    // Get optional parameters
    FString SourcePath, DestinationPath;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("source_path"), SourcePath, TEXT("/Game/Blueprints"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("destination_path"), DestinationPath, SourcePath);

    // Build full source asset path
    FString SourceAssetPath = SourcePath + TEXT("/") + SourceName + TEXT(".") + SourceName;

    // Check if source Blueprint exists
    if (!UEditorAssetLibrary::DoesAssetExist(SourceAssetPath))
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("source_name"), SourceName);
        Details->SetStringField(TEXT("source_path"), SourcePath);
        Details->SetStringField(TEXT("full_path"), SourceAssetPath);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::BlueprintNotFound,
            FString::Printf(TEXT("Source Blueprint does not exist: %s"), *SourceAssetPath),
            Details);
    }

    // Build full destination asset path
    FString DestinationAssetPath = DestinationPath + TEXT("/") + NewName;

    // Check if destination already exists
    if (UEditorAssetLibrary::DoesAssetExist(DestinationAssetPath + TEXT(".") + NewName))
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("new_name"), NewName);
        Details->SetStringField(TEXT("destination_path"), DestinationPath);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Destination Blueprint already exists: %s"), *DestinationAssetPath),
            Details);
    }

    // Get AssetTools module
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    // Duplicate the asset
    UObject* SourceAsset = UEditorAssetLibrary::LoadAsset(SourceAssetPath);
    if (!SourceAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetLoadFailed,
            FString::Printf(TEXT("Failed to load source Blueprint: %s"), *SourceAssetPath));
    }

    UObject* DuplicatedAsset = AssetTools.DuplicateAsset(NewName, DestinationPath, SourceAsset);
    if (!DuplicatedAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            FString::Printf(TEXT("Failed to duplicate Blueprint from %s to %s"), *SourceAssetPath, *DestinationAssetPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Blueprint duplicated successfully: %s"), *NewName));
    ResultObj->SetStringField(TEXT("source_path"), SourceAssetPath);
    ResultObj->SetStringField(TEXT("new_path"), DuplicatedAsset->GetPathName());

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCoreCommands::HandleGetBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
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

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetBoolField(TEXT("success"), true);
    ResultJson->SetObjectField(TEXT("result"), ResultData);

    return ResultJson;
}
