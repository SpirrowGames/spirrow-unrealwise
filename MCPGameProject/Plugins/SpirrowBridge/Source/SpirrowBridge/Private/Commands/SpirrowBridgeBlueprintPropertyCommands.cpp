#include "Commands/SpirrowBridgeBlueprintPropertyCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DataAsset.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Controller.h"
#include "Blueprint/UserWidget.h"
#include "Animation/AnimInstance.h"
#include "UObject/UObjectIterator.h"
#include "UObject/SavePackage.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"

FSpirrowBridgeBlueprintPropertyCommands::FSpirrowBridgeBlueprintPropertyCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("scan_project_classes"))
    {
        return HandleScanProjectClasses(Params);
    }
    else if (CommandType == TEXT("set_blueprint_class_array"))
    {
        return HandleSetBlueprintClassArray(Params);
    }
    else if (CommandType == TEXT("set_struct_array_property"))
    {
        return HandleSetStructArrayProperty(Params);
    }
    // New property commands (v0.8.8)
    else if (CommandType == TEXT("create_data_asset"))
    {
        return HandleCreateDataAsset(Params);
    }
    else if (CommandType == TEXT("set_class_property"))
    {
        return HandleSetClassProperty(Params);
    }
    else if (CommandType == TEXT("set_object_property"))
    {
        return HandleSetObjectProperty(Params);
    }
    else if (CommandType == TEXT("get_blueprint_properties"))
    {
        return HandleGetBlueprintProperties(Params);
    }
    else if (CommandType == TEXT("set_struct_property"))
    {
        return HandleSetStructProperty(Params);
    }
    else if (CommandType == TEXT("set_data_asset_property"))
    {
        return HandleSetDataAssetProperty(Params);
    }
    else if (CommandType == TEXT("get_data_asset_properties"))
    {
        return HandleGetDataAssetProperties(Params);
    }
    else if (CommandType == TEXT("batch_set_properties"))
    {
        return HandleBatchSetProperties(Params);
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleScanProjectClasses(const TSharedPtr<FJsonObject>& Params)
{
    // Get optional parameters
    FString ClassType, ParentClassFilter, ModuleFilter, PathFilter, BlueprintTypeFilter;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("class_type"), ClassType, TEXT("all"));
    Params->TryGetStringField(TEXT("parent_class"), ParentClassFilter);
    Params->TryGetStringField(TEXT("module_filter"), ModuleFilter);
    Params->TryGetStringField(TEXT("path_filter"), PathFilter);
    Params->TryGetStringField(TEXT("blueprint_type"), BlueprintTypeFilter);

    bool bIncludeEngine, bExcludeReinst;
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("include_engine"), bIncludeEngine, false);
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("exclude_reinst"), bExcludeReinst, true);

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

            if (ClassName.EndsWith(TEXT("_C")))
            {
                continue;
            }

            if (bExcludeReinst)
            {
                if (ClassName.StartsWith(TEXT("REINST_")) || ClassPath.Contains(TEXT("/Engine/Transient.")))
                {
                    continue;
                }
            }

            if (!ModuleFilter.IsEmpty())
            {
                if (!ClassPath.Contains(FString::Printf(TEXT("/Script/%s."), *ModuleFilter)))
                {
                    continue;
                }
            }

            if (!ParentClassFilter.IsEmpty())
            {
                UClass* ParentClass = TestClass->GetSuperClass();
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
                if (!bMatchesParent) continue;
            }

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

            FString ParentClassName;
            if (TestClass->GetSuperClass())
            {
                ParentClassName = TestClass->GetSuperClass()->GetName();
            }

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
            UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
            if (!Blueprint) continue;

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
                if (!bMatchesParent) continue;
            }

            if (!BlueprintTypeFilter.IsEmpty() && Blueprint->ParentClass)
            {
                bool bMatchesType = false;
                UClass* ParentClass = Blueprint->ParentClass;

                if (BlueprintTypeFilter == TEXT("actor"))
                {
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
                    static UClass* ControlRigClass = FindObject<UClass>(nullptr, TEXT("/Script/ControlRig.ControlRig"));
                    if (ControlRigClass)
                    {
                        bMatchesType = ParentClass->IsChildOf(ControlRigClass);
                    }
                }
                else if (BlueprintTypeFilter == TEXT("interface"))
                {
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

                if (!bMatchesType) continue;
            }

            FString ParentClassName;
            if (Blueprint->ParentClass)
            {
                ParentClassName = Blueprint->ParentClass->GetName();
            }

            TSharedPtr<FJsonObject> BPObj = MakeShared<FJsonObject>();
            BPObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
            BPObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
            BPObj->SetStringField(TEXT("parent"), ParentClassName);

            BlueprintsArray.Add(MakeShared<FJsonValueObject>(BPObj));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("cpp_classes"), CppClassesArray);
    ResultObj->SetArrayField(TEXT("blueprints"), BlueprintsArray);
    ResultObj->SetNumberField(TEXT("total_cpp"), CppClassesArray.Num());
    ResultObj->SetNumberField(TEXT("total_blueprints"), BlueprintsArray.Num());

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetBlueprintClassArray(const TSharedPtr<FJsonObject>& Params)
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

    const TArray<TSharedPtr<FJsonValue>>* ClassPathsArray = nullptr;
    if (!Params->TryGetArrayField(TEXT("class_paths"), ClassPathsArray))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: class_paths"));
    }

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    // Get CDO
    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Failed to get CDO"));
    }

    // Find property
    FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(BPClass, *PropertyName);
    if (!ArrayProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertyNotFound,
            FString::Printf(TEXT("Property not found: %s"), *PropertyName));
    }

    FClassProperty* ClassProp = CastField<FClassProperty>(ArrayProp->Inner);
    if (!ClassProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParameter,
            FString::Printf(TEXT("Property %s is not a TSubclassOf array"), *PropertyName));
    }

    void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(CDO);
    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

    ArrayHelper.EmptyValues();

    int32 AddedCount = 0;
    for (const TSharedPtr<FJsonValue>& ClassPathValue : *ClassPathsArray)
    {
        FString ClassPath = ClassPathValue->AsString();
        UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
        if (!LoadedClass) continue;

        int32 NewIndex = ArrayHelper.AddValue();
        UClass** ElementPtr = reinterpret_cast<UClass**>(ArrayHelper.GetRawPtr(NewIndex));
        *ElementPtr = LoadedClass;
        AddedCount++;
    }

    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetBoolField(TEXT("success"), true);
    ResultJson->SetStringField(TEXT("property_name"), PropertyName);
    ResultJson->SetNumberField(TEXT("count"), AddedCount);

    return ResultJson;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetStructArrayProperty(const TSharedPtr<FJsonObject>& Params)
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

    const TArray<TSharedPtr<FJsonValue>>* ValuesArray = nullptr;
    if (!Params->TryGetArrayField(TEXT("values"), ValuesArray))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: values"));
    }

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Failed to get CDO"));
    }

    FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(BPClass, *PropertyName);
    if (!ArrayProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertyNotFound,
            FString::Printf(TEXT("Property not found: %s"), *PropertyName));
    }

    FStructProperty* StructProp = CastField<FStructProperty>(ArrayProp->Inner);
    if (!StructProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParameter,
            FString::Printf(TEXT("Property %s is not a struct array"), *PropertyName));
    }

    UScriptStruct* StructType = StructProp->Struct;

    void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(CDO);
    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

    ArrayHelper.EmptyValues();
    ArrayHelper.Resize(ValuesArray->Num());

    int32 SetCount = 0;
    TArray<FString> Errors;

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

        for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
        {
            FProperty* FieldProp = *PropIt;
            FString FieldName = FieldProp->GetName();

            if (!ElementObj->HasField(FieldName)) continue;

            TSharedPtr<FJsonValue> FieldValue = ElementObj->TryGetField(FieldName);
            if (!FieldValue.IsValid()) continue;

            void* FieldPtr = FieldProp->ContainerPtrToValuePtr<void>(ElementPtr);

            if (FClassProperty* ClassFieldProp = CastField<FClassProperty>(FieldProp))
            {
                FString ClassPath = FieldValue->AsString();
                UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
                if (LoadedClass)
                {
                    ClassFieldProp->SetObjectPropertyValue(FieldPtr, LoadedClass);
                }
                else
                {
                    Errors.Add(FString::Printf(TEXT("Element %d: Failed to load class for %s: %s"), i, *FieldName, *ClassPath));
                }
            }
            else if (FIntProperty* IntFieldProp = CastField<FIntProperty>(FieldProp))
            {
                IntFieldProp->SetPropertyValue(FieldPtr, static_cast<int32>(FieldValue->AsNumber()));
            }
            else if (FFloatProperty* FloatFieldProp = CastField<FFloatProperty>(FieldProp))
            {
                FloatFieldProp->SetPropertyValue(FieldPtr, static_cast<float>(FieldValue->AsNumber()));
            }
            else if (FDoubleProperty* DoubleFieldProp = CastField<FDoubleProperty>(FieldProp))
            {
                DoubleFieldProp->SetPropertyValue(FieldPtr, FieldValue->AsNumber());
            }
            else if (FBoolProperty* BoolFieldProp = CastField<FBoolProperty>(FieldProp))
            {
                BoolFieldProp->SetPropertyValue(FieldPtr, FieldValue->AsBool());
            }
            else if (FStrProperty* StrFieldProp = CastField<FStrProperty>(FieldProp))
            {
                StrFieldProp->SetPropertyValue(FieldPtr, FieldValue->AsString());
            }
            else if (FNameProperty* NameFieldProp = CastField<FNameProperty>(FieldProp))
            {
                NameFieldProp->SetPropertyValue(FieldPtr, FName(*FieldValue->AsString()));
            }
        }

        SetCount++;
    }

    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

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

// ===== New Property Commands (v0.8.8) =====

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleCreateDataAsset(
    const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString Name, ParentClassName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("parent_class"), ParentClassName))
    {
        return Error;
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Data"));

    // Find the parent class
    UClass* ParentClass = nullptr;

    // Method 1: Try direct load (e.g., "/Script/TrapxTrapCpp.UFirearmData")
    ParentClass = LoadClass<UDataAsset>(nullptr, *ParentClassName);

    // Method 2: Try with /Script/Engine prefix (e.g., "PrimaryDataAsset" -> "/Script/Engine.UPrimaryDataAsset")
    if (!ParentClass && !ParentClassName.Contains(TEXT("/")))
    {
        // Try Engine module
        ParentClass = LoadClass<UDataAsset>(nullptr,
            *FString::Printf(TEXT("/Script/Engine.U%s"), *ParentClassName));

        // Try without U prefix
        if (!ParentClass)
        {
            ParentClass = LoadClass<UDataAsset>(nullptr,
                *FString::Printf(TEXT("/Script/Engine.%s"), *ParentClassName));
        }
    }

    // Method 3: Try TObjectIterator
    if (!ParentClass)
    {
        FString SearchName = ParentClassName;
        if (!SearchName.StartsWith(TEXT("U")))
        {
            SearchName = TEXT("U") + ParentClassName;
        }

        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* TestClass = *It;
            if (TestClass->IsChildOf(UDataAsset::StaticClass()))
            {
                if (TestClass->GetName() == SearchName || TestClass->GetName() == ParentClassName)
                {
                    ParentClass = TestClass;
                    break;
                }
            }
        }
    }

    if (!ParentClass || !ParentClass->IsChildOf(UDataAsset::StaticClass()))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ClassNotFound,
            FString::Printf(TEXT("DataAsset class not found or invalid: %s"), *ParentClassName));
    }

    // Create package
    FString PackagePath = Path / Name;
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package"));
    }

    // Create DataAsset
    UDataAsset* DataAsset = NewObject<UDataAsset>(Package, ParentClass, *Name, RF_Public | RF_Standalone);
    if (!DataAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create DataAsset"));
    }

    // Set initial values if provided
    const TSharedPtr<FJsonObject>* InitialValues = nullptr;
    if (Params->TryGetObjectField(TEXT("initial_values"), InitialValues))
    {
        for (const auto& Pair : (*InitialValues)->Values)
        {
            FString ErrorMsg;
            FSpirrowBridgeCommonUtils::SetObjectProperty(DataAsset, Pair.Key, Pair.Value, ErrorMsg);
        }
    }

    // Save
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(DataAsset);

    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, DataAsset, *PackageFileName, SaveArgs);

    // Response
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("asset_path"), PackagePath);
    Result->SetStringField(TEXT("name"), Name);
    Result->SetStringField(TEXT("parent_class"), ParentClass->GetName());
    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetClassProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, PropertyName, ClassPath;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("class_path"), ClassPath))
    {
        return Error;
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Failed to get CDO"));
    }

    // Use SetObjectProperty which handles FClassProperty
    FString ErrorMsg;
    TSharedPtr<FJsonValue> Value = MakeShareable(new FJsonValueString(ClassPath));

    if (!FSpirrowBridgeCommonUtils::SetObjectProperty(CDO, PropertyName, Value, ErrorMsg))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertySetFailed, ErrorMsg);
    }

    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("blueprint_name"), BlueprintName);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetStringField(TEXT("class_path"), ClassPath);
    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetObjectProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, PropertyName, AssetPath;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_path"), AssetPath))
    {
        return Error;
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Failed to get CDO"));
    }

    // Use SetObjectProperty which handles FObjectProperty
    FString ErrorMsg;
    TSharedPtr<FJsonValue> Value = MakeShareable(new FJsonValueString(AssetPath));

    if (!FSpirrowBridgeCommonUtils::SetObjectProperty(CDO, PropertyName, Value, ErrorMsg))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertySetFailed, ErrorMsg);
    }

    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("blueprint_name"), BlueprintName);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetStringField(TEXT("asset_path"), AssetPath);
    return Result;
}

FString FSpirrowBridgeBlueprintPropertyCommands::GetPropertyTypeName(FProperty* Property)
{
    if (!Property) return TEXT("Unknown");

    if (Property->IsA<FBoolProperty>()) return TEXT("Bool");
    if (Property->IsA<FIntProperty>()) return TEXT("Int32");
    if (Property->IsA<FInt64Property>()) return TEXT("Int64");
    if (Property->IsA<FFloatProperty>()) return TEXT("Float");
    if (Property->IsA<FDoubleProperty>()) return TEXT("Double");
    if (Property->IsA<FStrProperty>()) return TEXT("String");
    if (Property->IsA<FNameProperty>()) return TEXT("Name");
    if (Property->IsA<FTextProperty>()) return TEXT("Text");

    if (FClassProperty* CP = CastField<FClassProperty>(Property))
    {
        return FString::Printf(TEXT("TSubclassOf<%s>"),
            CP->MetaClass ? *CP->MetaClass->GetName() : TEXT("UObject"));
    }

    if (FSoftClassProperty* SCP = CastField<FSoftClassProperty>(Property))
    {
        return FString::Printf(TEXT("TSoftClassPtr<%s>"),
            SCP->MetaClass ? *SCP->MetaClass->GetName() : TEXT("UObject"));
    }

    if (FObjectProperty* OP = CastField<FObjectProperty>(Property))
    {
        return FString::Printf(TEXT("TObjectPtr<%s>"),
            OP->PropertyClass ? *OP->PropertyClass->GetName() : TEXT("UObject"));
    }

    if (FSoftObjectProperty* SOP = CastField<FSoftObjectProperty>(Property))
    {
        return FString::Printf(TEXT("TSoftObjectPtr<%s>"),
            SOP->PropertyClass ? *SOP->PropertyClass->GetName() : TEXT("UObject"));
    }

    if (FArrayProperty* AP = CastField<FArrayProperty>(Property))
    {
        return FString::Printf(TEXT("TArray<%s>"), *GetPropertyTypeName(AP->Inner));
    }

    if (FMapProperty* MP = CastField<FMapProperty>(Property))
    {
        return FString::Printf(TEXT("TMap<%s, %s>"),
            *GetPropertyTypeName(MP->KeyProp), *GetPropertyTypeName(MP->ValueProp));
    }

    if (FSetProperty* SP = CastField<FSetProperty>(Property))
    {
        return FString::Printf(TEXT("TSet<%s>"), *GetPropertyTypeName(SP->ElementProp));
    }

    if (FStructProperty* StructP = CastField<FStructProperty>(Property))
    {
        return StructP->Struct ? StructP->Struct->GetName() : TEXT("Struct");
    }

    if (FEnumProperty* EP = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EP->GetEnum();
        return Enum ? Enum->GetName() : TEXT("Enum");
    }

    if (FByteProperty* BP = CastField<FByteProperty>(Property))
    {
        if (BP->Enum)
        {
            return BP->Enum->GetName();
        }
        return TEXT("Byte");
    }

    return Property->GetClass()->GetName();
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleGetBlueprintProperties(
    const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }

    FString Path, CategoryFilter;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("category_filter"), CategoryFilter, TEXT(""));

    bool bIncludeInherited;
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("include_inherited"), bIncludeInherited, true);

    // Load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();

    TArray<TSharedPtr<FJsonValue>> PropertiesArray;

    for (TFieldIterator<FProperty> PropIt(BPClass); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;

        // Skip inherited if not wanted
        if (!bIncludeInherited && Property->GetOwnerClass() != BPClass)
        {
            continue;
        }

        // Skip non-editable properties
        if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
        {
            continue;
        }

        // Category filter
        FString Category = Property->GetMetaData(TEXT("Category"));
        if (!CategoryFilter.IsEmpty() && !Category.Contains(CategoryFilter))
        {
            continue;
        }

        TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
        PropObj->SetStringField(TEXT("name"), Property->GetName());
        PropObj->SetStringField(TEXT("type"), GetPropertyTypeName(Property));
        PropObj->SetStringField(TEXT("cpp_type"), Property->GetCPPType());
        PropObj->SetStringField(TEXT("category"), Category);
        PropObj->SetBoolField(TEXT("is_editable"), Property->HasAnyPropertyFlags(CPF_Edit));
        PropObj->SetBoolField(TEXT("is_blueprint_visible"), Property->HasAnyPropertyFlags(CPF_BlueprintVisible));
        PropObj->SetBoolField(TEXT("is_blueprint_read_only"), Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly));

        // Get default value as string
        if (CDO)
        {
            FString DefaultValue;
            Property->ExportTextItem_Direct(DefaultValue,
                Property->ContainerPtrToValuePtr<void>(CDO), nullptr, CDO, PPF_None);
            PropObj->SetStringField(TEXT("default_value"), DefaultValue);
        }

        // Get owner class name
        PropObj->SetStringField(TEXT("owner_class"), Property->GetOwnerClass()->GetName());

        PropertiesArray.Add(MakeShared<FJsonValueObject>(PropObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("blueprint_name"), BlueprintName);
    Result->SetArrayField(TEXT("properties"), PropertiesArray);
    Result->SetNumberField(TEXT("total"), PropertiesArray.Num());
    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetStructProperty(
    const TSharedPtr<FJsonObject>& Params)
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

    double IndexDouble;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredNumber(Params, TEXT("index"), IndexDouble))
    {
        return Error;
    }
    int32 Index = static_cast<int32>(IndexDouble);

    const TSharedPtr<FJsonObject>* ValuesObj = nullptr;
    if (!Params->TryGetObjectField(TEXT("values"), ValuesObj))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: values"));
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Load Blueprint
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, Path, Blueprint))
    {
        return Error;
    }

    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Failed to get CDO"));
    }

    // Find array property
    FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(BPClass, *PropertyName);
    if (!ArrayProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertyNotFound,
            FString::Printf(TEXT("Array property not found: %s"), *PropertyName));
    }

    FStructProperty* StructProp = CastField<FStructProperty>(ArrayProp->Inner);
    if (!StructProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParameter,
            FString::Printf(TEXT("Property %s is not a struct array"), *PropertyName));
    }

    void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(CDO);
    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

    if (Index < 0 || Index >= ArrayHelper.Num())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParamValue,
            FString::Printf(TEXT("Index %d out of range (array size: %d)"), Index, ArrayHelper.Num()));
    }

    void* ElementPtr = ArrayHelper.GetRawPtr(Index);
    UScriptStruct* StructType = StructProp->Struct;

    // Update only specified fields
    TArray<FString> UpdatedFields;
    TArray<FString> Errors;

    for (const auto& Pair : (*ValuesObj)->Values)
    {
        FString FieldName = Pair.Key;
        TSharedPtr<FJsonValue> FieldValue = Pair.Value;

        // Find field in struct
        FProperty* FieldProp = nullptr;
        for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
        {
            if ((*PropIt)->GetName() == FieldName)
            {
                FieldProp = *PropIt;
                break;
            }
        }

        if (!FieldProp)
        {
            Errors.Add(FString::Printf(TEXT("Field not found: %s"), *FieldName));
            continue;
        }

        void* FieldPtr = FieldProp->ContainerPtrToValuePtr<void>(ElementPtr);

        // Handle different field types
        if (FClassProperty* ClassFieldProp = CastField<FClassProperty>(FieldProp))
        {
            FString ClassPath = FieldValue->AsString();
            UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
            if (LoadedClass)
            {
                ClassFieldProp->SetObjectPropertyValue(FieldPtr, LoadedClass);
                UpdatedFields.Add(FieldName);
            }
            else
            {
                Errors.Add(FString::Printf(TEXT("Failed to load class for %s: %s"), *FieldName, *ClassPath));
            }
        }
        else if (FObjectProperty* ObjectFieldProp = CastField<FObjectProperty>(FieldProp))
        {
            FString AssetPath = FieldValue->AsString();
            UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
            if (LoadedAsset)
            {
                ObjectFieldProp->SetObjectPropertyValue(FieldPtr, LoadedAsset);
                UpdatedFields.Add(FieldName);
            }
            else
            {
                Errors.Add(FString::Printf(TEXT("Failed to load asset for %s: %s"), *FieldName, *AssetPath));
            }
        }
        else if (FIntProperty* IntFieldProp = CastField<FIntProperty>(FieldProp))
        {
            IntFieldProp->SetPropertyValue(FieldPtr, static_cast<int32>(FieldValue->AsNumber()));
            UpdatedFields.Add(FieldName);
        }
        else if (FFloatProperty* FloatFieldProp = CastField<FFloatProperty>(FieldProp))
        {
            FloatFieldProp->SetPropertyValue(FieldPtr, static_cast<float>(FieldValue->AsNumber()));
            UpdatedFields.Add(FieldName);
        }
        else if (FDoubleProperty* DoubleFieldProp = CastField<FDoubleProperty>(FieldProp))
        {
            DoubleFieldProp->SetPropertyValue(FieldPtr, FieldValue->AsNumber());
            UpdatedFields.Add(FieldName);
        }
        else if (FBoolProperty* BoolFieldProp = CastField<FBoolProperty>(FieldProp))
        {
            BoolFieldProp->SetPropertyValue(FieldPtr, FieldValue->AsBool());
            UpdatedFields.Add(FieldName);
        }
        else if (FStrProperty* StrFieldProp = CastField<FStrProperty>(FieldProp))
        {
            StrFieldProp->SetPropertyValue(FieldPtr, FieldValue->AsString());
            UpdatedFields.Add(FieldName);
        }
        else if (FNameProperty* NameFieldProp = CastField<FNameProperty>(FieldProp))
        {
            NameFieldProp->SetPropertyValue(FieldPtr, FName(*FieldValue->AsString()));
            UpdatedFields.Add(FieldName);
        }
        else
        {
            Errors.Add(FString::Printf(TEXT("Unsupported field type for %s"), *FieldName));
        }
    }

    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), Errors.Num() == 0);
    Result->SetStringField(TEXT("blueprint_name"), BlueprintName);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetNumberField(TEXT("index"), Index);
    Result->SetNumberField(TEXT("updated_fields_count"), UpdatedFields.Num());

    // Add updated fields list
    TArray<TSharedPtr<FJsonValue>> UpdatedFieldsArray;
    for (const FString& Field : UpdatedFields)
    {
        UpdatedFieldsArray.Add(MakeShareable(new FJsonValueString(Field)));
    }
    Result->SetArrayField(TEXT("updated_fields"), UpdatedFieldsArray);

    // Add errors if any
    if (Errors.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrorsArray;
        for (const FString& Error : Errors)
        {
            ErrorsArray.Add(MakeShareable(new FJsonValueString(Error)));
        }
        Result->SetArrayField(TEXT("errors"), ErrorsArray);
    }

    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetDataAssetProperty(
    const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString AssetName, PropertyName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_name"), AssetName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
    {
        return Error;
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Data"));

    // Load DataAsset
    FString FullPath = Path / AssetName + TEXT(".") + AssetName;
    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(FullPath);

    if (!LoadedAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("DataAsset not found: %s"), *FullPath));
    }

    UDataAsset* DataAsset = Cast<UDataAsset>(LoadedAsset);
    if (!DataAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParameter,
            FString::Printf(TEXT("Asset is not a DataAsset: %s"), *FullPath));
    }

    // Set the property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing 'property_value' parameter"));
    }

    TSharedPtr<FJsonValue> JsonValue = Params->Values.FindRef(TEXT("property_value"));

    FString ErrorMessage;
    if (!FSpirrowBridgeCommonUtils::SetObjectProperty(DataAsset, PropertyName, JsonValue, ErrorMessage))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertySetFailed,
            ErrorMessage);
    }

    // Save the DataAsset
    DataAsset->MarkPackageDirty();

    UPackage* Package = DataAsset->GetOutermost();
    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, DataAsset, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("asset_name"), AssetName);
    Result->SetStringField(TEXT("property_name"), PropertyName);
    Result->SetStringField(TEXT("asset_path"), FullPath);
    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleGetDataAssetProperties(
    const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString AssetName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_name"), AssetName))
    {
        return Error;
    }

    FString Path, CategoryFilter;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("category_filter"), CategoryFilter, TEXT(""));

    bool bIncludeInherited;
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("include_inherited"), bIncludeInherited, true);

    // Load DataAsset
    FString FullPath = Path / AssetName + TEXT(".") + AssetName;
    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(FullPath);

    if (!LoadedAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("DataAsset not found: %s"), *FullPath));
    }

    UDataAsset* DataAsset = Cast<UDataAsset>(LoadedAsset);
    if (!DataAsset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParameter,
            FString::Printf(TEXT("Asset is not a DataAsset: %s"), *FullPath));
    }

    UClass* AssetClass = DataAsset->GetClass();
    TArray<TSharedPtr<FJsonValue>> PropertiesArray;

    for (TFieldIterator<FProperty> PropIt(AssetClass); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;

        // Skip inherited if not wanted
        if (!bIncludeInherited && Property->GetOwnerClass() != AssetClass)
        {
            continue;
        }

        // Skip non-editable properties
        if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
        {
            continue;
        }

        // Category filter
        FString Category = Property->GetMetaData(TEXT("Category"));
        if (!CategoryFilter.IsEmpty() && !Category.Contains(CategoryFilter))
        {
            continue;
        }

        TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
        PropObj->SetStringField(TEXT("name"), Property->GetName());
        PropObj->SetStringField(TEXT("type"), GetPropertyTypeName(Property));
        PropObj->SetStringField(TEXT("category"), Category);
        PropObj->SetBoolField(TEXT("editable_in_editor"), Property->HasAnyPropertyFlags(CPF_Edit));

        // Get current value from the DataAsset instance
        FString Value;
        Property->ExportTextItem_Direct(Value,
            Property->ContainerPtrToValuePtr<void>(DataAsset), nullptr, DataAsset, PPF_None);
        PropObj->SetStringField(TEXT("value"), Value);

        PropertiesArray.Add(MakeShared<FJsonValueObject>(PropObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("asset_name"), AssetName);
    Result->SetStringField(TEXT("asset_class"), AssetClass->GetName());
    Result->SetNumberField(TEXT("property_count"), PropertiesArray.Num());
    Result->SetArrayField(TEXT("properties"), PropertiesArray);
    return Result;
}

// ===== Batch Set Properties =====
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleBatchSetProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_name"), AssetName))
    {
        return Error;
    }

    const TSharedPtr<FJsonObject>* PropertiesObj = nullptr;
    if (!Params->TryGetObjectField(TEXT("properties"), PropertiesObj))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: properties"));
    }

    FString Path, AssetType;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Data"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("asset_type"), AssetType, TEXT("dataasset"));

    // Load asset
    FString FullPath = Path / AssetName + TEXT(".") + AssetName;
    UObject* Asset = UEditorAssetLibrary::LoadAsset(FullPath);
    if (!Asset)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Asset not found: %s"), *FullPath));
    }

    // Set all properties
    TArray<FString> SuccessProperties;
    TArray<FString> FailedProperties;

    for (const auto& Pair : (*PropertiesObj)->Values)
    {
        FString ErrorMsg;
        if (FSpirrowBridgeCommonUtils::SetObjectProperty(Asset, Pair.Key, Pair.Value, ErrorMsg))
        {
            SuccessProperties.Add(Pair.Key);
        }
        else
        {
            FailedProperties.Add(FString::Printf(TEXT("%s: %s"), *Pair.Key, *ErrorMsg));
        }
    }

    // Save
    Asset->MarkPackageDirty();
    UPackage* Package = Asset->GetOutermost();
    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), FailedProperties.Num() == 0);
    Result->SetStringField(TEXT("asset_path"), FullPath);
    Result->SetNumberField(TEXT("succeeded_count"), SuccessProperties.Num());
    Result->SetNumberField(TEXT("failed_count"), FailedProperties.Num());

    TArray<TSharedPtr<FJsonValue>> SucceededArray;
    for (const FString& PropName : SuccessProperties)
    {
        SucceededArray.Add(MakeShared<FJsonValueString>(PropName));
    }
    Result->SetArrayField(TEXT("succeeded"), SucceededArray);

    TArray<TSharedPtr<FJsonValue>> FailedArray;
    for (const FString& ErrorStr : FailedProperties)
    {
        FailedArray.Add(MakeShared<FJsonValueString>(ErrorStr));
    }
    Result->SetArrayField(TEXT("failed"), FailedArray);

    return Result;
}
