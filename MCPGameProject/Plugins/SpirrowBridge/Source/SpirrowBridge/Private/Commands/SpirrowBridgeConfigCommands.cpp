// SpirrowBridgeConfigCommands.cpp
#include "Commands/SpirrowBridgeConfigCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

FSpirrowBridgeConfigCommands::FSpirrowBridgeConfigCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeConfigCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("get_config_value"))
    {
        return HandleGetConfigValue(Params);
    }
    else if (CommandType == TEXT("set_config_value"))
    {
        return HandleSetConfigValue(Params);
    }
    else if (CommandType == TEXT("list_config_sections"))
    {
        return HandleListConfigSections(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Unknown config command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FSpirrowBridgeConfigCommands::HandleGetConfigValue(const TSharedPtr<FJsonObject>& Params)
{
    FString ConfigFile;
    if (!Params->TryGetStringField(TEXT("config_file"), ConfigFile))
    {
        ConfigFile = TEXT("DefaultEngine");
    }

    FString Section;
    if (!Params->TryGetStringField(TEXT("section"), Section))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'section' parameter"));
    }

    FString Key;
    if (!Params->TryGetStringField(TEXT("key"), Key))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Determine which config file to use
    FString ConfigFilePath;
    if (ConfigFile == TEXT("DefaultEngine") || ConfigFile == TEXT("Engine"))
    {
        ConfigFilePath = GEngineIni;
    }
    else if (ConfigFile == TEXT("DefaultGame") || ConfigFile == TEXT("Game"))
    {
        ConfigFilePath = GGameIni;
    }
    else if (ConfigFile == TEXT("DefaultEditor") || ConfigFile == TEXT("Editor"))
    {
        ConfigFilePath = GEditorIni;
    }
    else if (ConfigFile == TEXT("DefaultInput") || ConfigFile == TEXT("Input"))
    {
        ConfigFilePath = GInputIni;
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown config file: %s. Use DefaultEngine, DefaultGame, DefaultEditor, or DefaultInput"), *ConfigFile));
    }

    FString Value;
    if (GConfig->GetString(*Section, *Key, Value, ConfigFilePath))
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("config_file"), ConfigFile);
        ResultObj->SetStringField(TEXT("section"), Section);
        ResultObj->SetStringField(TEXT("key"), Key);
        ResultObj->SetStringField(TEXT("value"), Value);
        return ResultObj;
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Key not found: [%s] %s"), *Section, *Key));
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeConfigCommands::HandleSetConfigValue(const TSharedPtr<FJsonObject>& Params)
{
    FString ConfigFile;
    if (!Params->TryGetStringField(TEXT("config_file"), ConfigFile))
    {
        ConfigFile = TEXT("DefaultEngine");
    }

    FString Section;
    if (!Params->TryGetStringField(TEXT("section"), Section))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'section' parameter"));
    }

    FString Key;
    if (!Params->TryGetStringField(TEXT("key"), Key))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    FString Value;
    if (!Params->TryGetStringField(TEXT("value"), Value))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    // Build the actual file path in project Config folder
    FString FileName;
    if (ConfigFile == TEXT("DefaultEngine") || ConfigFile == TEXT("Engine"))
    {
        FileName = TEXT("DefaultEngine.ini");
    }
    else if (ConfigFile == TEXT("DefaultGame") || ConfigFile == TEXT("Game"))
    {
        FileName = TEXT("DefaultGame.ini");
    }
    else if (ConfigFile == TEXT("DefaultEditor") || ConfigFile == TEXT("Editor"))
    {
        FileName = TEXT("DefaultEditor.ini");
    }
    else if (ConfigFile == TEXT("DefaultInput") || ConfigFile == TEXT("Input"))
    {
        FileName = TEXT("DefaultInput.ini");
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown config file: %s"), *ConfigFile));
    }

    FString FilePath = FPaths::ProjectConfigDir() / FileName;

    // Use FConfigFile to load, modify, and save
    FConfigFile ConfigFileObj;
    ConfigFileObj.Read(FilePath);

    // Set the value
    ConfigFileObj.SetString(*Section, *Key, *Value);

    // Write back to disk
    ConfigFileObj.Write(FilePath);

    // Also update in-memory GConfig cache
    FString GConfigPath;
    if (ConfigFile == TEXT("DefaultEngine") || ConfigFile == TEXT("Engine"))
    {
        GConfigPath = GEngineIni;
    }
    else if (ConfigFile == TEXT("DefaultGame") || ConfigFile == TEXT("Game"))
    {
        GConfigPath = GGameIni;
    }
    else if (ConfigFile == TEXT("DefaultEditor") || ConfigFile == TEXT("Editor"))
    {
        GConfigPath = GEditorIni;
    }
    else if (ConfigFile == TEXT("DefaultInput") || ConfigFile == TEXT("Input"))
    {
        GConfigPath = GInputIni;
    }
    GConfig->SetString(*Section, *Key, *Value, GConfigPath);

    UE_LOG(LogTemp, Display, TEXT("Set config value: [%s] %s = %s in %s"),
           *Section, *Key, *Value, *FilePath);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("config_file"), ConfigFile);
    ResultObj->SetStringField(TEXT("section"), Section);
    ResultObj->SetStringField(TEXT("key"), Key);
    ResultObj->SetStringField(TEXT("value"), Value);
    ResultObj->SetStringField(TEXT("file_path"), FilePath);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeConfigCommands::HandleListConfigSections(const TSharedPtr<FJsonObject>& Params)
{
    FString ConfigFile;
    if (!Params->TryGetStringField(TEXT("config_file"), ConfigFile))
    {
        ConfigFile = TEXT("DefaultEngine");
    }

    // Determine which config file to use
    FString ConfigFilePath;
    if (ConfigFile == TEXT("DefaultEngine") || ConfigFile == TEXT("Engine"))
    {
        ConfigFilePath = GEngineIni;
    }
    else if (ConfigFile == TEXT("DefaultGame") || ConfigFile == TEXT("Game"))
    {
        ConfigFilePath = GGameIni;
    }
    else if (ConfigFile == TEXT("DefaultEditor") || ConfigFile == TEXT("Editor"))
    {
        ConfigFilePath = GEditorIni;
    }
    else if (ConfigFile == TEXT("DefaultInput") || ConfigFile == TEXT("Input"))
    {
        ConfigFilePath = GInputIni;
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unknown config file: %s"), *ConfigFile));
    }

    TArray<FString> SectionNames;
    GConfig->GetSectionNames(ConfigFilePath, SectionNames);

    TArray<TSharedPtr<FJsonValue>> SectionsArray;
    for (const FString& SectionName : SectionNames)
    {
        SectionsArray.Add(MakeShared<FJsonValueString>(SectionName));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("config_file"), ConfigFile);
    ResultObj->SetArrayField(TEXT("sections"), SectionsArray);
    ResultObj->SetNumberField(TEXT("count"), SectionNames.Num());
    return ResultObj;
}
