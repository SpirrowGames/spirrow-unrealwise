#include "Commands/SpirrowBridgeBlueprintPropertyCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
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

    return nullptr; // Not handled by this class
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleScanProjectClasses(const TSharedPtr<FJsonObject>& Params)
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

    bool bExcludeReinst = true;
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

            // Skip Blueprint-generated classes
            if (ClassName.EndsWith(TEXT("_C")))
            {
                continue;
            }

            // Filter REINST_* classes
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

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetBlueprintClassArray(const TSharedPtr<FJsonObject>& Params)
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

        UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
        if (!LoadedClass)
        {
            continue;
        }

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

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintPropertyCommands::HandleSetStructArrayProperty(const TSharedPtr<FJsonObject>& Params)
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
                continue;
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
                int32 IntValue = static_cast<int32>(FieldValue->AsNumber());
                IntFieldProp->SetPropertyValue(FieldPtr, IntValue);
            }
            else if (FFloatProperty* FloatFieldProp = CastField<FFloatProperty>(FieldProp))
            {
                float FloatValue = static_cast<float>(FieldValue->AsNumber());
                FloatFieldProp->SetPropertyValue(FieldPtr, FloatValue);
            }
            else if (FDoubleProperty* DoubleFieldProp = CastField<FDoubleProperty>(FieldProp))
            {
                double DoubleValue = FieldValue->AsNumber();
                DoubleFieldProp->SetPropertyValue(FieldPtr, DoubleValue);
            }
            else if (FBoolProperty* BoolFieldProp = CastField<FBoolProperty>(FieldProp))
            {
                bool BoolValue = FieldValue->AsBool();
                BoolFieldProp->SetPropertyValue(FieldPtr, BoolValue);
            }
            else if (FStrProperty* StrFieldProp = CastField<FStrProperty>(FieldProp))
            {
                FString StrValue = FieldValue->AsString();
                StrFieldProp->SetPropertyValue(FieldPtr, StrValue);
            }
            else if (FNameProperty* NameFieldProp = CastField<FNameProperty>(FieldProp))
            {
                FName NameValue = FName(*FieldValue->AsString());
                NameFieldProp->SetPropertyValue(FieldPtr, NameValue);
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
