#include "Commands/SpirrowBridgeLevelCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "EditorLevelLibrary.h"
#include "EditorAssetLibrary.h"
#include "FileHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Level.h"

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
