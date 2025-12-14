#include "Commands/SpirrowBridgeProjectCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "GameFramework/InputSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "EnhancedInputSubsystems.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "EditorAssetLibrary.h"

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
    else if (CommandType == TEXT("delete_asset"))
    {
        return HandleDeleteAsset(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown project command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCreateInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString Key;
    if (!Params->TryGetStringField(TEXT("key"), Key))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Get the input settings
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get input settings"));
    }

    // Create the input action mapping
    FInputActionKeyMapping ActionMapping;
    ActionMapping.ActionName = FName(*ActionName);
    ActionMapping.Key = FKey(*Key);

    // Add modifiers if provided
    if (Params->HasField(TEXT("shift")))
    {
        ActionMapping.bShift = Params->GetBoolField(TEXT("shift"));
    }
    if (Params->HasField(TEXT("ctrl")))
    {
        ActionMapping.bCtrl = Params->GetBoolField(TEXT("ctrl"));
    }
    if (Params->HasField(TEXT("alt")))
    {
        ActionMapping.bAlt = Params->GetBoolField(TEXT("alt"));
    }
    if (Params->HasField(TEXT("cmd")))
    {
        ActionMapping.bCmd = Params->GetBoolField(TEXT("cmd"));
    }

    // Add the mapping
    InputSettings->AddActionMapping(ActionMapping);
    InputSettings->SaveConfig();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("action_name"), ActionName);
    ResultObj->SetStringField(TEXT("key"), Key);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCreateInputAction(const TSharedPtr<FJsonObject>& Params)
{
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString ValueType;
    if (!Params->TryGetStringField(TEXT("value_type"), ValueType))
    {
        ValueType = TEXT("Digital");
    }

    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Input");
    }

    // ValueType マッピング
    EInputActionValueType ActionValueType = EInputActionValueType::Boolean;
    if (ValueType == TEXT("Axis1D"))
    {
        ActionValueType = EInputActionValueType::Axis1D;
    }
    else if (ValueType == TEXT("Axis2D"))
    {
        ActionValueType = EInputActionValueType::Axis2D;
    }
    else if (ValueType == TEXT("Axis3D"))
    {
        ActionValueType = EInputActionValueType::Axis3D;
    }

    // パッケージパス作成
    FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *ActionName);

    // 既存アセットチェック
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Input Action already exists: %s"), *PackagePath));
    }

    UPackage* Package = CreatePackage(*PackagePath);

    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }

    // InputAction作成
    UInputAction* NewAction = NewObject<UInputAction>(Package, *ActionName, RF_Public | RF_Standalone);

    if (!NewAction)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Input Action"));
    }

    NewAction->ValueType = ActionValueType;

    // アセット登録
    FAssetRegistryModule::AssetCreated(NewAction);
    NewAction->MarkPackageDirty();

    // 保存
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewAction, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("path"), PackagePath);
    ResultObj->SetStringField(TEXT("value_type"), ValueType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    FString ContextName;
    if (!Params->TryGetStringField(TEXT("context_name"), ContextName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'context_name' parameter"));
    }

    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Input");
    }

    // パッケージパス作成
    FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *ContextName);

    // 既存アセットチェック
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Input Mapping Context already exists: %s"), *PackagePath));
    }

    UPackage* Package = CreatePackage(*PackagePath);

    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }

    // InputMappingContext作成
    UInputMappingContext* NewContext = NewObject<UInputMappingContext>(Package, *ContextName, RF_Public | RF_Standalone);

    if (!NewContext)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Input Mapping Context"));
    }

    FAssetRegistryModule::AssetCreated(NewContext);
    NewContext->MarkPackageDirty();

    // 保存
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewContext, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("path"), PackagePath);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleAddActionToMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    FString ContextName;
    if (!Params->TryGetStringField(TEXT("context_name"), ContextName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'context_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString KeyName;
    if (!Params->TryGetStringField(TEXT("key"), KeyName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    FString TriggerType;
    if (!Params->TryGetStringField(TEXT("trigger_type"), TriggerType))
    {
        TriggerType = TEXT("Down");
    }

    FString ContextPath;
    if (!Params->TryGetStringField(TEXT("context_path"), ContextPath))
    {
        ContextPath = TEXT("/Game/Input");
    }

    FString ActionPath;
    if (!Params->TryGetStringField(TEXT("action_path"), ActionPath))
    {
        ActionPath = TEXT("/Game/Input");
    }

    // IMCアセットをロード
    FString FullContextPath = FString::Printf(TEXT("%s/%s.%s"), *ContextPath, *ContextName, *ContextName);
    UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *FullContextPath);

    if (!Context)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Mapping Context not found: %s"), *FullContextPath));
    }

    // IAアセットをロード
    FString FullActionPath = FString::Printf(TEXT("%s/%s.%s"), *ActionPath, *ActionName, *ActionName);
    UInputAction* Action = LoadObject<UInputAction>(nullptr, *FullActionPath);

    if (!Action)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input Action not found: %s"), *FullActionPath));
    }

    // キーをパース
    FKey Key(*KeyName);
    if (!Key.IsValid())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid key: %s"), *KeyName));
    }

    // マッピング追加
    FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);

    // トリガー設定
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
    // "Down" はデフォルト動作、トリガー追加不要

    // Modifiers処理
    const TArray<TSharedPtr<FJsonValue>>* ModifiersArray;
    if (Params->TryGetArrayField(TEXT("modifiers"), ModifiersArray))
    {
        for (const auto& ModValue : *ModifiersArray)
        {
            FString ModName = ModValue->AsString();

            if (ModName == TEXT("Negate"))
            {
                UInputModifierNegate* Mod = NewObject<UInputModifierNegate>(Context);
                Mapping.Modifiers.Add(Mod);
            }
            else if (ModName == TEXT("SwizzleYXZ"))
            {
                UInputModifierSwizzleAxis* Mod = NewObject<UInputModifierSwizzleAxis>(Context);
                Mod->Order = EInputAxisSwizzle::YXZ;
                Mapping.Modifiers.Add(Mod);
            }
            else if (ModName == TEXT("DeadZone"))
            {
                UInputModifierDeadZone* Mod = NewObject<UInputModifierDeadZone>(Context);
                Mapping.Modifiers.Add(Mod);
            }
        }
    }

    // 保存
    Context->MarkPackageDirty();
    FString PackagePath = FString::Printf(TEXT("%s/%s"), *ContextPath, *ContextName);
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Context->GetOutermost(), Context, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("context"), ContextName);
    ResultObj->SetStringField(TEXT("action"), ActionName);
    ResultObj->SetStringField(TEXT("key"), KeyName);
    ResultObj->SetStringField(TEXT("trigger"), TriggerType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleDeleteAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    // アセットが存在するか確認
    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    // アセット削除
    bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetPath);

    if (!bDeleted)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to delete asset: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("deleted"), AssetPath);
    return ResultObj;
} 