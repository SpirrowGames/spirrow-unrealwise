#include "Commands/SpirrowBridgeGASCommands.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "UObject/SavePackage.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"

FSpirrowBridgeGASCommands::FSpirrowBridgeGASCommands()
{
}

FSpirrowBridgeGASCommands::~FSpirrowBridgeGASCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("add_gameplay_tags"))
    {
        return HandleAddGameplayTags(Params);
    }
    else if (CommandType == TEXT("list_gameplay_tags"))
    {
        return HandleListGameplayTags(Params);
    }
    else if (CommandType == TEXT("remove_gameplay_tag"))
    {
        return HandleRemoveGameplayTag(Params);
    }
    else if (CommandType == TEXT("list_gas_assets"))
    {
        return HandleListGASAssets(Params);
    }
    else if (CommandType == TEXT("create_gameplay_effect"))
    {
        return HandleCreateGameplayEffect(Params);
    }
    else if (CommandType == TEXT("create_gas_character"))
    {
        return HandleCreateGASCharacter(Params);
    }
    else if (CommandType == TEXT("set_ability_system_defaults"))
    {
        return HandleSetAbilitySystemDefaults(Params);
    }

    TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject);
    ErrorResponse->SetBoolField(TEXT("success"), false);
    ErrorResponse->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown GAS command: %s"), *CommandType));
    return ErrorResponse;
}

FString FSpirrowBridgeGASCommands::GetGameplayTagsConfigPath() const
{
    return FPaths::ProjectConfigDir() / TEXT("DefaultGameplayTags.ini");
}

TArray<TPair<FString, FString>> FSpirrowBridgeGASCommands::ParseExistingTags(const FString& ConfigPath)
{
    TArray<TPair<FString, FString>> ExistingTags;

    FString FileContent;
    if (!FFileHelper::LoadFileToString(FileContent, *ConfigPath))
    {
        return ExistingTags;
    }

    // Parse lines looking for GameplayTagList entries
    // Format: +GameplayTagList=(Tag="TagName",DevComment="Comment")
    TArray<FString> Lines;
    FileContent.ParseIntoArrayLines(Lines);

    for (const FString& Line : Lines)
    {
        if (Line.Contains(TEXT("GameplayTagList=")))
        {
            // Extract Tag value
            FString TagValue;
            FString CommentValue;

            // Find Tag="..."
            int32 TagStart = Line.Find(TEXT("Tag=\""));
            if (TagStart != INDEX_NONE)
            {
                TagStart += 5; // Length of 'Tag="'
                int32 TagEnd = Line.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, TagStart);
                if (TagEnd != INDEX_NONE)
                {
                    TagValue = Line.Mid(TagStart, TagEnd - TagStart);
                }
            }

            // Find DevComment="..."
            int32 CommentStart = Line.Find(TEXT("DevComment=\""));
            if (CommentStart != INDEX_NONE)
            {
                CommentStart += 12; // Length of 'DevComment="'
                int32 CommentEnd = Line.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, CommentStart);
                if (CommentEnd != INDEX_NONE)
                {
                    CommentValue = Line.Mid(CommentStart, CommentEnd - CommentStart);
                }
            }

            if (!TagValue.IsEmpty())
            {
                ExistingTags.Add(TPair<FString, FString>(TagValue, CommentValue));
            }
        }
    }

    return ExistingTags;
}

bool FSpirrowBridgeGASCommands::WriteTagsToConfig(const FString& ConfigPath, const TArray<TPair<FString, FString>>& Tags)
{
    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *ConfigPath);

    // Find or create the GameplayTags section
    const FString SectionName = TEXT("[/Script/GameplayTags.GameplayTagsSettings]");

    // Remove existing GameplayTagList entries
    TArray<FString> Lines;
    FileContent.ParseIntoArrayLines(Lines);

    TArray<FString> NewLines;
    bool bInSection = false;
    bool bSectionFound = false;

    for (const FString& Line : Lines)
    {
        if (Line.StartsWith(TEXT("[")))
        {
            bInSection = Line.Equals(SectionName);
            if (bInSection)
            {
                bSectionFound = true;
            }
        }

        // Skip existing GameplayTagList entries in our section
        if (bInSection && Line.Contains(TEXT("GameplayTagList=")))
        {
            continue;
        }

        NewLines.Add(Line);
    }

    // If section doesn't exist, add it
    if (!bSectionFound)
    {
        NewLines.Add(TEXT(""));
        NewLines.Add(SectionName);
    }

    // Find where to insert new tags (after section header)
    int32 InsertIndex = NewLines.IndexOfByPredicate([&SectionName](const FString& Line) {
        return Line.Equals(SectionName);
    });

    if (InsertIndex != INDEX_NONE)
    {
        InsertIndex++; // Insert after section header

        // Add all tags
        for (const auto& TagPair : Tags)
        {
            FString TagLine;
            if (TagPair.Value.IsEmpty())
            {
                TagLine = FString::Printf(TEXT("+GameplayTagList=(Tag=\"%s\")"), *TagPair.Key);
            }
            else
            {
                TagLine = FString::Printf(TEXT("+GameplayTagList=(Tag=\"%s\",DevComment=\"%s\")"), *TagPair.Key, *TagPair.Value);
            }
            NewLines.Insert(TagLine, InsertIndex++);
        }
    }

    // Write back to file
    FString NewContent = FString::Join(NewLines, TEXT("\n"));
    return FFileHelper::SaveStringToFile(NewContent, *ConfigPath);
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleAddGameplayTags(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);

    // Get tags array from params
    const TArray<TSharedPtr<FJsonValue>>* TagsArray;
    if (!Params->TryGetArrayField(TEXT("tags"), TagsArray))
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Missing 'tags' parameter"));
        return Response;
    }

    // Get optional comments
    const TSharedPtr<FJsonObject>* CommentsObj = nullptr;
    Params->TryGetObjectField(TEXT("comments"), CommentsObj);

    FString ConfigPath = GetGameplayTagsConfigPath();

    // Parse existing tags
    TArray<TPair<FString, FString>> ExistingTags = ParseExistingTags(ConfigPath);
    TSet<FString> ExistingTagSet;
    for (const auto& TagPair : ExistingTags)
    {
        ExistingTagSet.Add(TagPair.Key);
    }

    // Process new tags
    TArray<FString> AddedTags;
    TArray<FString> SkippedTags;

    for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
    {
        FString Tag = TagValue->AsString();

        if (ExistingTagSet.Contains(Tag))
        {
            SkippedTags.Add(Tag);
            continue;
        }

        // Get comment if provided
        FString Comment;
        if (CommentsObj && (*CommentsObj)->HasField(Tag))
        {
            Comment = (*CommentsObj)->GetStringField(Tag);
        }

        ExistingTags.Add(TPair<FString, FString>(Tag, Comment));
        ExistingTagSet.Add(Tag);
        AddedTags.Add(Tag);
    }

    // Write back to config
    if (AddedTags.Num() > 0)
    {
        if (!WriteTagsToConfig(ConfigPath, ExistingTags))
        {
            Response->SetBoolField(TEXT("success"), false);
            Response->SetStringField(TEXT("error"), TEXT("Failed to write config file"));
            return Response;
        }
    }

    // Build response
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("file_path"), ConfigPath);

    TArray<TSharedPtr<FJsonValue>> AddedArray;
    for (const FString& Tag : AddedTags)
    {
        AddedArray.Add(MakeShareable(new FJsonValueString(Tag)));
    }
    Response->SetArrayField(TEXT("added_tags"), AddedArray);

    TArray<TSharedPtr<FJsonValue>> SkippedArray;
    for (const FString& Tag : SkippedTags)
    {
        SkippedArray.Add(MakeShareable(new FJsonValueString(Tag)));
    }
    Response->SetArrayField(TEXT("skipped_tags"), SkippedArray);

    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Added %d gameplay tags, skipped %d existing"), AddedTags.Num(), SkippedTags.Num());

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleListGameplayTags(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);

    FString FilterPrefix = Params->GetStringField(TEXT("filter_prefix"));
    FString ConfigPath = GetGameplayTagsConfigPath();

    TArray<TPair<FString, FString>> AllTags = ParseExistingTags(ConfigPath);

    TArray<TSharedPtr<FJsonValue>> TagsArray;
    for (const auto& TagPair : AllTags)
    {
        // Apply filter if specified
        if (!FilterPrefix.IsEmpty() && !TagPair.Key.StartsWith(FilterPrefix))
        {
            continue;
        }

        TSharedPtr<FJsonObject> TagObj = MakeShareable(new FJsonObject);
        TagObj->SetStringField(TEXT("tag"), TagPair.Key);
        TagObj->SetStringField(TEXT("comment"), TagPair.Value);
        TagsArray.Add(MakeShareable(new FJsonValueObject(TagObj)));
    }

    Response->SetBoolField(TEXT("success"), true);
    Response->SetArrayField(TEXT("tags"), TagsArray);
    Response->SetNumberField(TEXT("count"), TagsArray.Num());
    Response->SetStringField(TEXT("file_path"), ConfigPath);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleRemoveGameplayTag(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);

    FString TagToRemove = Params->GetStringField(TEXT("tag"));
    if (TagToRemove.IsEmpty())
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Missing 'tag' parameter"));
        return Response;
    }

    FString ConfigPath = GetGameplayTagsConfigPath();
    TArray<TPair<FString, FString>> ExistingTags = ParseExistingTags(ConfigPath);

    bool bFound = false;
    TArray<TPair<FString, FString>> NewTags;
    for (const auto& TagPair : ExistingTags)
    {
        if (TagPair.Key.Equals(TagToRemove))
        {
            bFound = true;
            continue;
        }
        NewTags.Add(TagPair);
    }

    if (bFound)
    {
        if (!WriteTagsToConfig(ConfigPath, NewTags))
        {
            Response->SetBoolField(TEXT("success"), false);
            Response->SetStringField(TEXT("error"), TEXT("Failed to write config file"));
            return Response;
        }
    }

    Response->SetBoolField(TEXT("success"), true);
    Response->SetBoolField(TEXT("removed"), bFound);
    Response->SetStringField(TEXT("message"), bFound ? FString::Printf(TEXT("Removed tag: %s"), *TagToRemove) : FString::Printf(TEXT("Tag not found: %s"), *TagToRemove));
    Response->SetStringField(TEXT("file_path"), ConfigPath);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleListGASAssets(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);

    FString AssetType = Params->GetStringField(TEXT("asset_type"));
    FString PathFilter = Params->GetStringField(TEXT("path_filter"));

    // Get Asset Registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Results arrays
    TArray<TSharedPtr<FJsonValue>> EffectsArray;
    TArray<TSharedPtr<FJsonValue>> AbilitiesArray;
    TArray<TSharedPtr<FJsonValue>> CuesArray;
    TArray<TSharedPtr<FJsonValue>> AttributeSetsArray;

    // Helper lambda to create asset info JSON
    auto CreateAssetInfo = [](const FAssetData& Asset) -> TSharedPtr<FJsonObject> {
        TSharedPtr<FJsonObject> AssetObj = MakeShareable(new FJsonObject);
        AssetObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
        AssetObj->SetStringField(TEXT("class"), Asset.AssetClassPath.GetAssetName().ToString());
        return AssetObj;
    };

    // Search for Blueprint assets
    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint")));
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;

    if (!PathFilter.IsEmpty())
    {
        Filter.PackagePaths.Add(FName(*PathFilter));
        Filter.bRecursivePaths = true;
    }

    TArray<FAssetData> BlueprintAssets;
    AssetRegistry.GetAssets(Filter, BlueprintAssets);

    for (const FAssetData& Asset : BlueprintAssets)
    {
        // Load the blueprint to check its parent class
        UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
        if (!Blueprint || !Blueprint->GeneratedClass)
        {
            continue;
        }

        UClass* ParentClass = Blueprint->GeneratedClass->GetSuperClass();
        if (!ParentClass)
        {
            continue;
        }

        FString ParentClassName = ParentClass->GetName();

        // Check if it's a GameplayEffect
        bool bIsEffect = false;
        bool bIsAbility = false;
        bool bIsCue = false;
        bool bIsAttributeSet = false;

        // Walk up the class hierarchy to find GAS base classes
        UClass* CurrentClass = ParentClass;
        while (CurrentClass)
        {
            FString ClassName = CurrentClass->GetName();

            if (ClassName.Contains(TEXT("GameplayEffect")))
            {
                bIsEffect = true;
                break;
            }
            else if (ClassName.Contains(TEXT("GameplayAbility")))
            {
                bIsAbility = true;
                break;
            }
            else if (ClassName.Contains(TEXT("GameplayCueNotify")))
            {
                bIsCue = true;
                break;
            }
            else if (ClassName.Contains(TEXT("AttributeSet")))
            {
                bIsAttributeSet = true;
                break;
            }

            CurrentClass = CurrentClass->GetSuperClass();
        }

        // Add to appropriate array based on type filter
        if (bIsEffect && (AssetType == TEXT("all") || AssetType == TEXT("effect")))
        {
            TSharedPtr<FJsonObject> AssetObj = CreateAssetInfo(Asset);
            AssetObj->SetStringField(TEXT("type"), TEXT("GameplayEffect"));
            AssetObj->SetStringField(TEXT("parent_class"), ParentClassName);
            EffectsArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
        }
        else if (bIsAbility && (AssetType == TEXT("all") || AssetType == TEXT("ability")))
        {
            TSharedPtr<FJsonObject> AssetObj = CreateAssetInfo(Asset);
            AssetObj->SetStringField(TEXT("type"), TEXT("GameplayAbility"));
            AssetObj->SetStringField(TEXT("parent_class"), ParentClassName);
            AbilitiesArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
        }
        else if (bIsCue && (AssetType == TEXT("all") || AssetType == TEXT("cue")))
        {
            TSharedPtr<FJsonObject> AssetObj = CreateAssetInfo(Asset);
            AssetObj->SetStringField(TEXT("type"), TEXT("GameplayCue"));
            AssetObj->SetStringField(TEXT("parent_class"), ParentClassName);
            CuesArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
        }
        else if (bIsAttributeSet && (AssetType == TEXT("all") || AssetType == TEXT("attribute_set")))
        {
            TSharedPtr<FJsonObject> AssetObj = CreateAssetInfo(Asset);
            AssetObj->SetStringField(TEXT("type"), TEXT("AttributeSet"));
            AssetObj->SetStringField(TEXT("parent_class"), ParentClassName);
            AttributeSetsArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
        }
    }

    // Build response
    Response->SetBoolField(TEXT("success"), true);
    Response->SetArrayField(TEXT("effects"), EffectsArray);
    Response->SetArrayField(TEXT("abilities"), AbilitiesArray);
    Response->SetArrayField(TEXT("cues"), CuesArray);
    Response->SetArrayField(TEXT("attribute_sets"), AttributeSetsArray);

    int32 TotalCount = EffectsArray.Num() + AbilitiesArray.Num() + CuesArray.Num() + AttributeSetsArray.Num();
    Response->SetNumberField(TEXT("total_count"), TotalCount);

    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Found %d GAS assets (Effects: %d, Abilities: %d, Cues: %d, AttributeSets: %d)"),
        TotalCount, EffectsArray.Num(), AbilitiesArray.Num(), CuesArray.Num(), AttributeSetsArray.Num());

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleCreateGameplayEffect(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);

    // Get parameters
    FString Name = Params->GetStringField(TEXT("name"));
    FString DurationPolicyStr = Params->GetStringField(TEXT("duration_policy"));
    double DurationMagnitude = Params->GetNumberField(TEXT("duration_magnitude"));
    double Period = Params->GetNumberField(TEXT("period"));
    FString Path = Params->GetStringField(TEXT("path"));

    const TArray<TSharedPtr<FJsonValue>>* ModifiersArray = nullptr;
    Params->TryGetArrayField(TEXT("modifiers"), ModifiersArray);

    const TArray<TSharedPtr<FJsonValue>>* ApplicationTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("application_tags"), ApplicationTagsArray);

    const TArray<TSharedPtr<FJsonValue>>* RemovalTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("removal_tags"), RemovalTagsArray);

    // Validate name
    if (Name.IsEmpty())
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Missing 'name' parameter"));
        return Response;
    }

    // Construct full asset path
    FString PackagePath = Path / Name;
    FString AssetPath = PackagePath + TEXT(".") + Name;

    // Check if asset already exists
    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Asset already exists: %s"), *AssetPath));
        return Response;
    }

    // Create the package
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to create package"));
        return Response;
    }

    // Create Blueprint factory
    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    Factory->ParentClass = UGameplayEffect::StaticClass();

    // Create the Blueprint asset
    UBlueprint* Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
        UBlueprint::StaticClass(),
        Package,
        FName(*Name),
        RF_Public | RF_Standalone,
        nullptr,
        GWarn
    ));

    if (!Blueprint)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to create GameplayEffect Blueprint"));
        return Response;
    }

    // Compile the blueprint to generate the class
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Get the CDO (Class Default Object) to set properties
    UGameplayEffect* EffectCDO = Cast<UGameplayEffect>(Blueprint->GeneratedClass->GetDefaultObject());
    if (!EffectCDO)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to get GameplayEffect CDO"));
        return Response;
    }

    // Set Duration Policy
    if (DurationPolicyStr == TEXT("Instant"))
    {
        EffectCDO->DurationPolicy = EGameplayEffectDurationType::Instant;
    }
    else if (DurationPolicyStr == TEXT("HasDuration"))
    {
        EffectCDO->DurationPolicy = EGameplayEffectDurationType::HasDuration;
        // Set duration magnitude
        EffectCDO->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationMagnitude));
    }
    else if (DurationPolicyStr == TEXT("Infinite"))
    {
        EffectCDO->DurationPolicy = EGameplayEffectDurationType::Infinite;
    }

    // Set Period if specified
    if (Period > 0.0)
    {
        EffectCDO->Period = FScalableFloat(Period);
    }

    // Set Modifiers
    if (ModifiersArray && ModifiersArray->Num() > 0)
    {
        for (const TSharedPtr<FJsonValue>& ModValue : *ModifiersArray)
        {
            const TSharedPtr<FJsonObject>* ModObj = nullptr;
            if (!ModValue->TryGetObject(ModObj) || !ModObj)
            {
                continue;
            }

            FString AttributeName = (*ModObj)->GetStringField(TEXT("attribute"));
            FString OperationStr = (*ModObj)->GetStringField(TEXT("operation"));
            double Magnitude = (*ModObj)->GetNumberField(TEXT("magnitude"));

            // Create modifier info
            FGameplayModifierInfo ModInfo;

            // Set operation
            if (OperationStr == TEXT("Add"))
            {
                ModInfo.ModifierOp = EGameplayModOp::Additive;
            }
            else if (OperationStr == TEXT("Multiply"))
            {
                ModInfo.ModifierOp = EGameplayModOp::Multiplicitive;
            }
            else if (OperationStr == TEXT("Divide"))
            {
                ModInfo.ModifierOp = EGameplayModOp::Division;
            }
            else if (OperationStr == TEXT("Override"))
            {
                ModInfo.ModifierOp = EGameplayModOp::Override;
            }
            else
            {
                ModInfo.ModifierOp = EGameplayModOp::Additive;
            }

            // Set magnitude
            ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));

            // Note: Setting the Attribute requires finding the actual FGameplayAttribute
            // For now, we store the attribute name in the modifier info
            // The actual attribute binding would need the AttributeSet class reference
            // This is a simplified implementation - full implementation would need AttributeSet lookup

            EffectCDO->Modifiers.Add(ModInfo);

            UE_LOG(LogTemp, Display, TEXT("Added modifier: %s %s %.2f"),
                *AttributeName, *OperationStr, Magnitude);
        }
    }

    // Set Application Tags (GrantedTags - tags granted while effect is active)
    if (ApplicationTagsArray && ApplicationTagsArray->Num() > 0)
    {
        for (const TSharedPtr<FJsonValue>& TagValue : *ApplicationTagsArray)
        {
            FString TagString = TagValue->AsString();
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
            if (Tag.IsValid())
            {
                EffectCDO->InheritableOwnedTagsContainer.AddTag(Tag);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid gameplay tag: %s"), *TagString);
            }
        }
    }

    // Set Removal Tags (RemoveGameplayEffectsWithTags - effects with these tags are removed)
    if (RemovalTagsArray && RemovalTagsArray->Num() > 0)
    {
        for (const TSharedPtr<FJsonValue>& TagValue : *RemovalTagsArray)
        {
            FString TagString = TagValue->AsString();
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
            if (Tag.IsValid())
            {
                EffectCDO->RemoveGameplayEffectsWithTags.Added.AddTag(Tag);
            }
        }
    }

    // Mark package dirty and save
    Package->MarkPackageDirty();
    Blueprint->MarkPackageDirty();

    // Compile again after setting properties
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Save the asset
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    bool bSaved = UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

    if (!bSaved)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to save GameplayEffect asset to disk, but asset was created in memory"));
    }

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(Blueprint);

    // Build response
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("name"), Name);
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("duration_policy"), DurationPolicyStr);
    Response->SetNumberField(TEXT("modifier_count"), ModifiersArray ? ModifiersArray->Num() : 0);

    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Created GameplayEffect '%s' at %s"), *Name, *AssetPath);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleCreateGASCharacter(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);

    // Extract parameters
    FString Name;
    if (!Params->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Missing or empty 'name' parameter"));
        return Response;
    }

    FString ParentClassStr;
    Params->TryGetStringField(TEXT("parent_class"), ParentClassStr);
    if (ParentClassStr.IsEmpty())
    {
        ParentClassStr = TEXT("Character");
    }

    FString ASCComponentName;
    Params->TryGetStringField(TEXT("asc_component_name"), ASCComponentName);
    if (ASCComponentName.IsEmpty())
    {
        ASCComponentName = TEXT("AbilitySystemComponent");
    }

    FString ReplicationMode;
    Params->TryGetStringField(TEXT("replication_mode"), ReplicationMode);
    if (ReplicationMode.IsEmpty())
    {
        ReplicationMode = TEXT("Mixed");
    }

    FString PathStr;
    Params->TryGetStringField(TEXT("path"), PathStr);
    if (PathStr.IsEmpty())
    {
        PathStr = TEXT("/Game/GAS/Characters");
    }

    // Get default abilities and effects arrays
    const TArray<TSharedPtr<FJsonValue>>* DefaultAbilitiesArray = nullptr;
    Params->TryGetArrayField(TEXT("default_abilities"), DefaultAbilitiesArray);

    const TArray<TSharedPtr<FJsonValue>>* DefaultEffectsArray = nullptr;
    Params->TryGetArrayField(TEXT("default_effects"), DefaultEffectsArray);

    // Find parent class
    UClass* ParentClass = nullptr;
    if (ParentClassStr == TEXT("Character"))
    {
        ParentClass = ACharacter::StaticClass();
    }
    else
    {
        ParentClass = FindObject<UClass>(nullptr, *ParentClassStr);
        if (!ParentClass)
        {
            Response->SetBoolField(TEXT("success"), false);
            Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Parent class not found: %s"), *ParentClassStr));
            return Response;
        }
    }

    // Create package and asset
    FString PackageName = PathStr / Name;
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to create package"));
        return Response;
    }

    // Create Blueprint
    UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass,
        Package,
        FName(*Name),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        NAME_None
    );

    if (!NewBlueprint)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to create Blueprint"));
        return Response;
    }

    // Add AbilitySystemComponent to the Blueprint
    USimpleConstructionScript* SCS = NewBlueprint->SimpleConstructionScript;
    if (!SCS)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Blueprint has no SimpleConstructionScript"));
        return Response;
    }

    USCS_Node* NewNode = SCS->CreateNode(UAbilitySystemComponent::StaticClass(), FName(*ASCComponentName));
    if (!NewNode)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to create AbilitySystemComponent node"));
        return Response;
    }

    SCS->AddNode(NewNode);
    UE_LOG(LogTemp, Log, TEXT("Added AbilitySystemComponent '%s' to Blueprint"), *ASCComponentName);

    // Configure ASC replication mode
    UAbilitySystemComponent* ASCTemplate = Cast<UAbilitySystemComponent>(NewNode->ComponentTemplate);
    if (ASCTemplate)
    {
        if (ReplicationMode == TEXT("Full"))
        {
            ASCTemplate->SetReplicationMode(EGameplayEffectReplicationMode::Full);
            UE_LOG(LogTemp, Log, TEXT("Set ASC replication mode to Full"));
        }
        else if (ReplicationMode == TEXT("Mixed"))
        {
            ASCTemplate->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
            UE_LOG(LogTemp, Log, TEXT("Set ASC replication mode to Mixed"));
        }
        else if (ReplicationMode == TEXT("Minimal"))
        {
            ASCTemplate->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
            UE_LOG(LogTemp, Log, TEXT("Set ASC replication mode to Minimal"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Unknown replication mode '%s', using default"), *ReplicationMode);
        }

        // Note: UAbilitySystemComponent doesn't have DefaultAbilities/DefaultEffects properties by default
        // These would typically be set in a custom ASC subclass or in BeginPlay
        // For now, we'll log them for reference
        if (DefaultAbilitiesArray && DefaultAbilitiesArray->Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Default abilities specified (%d abilities) - note these need to be granted in BeginPlay or custom ASC class"), DefaultAbilitiesArray->Num());
            for (const TSharedPtr<FJsonValue>& AbilityValue : *DefaultAbilitiesArray)
            {
                FString AbilityPath = AbilityValue->AsString();
                UE_LOG(LogTemp, Log, TEXT("  - %s"), *AbilityPath);
            }
        }

        if (DefaultEffectsArray && DefaultEffectsArray->Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Default effects specified (%d effects) - note these need to be applied in BeginPlay or custom ASC class"), DefaultEffectsArray->Num());
            for (const TSharedPtr<FJsonValue>& EffectValue : *DefaultEffectsArray)
            {
                FString EffectPath = EffectValue->AsString();
                UE_LOG(LogTemp, Log, TEXT("  - %s"), *EffectPath);
            }
        }
    }

    // Compile and save
    FKismetEditorUtilities::CompileBlueprint(NewBlueprint);

    FString AssetPath = PackageName + TEXT(".") + Name;
    FAssetRegistryModule::AssetCreated(NewBlueprint);
    Package->MarkPackageDirty();

    // Build response
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("name"), Name);
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("parent_class"), ParentClassStr);
    Response->SetStringField(TEXT("asc_component_name"), ASCComponentName);
    Response->SetStringField(TEXT("replication_mode"), ReplicationMode);

    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Created GAS Character Blueprint '%s' at %s with ASC"), *Name, *AssetPath);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleSetAbilitySystemDefaults(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);

    // Extract parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName) || BlueprintName.IsEmpty())
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Missing or empty 'blueprint_name' parameter"));
        return Response;
    }

    FString ASCComponentName;
    Params->TryGetStringField(TEXT("asc_component_name"), ASCComponentName);
    if (ASCComponentName.IsEmpty())
    {
        ASCComponentName = TEXT("AbilitySystemComponent");
    }

    FString PathStr;
    Params->TryGetStringField(TEXT("path"), PathStr);
    if (PathStr.IsEmpty())
    {
        PathStr = TEXT("/Game/Blueprints");
    }

    // Get default abilities and effects arrays
    const TArray<TSharedPtr<FJsonValue>>* DefaultAbilitiesArray = nullptr;
    Params->TryGetArrayField(TEXT("default_abilities"), DefaultAbilitiesArray);

    const TArray<TSharedPtr<FJsonValue>>* DefaultEffectsArray = nullptr;
    Params->TryGetArrayField(TEXT("default_effects"), DefaultEffectsArray);

    // Load Blueprint
    FString AssetPath = PathStr / BlueprintName + TEXT(".") + BlueprintName;
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!Blueprint)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Blueprint not found: %s"), *AssetPath));
        return Response;
    }

    // Find ASC component
    USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
    if (!SCS)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Blueprint has no SimpleConstructionScript"));
        return Response;
    }

    USCS_Node* ASCNode = nullptr;
    TArray<USCS_Node*> AllNodes = SCS->GetAllNodes();
    for (USCS_Node* Node : AllNodes)
    {
        if (Node && Node->GetVariableName().ToString() == ASCComponentName)
        {
            ASCNode = Node;
            break;
        }
    }

    if (!ASCNode)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), FString::Printf(TEXT("AbilitySystemComponent '%s' not found in Blueprint"), *ASCComponentName));
        return Response;
    }

    UAbilitySystemComponent* ASCTemplate = Cast<UAbilitySystemComponent>(ASCNode->ComponentTemplate);
    if (!ASCTemplate)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Component is not an AbilitySystemComponent"));
        return Response;
    }

    // Note: UAbilitySystemComponent doesn't have DefaultAbilities/DefaultEffects properties by default
    // These would typically be set in a custom ASC subclass or in BeginPlay
    // For now, we'll log them for reference
    if (DefaultAbilitiesArray && DefaultAbilitiesArray->Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Default abilities specified (%d abilities) - note these need to be granted in BeginPlay or custom ASC class"), DefaultAbilitiesArray->Num());
        for (const TSharedPtr<FJsonValue>& AbilityValue : *DefaultAbilitiesArray)
        {
            FString AbilityPath = AbilityValue->AsString();
            UE_LOG(LogTemp, Log, TEXT("  - %s"), *AbilityPath);
        }
    }

    if (DefaultEffectsArray && DefaultEffectsArray->Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Default effects specified (%d effects) - note these need to be applied in BeginPlay or custom ASC class"), DefaultEffectsArray->Num());
        for (const TSharedPtr<FJsonValue>& EffectValue : *DefaultEffectsArray)
        {
            FString EffectPath = EffectValue->AsString();
            UE_LOG(LogTemp, Log, TEXT("  - %s"), *EffectPath);
        }
    }

    // Compile and save
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();

    // Build response
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("blueprint_name"), BlueprintName);
    Response->SetStringField(TEXT("asc_component_name"), ASCComponentName);
    Response->SetNumberField(TEXT("default_abilities_count"), DefaultAbilitiesArray ? DefaultAbilitiesArray->Num() : 0);
    Response->SetNumberField(TEXT("default_effects_count"), DefaultEffectsArray ? DefaultEffectsArray->Num() : 0);

    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Configured ASC defaults for Blueprint '%s'"), *BlueprintName);

    return Response;
}
