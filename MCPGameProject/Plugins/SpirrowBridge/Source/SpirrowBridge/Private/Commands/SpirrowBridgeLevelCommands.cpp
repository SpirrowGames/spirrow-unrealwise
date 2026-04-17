#include "Commands/SpirrowBridgeLevelCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "EditorLevelLibrary.h"
#include "EditorAssetLibrary.h"
#include "FileHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/WorldSettings.h"

namespace
{
    const TCHAR* const EmptyTemplatePath = TEXT("/Engine/Maps/Templates/Empty");

    bool ContainsInvalidChar(const FString& Name)
    {
        const TCHAR* Invalid = TEXT(" /\\\t\r\n:*?\"<>|");
        for (int32 i = 0; Invalid[i] != 0; ++i)
        {
            if (Name.Contains(FString(1, &Invalid[i])))
            {
                return true;
            }
        }
        return false;
    }

    FString NormalizeFolderPath(const FString& Path)
    {
        FString Result = Path;
        while (Result.EndsWith(TEXT("/")) && Result.Len() > 1)
        {
            Result.LeftChopInline(1);
        }
        return Result;
    }
}

FSpirrowBridgeLevelCommands::FSpirrowBridgeLevelCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeLevelCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_level"))
    {
        return HandleCreateLevel(Params);
    }
    if (CommandType == TEXT("save_current_level"))
    {
        return HandleSaveCurrentLevel(Params);
    }
    if (CommandType == TEXT("open_level"))
    {
        return HandleOpenLevel(Params);
    }
    if (CommandType == TEXT("get_world_settings"))
    {
        return HandleGetWorldSettings(Params);
    }
    if (CommandType == TEXT("set_world_properties"))
    {
        return HandleSetWorldProperties(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown level command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FSpirrowBridgeLevelCommands::HandleCreateLevel(const TSharedPtr<FJsonObject>& Params)
{
    // Required: name
    FString Name;
    if (!Params->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'name'"));
    }
    if (ContainsInvalidChar(Name))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Invalid level name '%s' (must not contain spaces, slashes or reserved chars)"), *Name));
    }

    // Optional: path (default /Game/Maps)
    FString Path = TEXT("/Game/Maps");
    Params->TryGetStringField(TEXT("path"), Path);
    Path = NormalizeFolderPath(Path);
    if (!Path.StartsWith(TEXT("/Game/")) && Path != TEXT("/Game"))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("'path' must start with /Game/ (got '%s')"), *Path));
    }

    // Optional: template (default "default")
    FString Template = TEXT("default");
    Params->TryGetStringField(TEXT("template"), Template);

    // Optional: overwrite (default false)
    bool bOverwrite = false;
    Params->TryGetBoolField(TEXT("overwrite"), bOverwrite);

    const FString AssetPath = FString::Printf(TEXT("%s/%s"), *Path, *Name);

    // Check existence
    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        if (!bOverwrite)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Level already exists at '%s' (pass overwrite=true to replace)"), *AssetPath));
        }
        if (!UEditorAssetLibrary::DeleteAsset(AssetPath))
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Failed to delete existing level at '%s' for overwrite"), *AssetPath));
        }
    }

    // Resolve template
    FString TemplatePath;  // empty = use NewLevel (default UE 5.7 behavior = WP)
    const FString TemplateLower = Template.ToLower();
    if (TemplateLower == TEXT("default"))
    {
        TemplatePath = TEXT("");
    }
    else if (TemplateLower == TEXT("empty"))
    {
        TemplatePath = EmptyTemplatePath;
    }
    else if (Template.StartsWith(TEXT("/")))
    {
        // Explicit template asset path
        if (!UEditorAssetLibrary::DoesAssetExist(Template))
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Template level not found at '%s'"), *Template));
        }
        TemplatePath = Template;
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown template '%s' (expected 'default', 'empty' or /Game/... path)"), *Template));
    }

    // Create the level (NewLevel / NewLevelFromTemplate closes current level and switches to the new one)
    bool bSuccess = false;
    if (TemplatePath.IsEmpty())
    {
        bSuccess = UEditorLevelLibrary::NewLevel(AssetPath);
    }
    else
    {
        bSuccess = UEditorLevelLibrary::NewLevelFromTemplate(AssetPath, TemplatePath);
    }

    if (!bSuccess)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("UEditorLevelLibrary failed to create level at '%s' (template='%s')"),
                            *AssetPath, *Template));
    }

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("level_path"), AssetPath);
    Data->SetStringField(TEXT("template_used"), Template);
    if (!TemplatePath.IsEmpty())
    {
        Data->SetStringField(TEXT("template_path"), TemplatePath);
    }
    Data->SetBoolField(TEXT("switched_editor"), true);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgeLevelCommands::HandleSaveCurrentLevel(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World || !World->PersistentLevel)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No editor world / persistent level available"));
    }

    const bool bSuccess = UEditorLoadingAndSavingUtils::SaveCurrentLevel();
    if (!bSuccess)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("SaveCurrentLevel returned false (save dialog cancelled or write failed)"));
    }

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    if (UPackage* Pkg = World->PersistentLevel->GetOutermost())
    {
        Data->SetStringField(TEXT("level_package"), Pkg->GetName());
    }
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgeLevelCommands::HandleOpenLevel(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("level_path"), AssetPath) || AssetPath.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'level_path'"));
    }
    if (!AssetPath.StartsWith(TEXT("/")))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("'level_path' must be an asset path starting with / (got '%s')"), *AssetPath));
    }
    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Level not found at '%s'"), *AssetPath));
    }

    const bool bSuccess = UEditorLevelLibrary::LoadLevel(AssetPath);
    if (!bSuccess)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("UEditorLevelLibrary::LoadLevel failed for '%s' (user may have cancelled save-prompt)"), *AssetPath));
    }

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("level_path"), AssetPath);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

namespace
{
    const TArray<FString>& GetWorldSettingsCuratedPreset()
    {
        static const TArray<FString> Preset = {
            TEXT("DefaultGameMode"),          // shown as "GameMode Override" in UI
            TEXT("DefaultPhysicsVolumeClass"),
            TEXT("KillZ"),
            TEXT("KillZDamageType"),
            TEXT("WorldToMeters"),
            TEXT("GlobalGravityZ"),
            TEXT("TimeDilation"),
            TEXT("bEnableWorldBoundsChecks"),
            TEXT("bEnableWorldComposition"),
        };
        return Preset;
    }

    void SetPropertyValueOnJson(TSharedPtr<FJsonObject>& Obj, const FString& Name,
                                FProperty* Prop, UObject* Owner)
    {
        const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Owner);

        if (auto* BoolProp = CastField<FBoolProperty>(Prop))
        {
            Obj->SetBoolField(Name, BoolProp->GetPropertyValue(ValuePtr));
            return;
        }
        if (auto* FloatProp = CastField<FFloatProperty>(Prop))
        {
            Obj->SetNumberField(Name, FloatProp->GetPropertyValue(ValuePtr));
            return;
        }
        if (auto* DoubleProp = CastField<FDoubleProperty>(Prop))
        {
            Obj->SetNumberField(Name, DoubleProp->GetPropertyValue(ValuePtr));
            return;
        }
        if (auto* IntProp = CastField<FIntProperty>(Prop))
        {
            Obj->SetNumberField(Name, IntProp->GetPropertyValue(ValuePtr));
            return;
        }
        if (auto* Int64Prop = CastField<FInt64Property>(Prop))
        {
            Obj->SetNumberField(Name, static_cast<double>(Int64Prop->GetPropertyValue(ValuePtr)));
            return;
        }
        if (auto* NameProp = CastField<FNameProperty>(Prop))
        {
            Obj->SetStringField(Name, NameProp->GetPropertyValue(ValuePtr).ToString());
            return;
        }
        if (auto* StrProp = CastField<FStrProperty>(Prop))
        {
            Obj->SetStringField(Name, StrProp->GetPropertyValue(ValuePtr));
            return;
        }
        // Fallback for class / object / struct / enum: export as text
        FString Exported;
        Prop->ExportTextItem_Direct(Exported, ValuePtr, nullptr, Owner, PPF_None);
        Obj->SetStringField(Name, Exported);
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeLevelCommands::HandleGetWorldSettings(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    AWorldSettings* Settings = World ? World->GetWorldSettings() : nullptr;
    if (!Settings)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No editor world / WorldSettings available"));
    }

    TArray<FString> PropertyNames;
    const TArray<TSharedPtr<FJsonValue>>* PropsArray = nullptr;
    if (Params.IsValid() && Params->TryGetArrayField(TEXT("properties"), PropsArray))
    {
        for (const TSharedPtr<FJsonValue>& V : *PropsArray)
        {
            FString S;
            if (V.IsValid() && V->TryGetString(S) && !S.IsEmpty())
            {
                PropertyNames.Add(S);
            }
        }
        if (PropertyNames.Num() == 0)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("'properties' must be a non-empty array of strings if provided"));
        }
    }
    else
    {
        PropertyNames = GetWorldSettingsCuratedPreset();
    }

    TSharedPtr<FJsonObject> PropsObj = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>> UnknownArr;
    UClass* Cls = Settings->GetClass();

    for (const FString& Name : PropertyNames)
    {
        FProperty* Prop = Cls->FindPropertyByName(*Name);
        if (!Prop)
        {
            UnknownArr.Add(MakeShared<FJsonValueString>(Name));
            continue;
        }
        SetPropertyValueOnJson(PropsObj, Name, Prop, Settings);
    }

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    if (World->PersistentLevel)
    {
        if (UPackage* Pkg = World->PersistentLevel->GetOutermost())
        {
            Data->SetStringField(TEXT("level_package"), Pkg->GetName());
        }
    }
    Data->SetObjectField(TEXT("properties"), PropsObj);
    Data->SetArrayField(TEXT("unknown_properties"), UnknownArr);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgeLevelCommands::HandleSetWorldProperties(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    AWorldSettings* Settings = World ? World->GetWorldSettings() : nullptr;
    if (!Settings)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No editor world / WorldSettings available"));
    }

    const TSharedPtr<FJsonObject>* PropsObjPtr = nullptr;
    if (!Params->TryGetObjectField(TEXT("properties"), PropsObjPtr) || !PropsObjPtr || !(*PropsObjPtr).IsValid()
        || (*PropsObjPtr)->Values.Num() == 0)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing or empty 'properties' object (expect dict of name -> value)"));
    }

    TArray<TSharedPtr<FJsonValue>> AppliedArr;
    TArray<TSharedPtr<FJsonValue>> FailedArr;

    for (const auto& Pair : (*PropsObjPtr)->Values)
    {
        FString ErrMsg;
        if (FSpirrowBridgeCommonUtils::SetObjectProperty(Settings, Pair.Key, Pair.Value, ErrMsg))
        {
            AppliedArr.Add(MakeShared<FJsonValueString>(Pair.Key));
        }
        else
        {
            TSharedPtr<FJsonObject> FailObj = MakeShareable(new FJsonObject);
            FailObj->SetStringField(TEXT("property"), Pair.Key);
            FailObj->SetStringField(TEXT("error"), ErrMsg);
            FailedArr.Add(MakeShared<FJsonValueObject>(FailObj));
        }
    }

    if (AppliedArr.Num() > 0)
    {
        Settings->MarkPackageDirty();
        if (World->PersistentLevel)
        {
            World->PersistentLevel->MarkPackageDirty();
        }
    }

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    if (World->PersistentLevel)
    {
        if (UPackage* Pkg = World->PersistentLevel->GetOutermost())
        {
            Data->SetStringField(TEXT("level_package"), Pkg->GetName());
        }
    }
    Data->SetArrayField(TEXT("applied"), AppliedArr);
    Data->SetArrayField(TEXT("failed"), FailedArr);

    if (AppliedArr.Num() == 0 && FailedArr.Num() > 0)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            FString::Printf(TEXT("All %d properties failed to set"), FailedArr.Num()),
            Data);
    }
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
