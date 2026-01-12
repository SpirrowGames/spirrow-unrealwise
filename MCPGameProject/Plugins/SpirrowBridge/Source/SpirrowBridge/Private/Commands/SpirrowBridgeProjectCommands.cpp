#include "Commands/SpirrowBridgeProjectCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "EnhancedInputSubsystems.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "EdGraphSchema_K2.h"
#include "Factories/TextureFactory.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/App.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/Engine.h"  // For FlushAsyncLoading()

FSpirrowBridgeProjectCommands::FSpirrowBridgeProjectCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_input_mapping"))
    {
        return HandleCreateInputMapping(Params);
    }
    else if (CommandType == TEXT("create_input_action"))
    {
        return HandleCreateInputAction(Params);
    }
    else if (CommandType == TEXT("create_input_mapping_context"))
    {
        return HandleCreateInputMappingContext(Params);
    }
    else if (CommandType == TEXT("add_action_to_mapping_context"))
    {
        return HandleAddActionToMappingContext(Params);
    }
    else if (CommandType == TEXT("get_input_mapping_context"))
    {
        return HandleGetInputMappingContext(Params);
    }
    else if (CommandType == TEXT("get_input_action"))
    {
        return HandleGetInputAction(Params);
    }
    else if (CommandType == TEXT("remove_action_from_mapping_context"))
    {
        return HandleRemoveActionFromMappingContext(Params);
    }
    else if (CommandType == TEXT("delete_asset"))
    {
        return HandleDeleteAsset(Params);
    }
    else if (CommandType == TEXT("add_mapping_context_to_blueprint"))
    {
        return HandleAddMappingContextToBlueprint(Params);
    }
    else if (CommandType == TEXT("set_default_mapping_context"))
    {
        return HandleSetDefaultMappingContext(Params);
    }
    // Asset utility commands
    else if (CommandType == TEXT("asset_exists"))
    {
        return HandleAssetExists(Params);
    }
    else if (CommandType == TEXT("create_content_folder"))
    {
        return HandleCreateContentFolder(Params);
    }
    else if (CommandType == TEXT("list_assets_in_folder"))
    {
        return HandleListAssetsInFolder(Params);
    }
    else if (CommandType == TEXT("import_texture"))
    {
        return HandleImportTexture(Params);
    }
    else if (CommandType == TEXT("get_project_info"))
    {
        return HandleGetProjectInfo(Params);
    }
    else if (CommandType == TEXT("find_asset_references"))
    {
        return HandleFindAssetReferences(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::UnknownCommand,
        FString::Printf(TEXT("Unknown project command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCreateInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ActionName, Key;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("action_name"), ActionName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key"), Key))
    {
        return Error;
    }

    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::SystemError,
            TEXT("Failed to get input settings"));
    }

    FInputActionKeyMapping ActionMapping;
    ActionMapping.ActionName = FName(*ActionName);
    ActionMapping.Key = FKey(*Key);

    bool bShift = false, bCtrl = false, bAlt = false, bCmd = false;
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("shift"), bShift, false);
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("ctrl"), bCtrl, false);
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("alt"), bAlt, false);
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("cmd"), bCmd, false);
    ActionMapping.bShift = bShift;
    ActionMapping.bCtrl = bCtrl;
    ActionMapping.bAlt = bAlt;
    ActionMapping.bCmd = bCmd;

    InputSettings->AddActionMapping(ActionMapping);
    InputSettings->SaveConfig();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("action_name"), ActionName);
    ResultObj->SetStringField(TEXT("key"), Key);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCreateInputAction(const TSharedPtr<FJsonObject>& Params)
{
    FString ActionName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("action_name"), ActionName))
    {
        return Error;
    }

    FString ValueType, Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("value_type"), ValueType, TEXT("Digital"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Input"));

    EInputActionValueType ActionValueType = EInputActionValueType::Boolean;
    if (ValueType == TEXT("Axis1D"))
        ActionValueType = EInputActionValueType::Axis1D;
    else if (ValueType == TEXT("Axis2D"))
        ActionValueType = EInputActionValueType::Axis2D;
    else if (ValueType == TEXT("Axis3D"))
        ActionValueType = EInputActionValueType::Axis3D;

    FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *ActionName);

    if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Input Action already exists: %s"), *PackagePath));
    }

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package"));
    }

    UInputAction* NewAction = NewObject<UInputAction>(Package, *ActionName, RF_Public | RF_Standalone);
    if (!NewAction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create Input Action"));
    }

    NewAction->ValueType = ActionValueType;

    FAssetRegistryModule::AssetCreated(NewAction);
    NewAction->MarkPackageDirty();

    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewAction, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("path"), PackagePath);
    ResultObj->SetStringField(TEXT("value_type"), ValueType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    FString ContextName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("context_name"), ContextName))
    {
        return Error;
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Input"));

    FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *ContextName);

    if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Input Mapping Context already exists: %s"), *PackagePath));
    }

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package"));
    }

    UInputMappingContext* NewContext = NewObject<UInputMappingContext>(Package, *ContextName, RF_Public | RF_Standalone);
    if (!NewContext)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create Input Mapping Context"));
    }

    FAssetRegistryModule::AssetCreated(NewContext);
    NewContext->MarkPackageDirty();

    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewContext, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("path"), PackagePath);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleAddActionToMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    FString ContextName, ActionName, KeyName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("context_name"), ContextName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("action_name"), ActionName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key"), KeyName))
    {
        return Error;
    }

    FString TriggerType, ContextPath, ActionPath;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("trigger_type"), TriggerType, TEXT("Down"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("context_path"), ContextPath, TEXT("/Game/Input"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("action_path"), ActionPath, TEXT("/Game/Input"));

    FString FullContextPath = FString::Printf(TEXT("%s/%s.%s"), *ContextPath, *ContextName, *ContextName);
    UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *FullContextPath);
    if (!Context)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Input Mapping Context not found: %s"), *FullContextPath));
    }

    FString FullActionPath = FString::Printf(TEXT("%s/%s.%s"), *ActionPath, *ActionName, *ActionName);
    UInputAction* Action = LoadObject<UInputAction>(nullptr, *FullActionPath);
    if (!Action)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Input Action not found: %s"), *FullActionPath));
    }

    FKey Key(*KeyName);
    if (!Key.IsValid())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidParameter,
            FString::Printf(TEXT("Invalid key: %s"), *KeyName));
    }

    FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);

    if (TriggerType == TEXT("Hold"))
    {
        UInputTriggerHold* Trigger = NewObject<UInputTriggerHold>(Context);
        Mapping.Triggers.Add(Trigger);
    }
    else if (TriggerType == TEXT("Pressed"))
    {
        UInputTriggerPressed* Trigger = NewObject<UInputTriggerPressed>(Context);
        Mapping.Triggers.Add(Trigger);
    }
    else if (TriggerType == TEXT("Released"))
    {
        UInputTriggerReleased* Trigger = NewObject<UInputTriggerReleased>(Context);
        Mapping.Triggers.Add(Trigger);
    }
    else if (TriggerType == TEXT("HoldAndRelease"))
    {
        UInputTriggerHoldAndRelease* Trigger = NewObject<UInputTriggerHoldAndRelease>(Context);
        Mapping.Triggers.Add(Trigger);
    }

    // ★ Modifiers処理（文字列またはオブジェクト形式に対応）★
    TArray<FString> AppliedModifiers;
    const TArray<TSharedPtr<FJsonValue>>* ModifiersArray;
    if (Params->TryGetArrayField(TEXT("modifiers"), ModifiersArray))
    {
        for (const auto& ModValue : *ModifiersArray)
        {
            FString ModName;
            TSharedPtr<FJsonObject> ModObj;

            // オブジェクト形式: {"type": "Scalar", "scale": 2.0}
            if (ModValue->Type == EJson::Object)
            {
                ModObj = ModValue->AsObject();
                ModObj->TryGetStringField(TEXT("type"), ModName);
            }
            // 文字列形式: "Negate"
            else if (ModValue->Type == EJson::String)
            {
                ModName = ModValue->AsString();
            }

            if (ModName == TEXT("Negate"))
            {
                UInputModifierNegate* Mod = NewObject<UInputModifierNegate>(Context);
                Mapping.Modifiers.Add(Mod);
                AppliedModifiers.Add(TEXT("Negate"));
            }
            else if (ModName == TEXT("Scalar"))
            {
                UInputModifierScalar* Mod = NewObject<UInputModifierScalar>(Context);
                if (ModObj)
                {
                    double ScaleValue = 1.0;
                    if (ModObj->TryGetNumberField(TEXT("scale"), ScaleValue))
                    {
                        Mod->Scalar = FVector(ScaleValue, ScaleValue, ScaleValue);
                    }
                    // 個別軸指定: {"type": "Scalar", "x": 1.0, "y": 2.0, "z": 0.0}
                    double X = ScaleValue, Y = ScaleValue, Z = ScaleValue;
                    ModObj->TryGetNumberField(TEXT("x"), X);
                    ModObj->TryGetNumberField(TEXT("y"), Y);
                    ModObj->TryGetNumberField(TEXT("z"), Z);
                    Mod->Scalar = FVector(X, Y, Z);
                }
                Mapping.Modifiers.Add(Mod);
                AppliedModifiers.Add(FString::Printf(TEXT("Scalar(%.2f,%.2f,%.2f)"),
                    Mod->Scalar.X, Mod->Scalar.Y, Mod->Scalar.Z));
            }
            else if (ModName == TEXT("SwizzleYXZ") || ModName == TEXT("Swizzle"))
            {
                UInputModifierSwizzleAxis* Mod = NewObject<UInputModifierSwizzleAxis>(Context);
                Mod->Order = EInputAxisSwizzle::YXZ;
                if (ModObj)
                {
                    FString OrderStr;
                    if (ModObj->TryGetStringField(TEXT("order"), OrderStr))
                    {
                        if (OrderStr == TEXT("YXZ")) Mod->Order = EInputAxisSwizzle::YXZ;
                        else if (OrderStr == TEXT("ZYX")) Mod->Order = EInputAxisSwizzle::ZYX;
                        else if (OrderStr == TEXT("XZY")) Mod->Order = EInputAxisSwizzle::XZY;
                        else if (OrderStr == TEXT("YZX")) Mod->Order = EInputAxisSwizzle::YZX;
                        else if (OrderStr == TEXT("ZXY")) Mod->Order = EInputAxisSwizzle::ZXY;
                    }
                }
                Mapping.Modifiers.Add(Mod);
                AppliedModifiers.Add(TEXT("Swizzle"));
            }
            else if (ModName == TEXT("DeadZone"))
            {
                UInputModifierDeadZone* Mod = NewObject<UInputModifierDeadZone>(Context);
                if (ModObj)
                {
                    double LowerThreshold = 0.2, UpperThreshold = 1.0;
                    ModObj->TryGetNumberField(TEXT("lower_threshold"), LowerThreshold);
                    ModObj->TryGetNumberField(TEXT("upper_threshold"), UpperThreshold);
                    Mod->LowerThreshold = LowerThreshold;
                    Mod->UpperThreshold = UpperThreshold;
                }
                Mapping.Modifiers.Add(Mod);
                AppliedModifiers.Add(TEXT("DeadZone"));
            }
            else if (!ModName.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("Unknown modifier type: %s"), *ModName);
            }
        }
    }

    Context->MarkPackageDirty();
    FString PackagePath = FString::Printf(TEXT("%s/%s"), *ContextPath, *ContextName);
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Context->GetOutermost(), Context, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("context"), ContextName);
    ResultObj->SetStringField(TEXT("action"), ActionName);
    ResultObj->SetStringField(TEXT("key"), KeyName);
    ResultObj->SetStringField(TEXT("trigger"), TriggerType);

    // ★ 適用されたモディファイアをレスポンスに含める ★
    if (AppliedModifiers.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ModArray;
        for (const FString& ModStr : AppliedModifiers)
        {
            ModArray.Add(MakeShared<FJsonValueString>(ModStr));
        }
        ResultObj->SetArrayField(TEXT("modifiers_applied"), ModArray);
    }

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleDeleteAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_path"), AssetPath))
    {
        return Error;
    }

    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetPath);
    if (!bDeleted)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::OperationFailed,
            FString::Printf(TEXT("Failed to delete asset: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("deleted"), AssetPath);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleAddMappingContextToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, ContextName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("context_name"), ContextName))
    {
        return Error;
    }

    FString ContextPath, Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("context_path"), ContextPath, TEXT("/Game/Input"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    double PriorityDouble;
    FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("priority"), PriorityDouble, 0.0);
    int32 Priority = static_cast<int32>(PriorityDouble);

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::BlueprintNotFound,
            FString::Printf(TEXT("Blueprint not found: %s/%s"), *Path, *BlueprintName));
    }

    FString FullContextPath = FString::Printf(TEXT("%s/%s.%s"), *ContextPath, *ContextName, *ContextName);
    UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr, *FullContextPath);
    if (!MappingContext)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("IMC not found: %s"), *FullContextPath));
    }

    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::GraphNotFound,
            TEXT("Event graph not found"));
    }

    int32 NodeX = 0;
    int32 NodeY = 0;
    TArray<FString> CreatedNodeIds;

    UK2Node_Event* BeginPlayNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName("ReceiveBeginPlay"))
        {
            BeginPlayNode = EventNode;
            NodeX = BeginPlayNode->NodePosX + 300;
            NodeY = BeginPlayNode->NodePosY;
            break;
        }
    }

    if (!BeginPlayNode)
    {
        int32 OutNodePosY = NodeY;
        BeginPlayNode = FKismetEditorUtilities::AddDefaultEventNode(
            Blueprint, EventGraph, FName("ReceiveBeginPlay"), AActor::StaticClass(), OutNodePosY);
        if (!BeginPlayNode)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::NodeCreationFailed,
                TEXT("Failed to create BeginPlay event"));
        }
        NodeX = BeginPlayNode->NodePosX + 300;
        NodeY = BeginPlayNode->NodePosY;
    }

    CreatedNodeIds.Add(BeginPlayNode->NodeGuid.ToString());

    UK2Node_CallFunction* GetControllerNode = NewObject<UK2Node_CallFunction>(EventGraph);
    UFunction* GetControllerFunc = APawn::StaticClass()->FindFunctionByName(TEXT("GetController"));
    if (GetControllerFunc)
    {
        GetControllerNode->SetFromFunction(GetControllerFunc);
    }
    else
    {
        GetControllerNode->FunctionReference.SetExternalMember(FName("GetController"), APawn::StaticClass());
    }
    GetControllerNode->NodePosX = NodeX;
    GetControllerNode->NodePosY = NodeY;
    EventGraph->AddNode(GetControllerNode, false, false);
    GetControllerNode->CreateNewGuid();
    GetControllerNode->PostPlacedNewNode();
    GetControllerNode->AllocateDefaultPins();
    CreatedNodeIds.Add(GetControllerNode->NodeGuid.ToString());
    NodeX += 250;

    UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(EventGraph);
    CastNode->TargetType = APlayerController::StaticClass();
    CastNode->NodePosX = NodeX;
    CastNode->NodePosY = NodeY;
    EventGraph->AddNode(CastNode, false, false);
    CastNode->CreateNewGuid();
    CastNode->PostPlacedNewNode();
    CastNode->AllocateDefaultPins();
    CreatedNodeIds.Add(CastNode->NodeGuid.ToString());
    NodeX += 300;

    UK2Node_CallFunction* GetSubsystemNode = NewObject<UK2Node_CallFunction>(EventGraph);
    GetSubsystemNode->FunctionReference.SetExternalMember(
        FName("Get"),
        UEnhancedInputLocalPlayerSubsystem::StaticClass());
    GetSubsystemNode->NodePosX = NodeX;
    GetSubsystemNode->NodePosY = NodeY;
    EventGraph->AddNode(GetSubsystemNode, false, false);
    GetSubsystemNode->CreateNewGuid();
    GetSubsystemNode->PostPlacedNewNode();
    GetSubsystemNode->AllocateDefaultPins();
    CreatedNodeIds.Add(GetSubsystemNode->NodeGuid.ToString());
    NodeX += 300;

    UK2Node_CallFunction* AddContextNode = NewObject<UK2Node_CallFunction>(EventGraph);
    UFunction* AddContextFunc = UEnhancedInputLocalPlayerSubsystem::StaticClass()->FindFunctionByName(TEXT("AddMappingContext"));
    if (AddContextFunc)
    {
        AddContextNode->SetFromFunction(AddContextFunc);
    }
    else
    {
        AddContextNode->FunctionReference.SetExternalMember(
            FName("AddMappingContext"),
            UEnhancedInputLocalPlayerSubsystem::StaticClass());
    }
    AddContextNode->NodePosX = NodeX;
    AddContextNode->NodePosY = NodeY;
    EventGraph->AddNode(AddContextNode, false, false);
    AddContextNode->CreateNewGuid();
    AddContextNode->PostPlacedNewNode();
    AddContextNode->AllocateDefaultPins();
    CreatedNodeIds.Add(AddContextNode->NodeGuid.ToString());

    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

    UEdGraphPin* BeginPlayExec = BeginPlayNode->FindPin(UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* CastExec = CastNode->FindPin(UEdGraphSchema_K2::PN_Execute);
    if (BeginPlayExec && CastExec)
    {
        BeginPlayExec->MakeLinkTo(CastExec);
    }

    UEdGraphPin* GetControllerReturn = GetControllerNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    UEdGraphPin* CastObject = CastNode->FindPin(TEXT("Object"));
    if (GetControllerReturn && CastObject)
    {
        GetControllerReturn->MakeLinkTo(CastObject);
    }

    UEdGraphPin* CastThenExec = CastNode->FindPin(UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* AddContextExec = AddContextNode->FindPin(UEdGraphSchema_K2::PN_Execute);
    if (CastThenExec && AddContextExec)
    {
        CastThenExec->MakeLinkTo(AddContextExec);
    }

    UEdGraphPin* CastResult = CastNode->GetCastResultPin();
    UEdGraphPin* SubsystemPlayerController = GetSubsystemNode->FindPin(TEXT("PlayerController"));
    if (CastResult && SubsystemPlayerController)
    {
        CastResult->MakeLinkTo(SubsystemPlayerController);
    }

    UEdGraphPin* SubsystemReturn = GetSubsystemNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
    UEdGraphPin* AddContextTarget = AddContextNode->FindPin(UEdGraphSchema_K2::PN_Self);
    if (SubsystemReturn && AddContextTarget)
    {
        SubsystemReturn->MakeLinkTo(AddContextTarget);
    }

    UEdGraphPin* MappingContextPin = AddContextNode->FindPin(TEXT("MappingContext"));
    if (MappingContextPin)
    {
        MappingContextPin->DefaultObject = MappingContext;
    }

    UEdGraphPin* PriorityPin = AddContextNode->FindPin(TEXT("Priority"));
    if (PriorityPin)
    {
        PriorityPin->DefaultValue = FString::FromInt(Priority);
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);

    TArray<TSharedPtr<FJsonValue>> NodeIdsArray;
    for (const FString& NodeId : CreatedNodeIds)
    {
        NodeIdsArray.Add(MakeShared<FJsonValueString>(NodeId));
    }
    Result->SetArrayField(TEXT("nodes"), NodeIdsArray);
    Result->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Added %d nodes for AddMappingContext to %s"), CreatedNodeIds.Num(), *BlueprintName));

    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleSetDefaultMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName, ContextName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("context_name"), ContextName))
    {
        return Error;
    }

    FString ContextPath, Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("context_path"), ContextPath, TEXT("/Game/Input"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::BlueprintNotFound,
            FString::Printf(TEXT("Blueprint not found: %s/%s"), *Path, *BlueprintName));
    }

    return HandleAddMappingContextToBlueprint(Params);
}

// ===== Get Input Mapping Context =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleGetInputMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    FString ContextName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("context_name"), ContextName))
    {
        return Error;
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Input"));

    FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *ContextName, *ContextName);
    UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *FullPath);
    if (!Context)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Input Mapping Context not found: %s"), *FullPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("context_name"), ContextName);
    ResultObj->SetStringField(TEXT("path"), Path);

    // マッピング一覧を取得
    TArray<TSharedPtr<FJsonValue>> MappingsArray;
    const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();

    for (const FEnhancedActionKeyMapping& Mapping : Mappings)
    {
        TSharedPtr<FJsonObject> MappingObj = MakeShared<FJsonObject>();

        // アクション名
        if (Mapping.Action)
        {
            MappingObj->SetStringField(TEXT("action"), Mapping.Action->GetName());
        }

        // キー
        MappingObj->SetStringField(TEXT("key"), Mapping.Key.GetFName().ToString());

        // トリガー
        TArray<TSharedPtr<FJsonValue>> TriggersArray;
        for (UInputTrigger* Trigger : Mapping.Triggers)
        {
            if (Trigger)
            {
                TriggersArray.Add(MakeShared<FJsonValueString>(Trigger->GetClass()->GetName()));
            }
        }
        if (TriggersArray.Num() > 0)
        {
            MappingObj->SetArrayField(TEXT("triggers"), TriggersArray);
        }

        // モディファイア
        TArray<TSharedPtr<FJsonValue>> ModifiersArray;
        for (UInputModifier* Modifier : Mapping.Modifiers)
        {
            if (Modifier)
            {
                TSharedPtr<FJsonObject> ModObj = MakeShared<FJsonObject>();
                ModObj->SetStringField(TEXT("type"), Modifier->GetClass()->GetName());

                // Scalar の場合は値も出力
                if (UInputModifierScalar* ScalarMod = Cast<UInputModifierScalar>(Modifier))
                {
                    ModObj->SetNumberField(TEXT("x"), ScalarMod->Scalar.X);
                    ModObj->SetNumberField(TEXT("y"), ScalarMod->Scalar.Y);
                    ModObj->SetNumberField(TEXT("z"), ScalarMod->Scalar.Z);
                }
                // DeadZone の場合
                else if (UInputModifierDeadZone* DeadZoneMod = Cast<UInputModifierDeadZone>(Modifier))
                {
                    ModObj->SetNumberField(TEXT("lower_threshold"), DeadZoneMod->LowerThreshold);
                    ModObj->SetNumberField(TEXT("upper_threshold"), DeadZoneMod->UpperThreshold);
                }

                ModifiersArray.Add(MakeShared<FJsonValueObject>(ModObj));
            }
        }
        if (ModifiersArray.Num() > 0)
        {
            MappingObj->SetArrayField(TEXT("modifiers"), ModifiersArray);
        }

        MappingsArray.Add(MakeShared<FJsonValueObject>(MappingObj));
    }

    ResultObj->SetArrayField(TEXT("mappings"), MappingsArray);
    ResultObj->SetNumberField(TEXT("mapping_count"), Mappings.Num());

    return ResultObj;
}

// ===== Get Input Action =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleGetInputAction(const TSharedPtr<FJsonObject>& Params)
{
    FString ActionName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("action_name"), ActionName))
    {
        return Error;
    }

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Input"));

    FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *ActionName, *ActionName);
    UInputAction* Action = LoadObject<UInputAction>(nullptr, *FullPath);
    if (!Action)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Input Action not found: %s"), *FullPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("action_name"), ActionName);
    ResultObj->SetStringField(TEXT("path"), Path);

    // ValueType
    FString ValueTypeStr;
    switch (Action->ValueType)
    {
        case EInputActionValueType::Boolean: ValueTypeStr = TEXT("Boolean"); break;
        case EInputActionValueType::Axis1D: ValueTypeStr = TEXT("Axis1D"); break;
        case EInputActionValueType::Axis2D: ValueTypeStr = TEXT("Axis2D"); break;
        case EInputActionValueType::Axis3D: ValueTypeStr = TEXT("Axis3D"); break;
        default: ValueTypeStr = TEXT("Unknown"); break;
    }
    ResultObj->SetStringField(TEXT("value_type"), ValueTypeStr);

    // ConsumeInput
    ResultObj->SetBoolField(TEXT("consume_input"), Action->bConsumeInput);

    // Triggers
    TArray<TSharedPtr<FJsonValue>> TriggersArray;
    for (UInputTrigger* Trigger : Action->Triggers)
    {
        if (Trigger)
        {
            TriggersArray.Add(MakeShared<FJsonValueString>(Trigger->GetClass()->GetName()));
        }
    }
    if (TriggersArray.Num() > 0)
    {
        ResultObj->SetArrayField(TEXT("triggers"), TriggersArray);
    }

    // Modifiers
    TArray<TSharedPtr<FJsonValue>> ModifiersArray;
    for (UInputModifier* Modifier : Action->Modifiers)
    {
        if (Modifier)
        {
            ModifiersArray.Add(MakeShared<FJsonValueString>(Modifier->GetClass()->GetName()));
        }
    }
    if (ModifiersArray.Num() > 0)
    {
        ResultObj->SetArrayField(TEXT("modifiers"), ModifiersArray);
    }

    return ResultObj;
}

// ===== Remove Action From Mapping Context =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleRemoveActionFromMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    FString ContextName, ActionName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("context_name"), ContextName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("action_name"), ActionName))
    {
        return Error;
    }

    FString KeyName;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("key"), KeyName, TEXT(""));

    FString ContextPath, ActionPath;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("context_path"), ContextPath, TEXT("/Game/Input"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("action_path"), ActionPath, TEXT("/Game/Input"));

    FString FullContextPath = FString::Printf(TEXT("%s/%s.%s"), *ContextPath, *ContextName, *ContextName);
    UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *FullContextPath);
    if (!Context)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Input Mapping Context not found: %s"), *FullContextPath));
    }

    FString FullActionPath = FString::Printf(TEXT("%s/%s.%s"), *ActionPath, *ActionName, *ActionName);
    UInputAction* Action = LoadObject<UInputAction>(nullptr, *FullActionPath);
    if (!Action)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Input Action not found: %s"), *FullActionPath));
    }

    int32 RemovedCount = 0;

    // マッピングを検索して削除対象のキーを収集
    TArray<FKey> KeysToRemove;
    const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();

    if (KeyName.IsEmpty())
    {
        // すべてのマッピングを削除（該当Actionの全キー）
        for (const FEnhancedActionKeyMapping& Mapping : Mappings)
        {
            if (Mapping.Action == Action)
            {
                KeysToRemove.Add(Mapping.Key);
            }
        }
    }
    else
    {
        // 特定のキーのマッピングのみ削除
        FKey Key(*KeyName);
        if (!Key.IsValid())
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::InvalidParameter,
                FString::Printf(TEXT("Invalid key: %s"), *KeyName));
        }

        for (const FEnhancedActionKeyMapping& Mapping : Mappings)
        {
            if (Mapping.Action == Action && Mapping.Key == Key)
            {
                KeysToRemove.Add(Mapping.Key);
            }
        }
    }

    // 収集したキーを削除
    for (const FKey& KeyToRemove : KeysToRemove)
    {
        Context->UnmapKey(Action, KeyToRemove);
        RemovedCount++;
    }

    // 保存
    Context->MarkPackageDirty();
    FString PackagePath = FString::Printf(TEXT("%s/%s"), *ContextPath, *ContextName);
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Context->GetOutermost(), Context, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("context"), ContextName);
    ResultObj->SetStringField(TEXT("action"), ActionName);
    if (!KeyName.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("key"), KeyName);
    }
    ResultObj->SetNumberField(TEXT("removed_count"), RemovedCount);

    return ResultObj;
}

// ===== Asset Exists =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleAssetExists(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_path"), AssetPath))
    {
        return Error;
    }

    bool bExists = UEditorAssetLibrary::DoesAssetExist(AssetPath);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetBoolField(TEXT("exists"), bExists);
    ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
    return ResultObj;
}

// ===== Create Content Folder =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCreateContentFolder(const TSharedPtr<FJsonObject>& Params)
{
    FString FolderPath;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("folder_path"), FolderPath))
    {
        return Error;
    }

    bool bSuccess = UEditorAssetLibrary::MakeDirectory(FolderPath);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bSuccess);
    ResultObj->SetStringField(TEXT("folder_path"), FolderPath);

    if (!bSuccess)
    {
        ResultObj->SetStringField(TEXT("error"), TEXT("Failed to create folder. It may already exist or the path is invalid."));
    }

    return ResultObj;
}

// ===== List Assets In Folder =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleListAssetsInFolder(const TSharedPtr<FJsonObject>& Params)
{
    FString FolderPath;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("folder_path"), FolderPath))
    {
        return Error;
    }

    FString ClassFilter;
    bool bRecursive = false;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("class_filter"), ClassFilter, TEXT(""));
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("recursive"), bRecursive, false);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetList;
    AssetRegistry.GetAssetsByPath(FName(*FolderPath), AssetList, bRecursive);

    TArray<TSharedPtr<FJsonValue>> AssetsArray;
    for (const FAssetData& Asset : AssetList)
    {
        FString AssetClassName = Asset.AssetClassPath.GetAssetName().ToString();

        // Apply class filter if specified
        if (!ClassFilter.IsEmpty())
        {
            if (!AssetClassName.Contains(ClassFilter))
            {
                continue;
            }
        }

        TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
        AssetObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
        AssetObj->SetStringField(TEXT("class"), AssetClassName);
        AssetsArray.Add(MakeShared<FJsonValueObject>(AssetObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("assets"), AssetsArray);
    ResultObj->SetNumberField(TEXT("count"), AssetsArray.Num());
    ResultObj->SetStringField(TEXT("folder_path"), FolderPath);
    return ResultObj;
}

// ===== Import Texture =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleImportTexture(const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Warning, TEXT("=== HandleImportTexture: START ==="));
    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: IsInGameThread = %s"), IsInGameThread() ? TEXT("YES") : TEXT("NO"));

    FString Source, AssetName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("source"), Source))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_name"), AssetName))
    {
        return Error;
    }
    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: AssetName = %s"), *AssetName);

    FString SourceType, DestinationPath, Compression, LodGroup;
    bool bSRGB = true;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("source_type"), SourceType, TEXT("file"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("destination_path"), DestinationPath, TEXT("/Game/Textures"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("compression"), Compression, TEXT("Default"));
    FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("srgb"), bSRGB, true);
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("lod_group"), LodGroup, TEXT("World"));

    FString SourceFilePath = Source;
    FString TempFilePath;
    bool bDeleteTempFile = false;

    // Handle Base64 input
    if (SourceType.Equals(TEXT("base64"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Processing Base64 input (length=%d)"), Source.Len());

        TArray<uint8> DecodedData;
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Starting Base64 decode..."));
        if (!FBase64::Decode(Source, DecodedData))
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::InvalidParameter,
                TEXT("Failed to decode Base64 data"));
        }
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Base64 decode complete (decoded size=%d bytes)"), DecodedData.Num());

        // Create temp directory if it doesn't exist
        FString TempDir = FPaths::ProjectSavedDir() / TEXT("Temp");
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.DirectoryExists(*TempDir))
        {
            UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Creating temp directory: %s"), *TempDir);
            PlatformFile.CreateDirectory(*TempDir);
        }

        // Write to temp file
        TempFilePath = TempDir / AssetName + TEXT(".png");
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Writing temp file: %s"), *TempFilePath);
        if (!FFileHelper::SaveArrayToFile(DecodedData, *TempFilePath))
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::OperationFailed,
                TEXT("Failed to write temporary file for Base64 data"));
        }
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Temp file written successfully"));

        SourceFilePath = TempFilePath;
        bDeleteTempFile = true;
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Base64 processing complete, TempFile = %s"), *TempFilePath);
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: SourceFilePath = %s"), *SourceFilePath);

    // Check if source file exists
    if (!FPaths::FileExists(SourceFilePath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Source file not found: %s"), *SourceFilePath));
    }

    // Create package for the texture
    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Creating package..."));
    FString PackagePath = DestinationPath / AssetName;
    UPackage* Package = CreatePackage(*PackagePath);
    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Package created = %s"), Package ? TEXT("YES") : TEXT("NO"));
    if (!Package)
    {
        if (bDeleteTempFile && !TempFilePath.IsEmpty())
        {
            IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
            PlatformFile.DeleteFile(*TempFilePath);
        }
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package for texture"));
    }
    Package->FullyLoad();

    // Use UTextureFactory for direct import (GameThread-safe, no TaskGraph issues)
    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Creating TextureFactory..."));
    UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
    TextureFactory->AddToRoot(); // Prevent GC during import
    TextureFactory->SuppressImportOverwriteDialog();
    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: TextureFactory created"));

    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Calling ImportObject..."));
    bool bCancelled = false;
    UTexture2D* ImportedTexture = Cast<UTexture2D>(
        TextureFactory->ImportObject(
            UTexture2D::StaticClass(),
            Package,
            *AssetName,
            RF_Public | RF_Standalone,
            SourceFilePath,
            nullptr,
            bCancelled
        )
    );
    UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: ImportObject completed, result = %s, cancelled = %s"),
        ImportedTexture ? TEXT("SUCCESS") : TEXT("NULL"), bCancelled ? TEXT("YES") : TEXT("NO"));

    TextureFactory->RemoveFromRoot(); // Allow GC

    // Clean up temp file if needed
    if (bDeleteTempFile && !TempFilePath.IsEmpty())
    {
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        PlatformFile.DeleteFile(*TempFilePath);
    }

    // Check import result
    if (!ImportedTexture || bCancelled)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to import texture"));
    }
    if (ImportedTexture)
    {
        ImportedTexture->SRGB = bSRGB;

        // Set compression settings
        if (Compression.Equals(TEXT("Normalmap"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->CompressionSettings = TC_Normalmap;
        }
        else if (Compression.Equals(TEXT("Masks"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->CompressionSettings = TC_Masks;
        }
        else if (Compression.Equals(TEXT("Grayscale"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->CompressionSettings = TC_Grayscale;
        }
        else if (Compression.Equals(TEXT("UserInterface2D"), ESearchCase::IgnoreCase) ||
                 Compression.Equals(TEXT("UI"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->CompressionSettings = TC_EditorIcon;
        }
        else if (Compression.Equals(TEXT("BC7"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->CompressionSettings = TC_BC7;
        }
        // Default is TC_Default

        // Set LOD group
        if (LodGroup.Equals(TEXT("UI"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->LODGroup = TEXTUREGROUP_UI;
        }
        else if (LodGroup.Equals(TEXT("WorldNormalMap"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->LODGroup = TEXTUREGROUP_WorldNormalMap;
        }
        else if (LodGroup.Equals(TEXT("Lightmap"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->LODGroup = TEXTUREGROUP_Lightmap;
        }
        else if (LodGroup.Equals(TEXT("Shadowmap"), ESearchCase::IgnoreCase))
        {
            ImportedTexture->LODGroup = TEXTUREGROUP_Shadowmap;
        }
        // Default is TEXTUREGROUP_World

        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Calling UpdateResource..."));
        ImportedTexture->UpdateResource();
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: UpdateResource completed"));

        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Calling MarkPackageDirty..."));
        ImportedTexture->MarkPackageDirty();
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: MarkPackageDirty completed"));

        // Notify asset registry of the new asset
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Calling AssetCreated..."));
        FAssetRegistryModule::AssetCreated(ImportedTexture);
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: AssetCreated completed"));

        // Flush async loading to prevent TaskGraph recursion guard crash
        // when SavePackage internally dispatches async operations
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Calling FlushAsyncLoading..."));
        FlushAsyncLoading();
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: FlushAsyncLoading completed"));

        // Save the texture
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Preparing SavePackage..."));
        FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: PackageFileName = %s"), *PackageFileName);
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        // Note: Do NOT use SAVE_Async flag to avoid TaskGraph recursion
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: Calling SavePackage..."));
        UPackage::SavePackage(ImportedTexture->GetOutermost(), ImportedTexture, *PackageFileName, SaveArgs);
        UE_LOG(LogTemp, Warning, TEXT("HandleImportTexture: SavePackage completed"));
    }

    UE_LOG(LogTemp, Warning, TEXT("=== HandleImportTexture: END ==="));

    FString AssetPath = DestinationPath / AssetName + TEXT(".") + AssetName;

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
    ResultObj->SetStringField(TEXT("asset_name"), AssetName);
    ResultObj->SetStringField(TEXT("destination_path"), DestinationPath);
    ResultObj->SetStringField(TEXT("source_type"), SourceType);
    return ResultObj;
}

// ===== Get Project Info =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleGetProjectInfo(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);

    ResultObj->SetStringField(TEXT("project_name"), FApp::GetProjectName());
    ResultObj->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
    ResultObj->SetStringField(TEXT("project_dir"), FPaths::ProjectDir());
    ResultObj->SetStringField(TEXT("content_dir"), FPaths::ProjectContentDir());
    ResultObj->SetStringField(TEXT("saved_dir"), FPaths::ProjectSavedDir());

    return ResultObj;
}

// ===== Find Asset References =====
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleFindAssetReferences(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("asset_path"), AssetPath))
    {
        return Error;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Get referencers (assets that reference this asset)
    TArray<FAssetIdentifier> Referencers;
    FAssetIdentifier AssetId = FAssetIdentifier(FName(*AssetPath));
    AssetRegistry.GetReferencers(AssetId, Referencers);

    TArray<TSharedPtr<FJsonValue>> ReferencersArray;
    for (const FAssetIdentifier& Ref : Referencers)
    {
        ReferencersArray.Add(MakeShared<FJsonValueString>(Ref.ToString()));
    }

    // Get dependencies (assets that this asset references)
    TArray<FAssetIdentifier> Dependencies;
    AssetRegistry.GetDependencies(AssetId, Dependencies);

    TArray<TSharedPtr<FJsonValue>> DependenciesArray;
    for (const FAssetIdentifier& Dep : Dependencies)
    {
        DependenciesArray.Add(MakeShared<FJsonValueString>(Dep.ToString()));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("asset_path"), AssetPath);
    ResultObj->SetArrayField(TEXT("referencers"), ReferencersArray);
    ResultObj->SetNumberField(TEXT("referencers_count"), ReferencersArray.Num());
    ResultObj->SetArrayField(TEXT("dependencies"), DependenciesArray);
    ResultObj->SetNumberField(TEXT("dependencies_count"), DependenciesArray.Num());
    return ResultObj;
}
