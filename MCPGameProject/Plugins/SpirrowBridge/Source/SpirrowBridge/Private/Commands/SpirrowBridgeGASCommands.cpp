#include "Commands/SpirrowBridgeGASCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
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
#include "Abilities/GameplayAbility.h"

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
    else if (CommandType == TEXT("create_gameplay_ability"))
    {
        return HandleCreateGameplayAbility(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::UnknownCommand,
        FString::Printf(TEXT("Unknown GAS command: %s"), *CommandType));
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

    TArray<FString> Lines;
    FileContent.ParseIntoArrayLines(Lines);

    for (const FString& Line : Lines)
    {
        if (Line.Contains(TEXT("GameplayTagList=")))
        {
            FString TagValue;
            FString CommentValue;

            int32 TagStart = Line.Find(TEXT("Tag=\""));
            if (TagStart != INDEX_NONE)
            {
                TagStart += 5;
                int32 TagEnd = Line.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, TagStart);
                if (TagEnd != INDEX_NONE)
                {
                    TagValue = Line.Mid(TagStart, TagEnd - TagStart);
                }
            }

            int32 CommentStart = Line.Find(TEXT("DevComment=\""));
            if (CommentStart != INDEX_NONE)
            {
                CommentStart += 12;
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

    const FString SectionName = TEXT("[/Script/GameplayTags.GameplayTagsSettings]");

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

        if (bInSection && Line.Contains(TEXT("GameplayTagList=")))
        {
            continue;
        }

        NewLines.Add(Line);
    }

    if (!bSectionFound)
    {
        NewLines.Add(TEXT(""));
        NewLines.Add(SectionName);
    }

    int32 InsertIndex = NewLines.IndexOfByPredicate([&SectionName](const FString& Line) {
        return Line.Equals(SectionName);
    });

    if (InsertIndex != INDEX_NONE)
    {
        InsertIndex++;

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

    FString NewContent = FString::Join(NewLines, TEXT("\n"));
    return FFileHelper::SaveStringToFile(NewContent, *ConfigPath);
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleAddGameplayTags(const TSharedPtr<FJsonObject>& Params)
{
    const TArray<TSharedPtr<FJsonValue>>* TagsArray;
    if (!Params->TryGetArrayField(TEXT("tags"), TagsArray))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing 'tags' parameter"));
    }

    const TSharedPtr<FJsonObject>* CommentsObj = nullptr;
    Params->TryGetObjectField(TEXT("comments"), CommentsObj);

    FString ConfigPath = GetGameplayTagsConfigPath();

    TArray<TPair<FString, FString>> ExistingTags = ParseExistingTags(ConfigPath);
    TSet<FString> ExistingTagSet;
    for (const auto& TagPair : ExistingTags)
    {
        ExistingTagSet.Add(TagPair.Key);
    }

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

        FString Comment;
        if (CommentsObj && (*CommentsObj)->HasField(Tag))
        {
            Comment = (*CommentsObj)->GetStringField(Tag);
        }

        ExistingTags.Add(TPair<FString, FString>(Tag, Comment));
        ExistingTagSet.Add(Tag);
        AddedTags.Add(Tag);
    }

    if (AddedTags.Num() > 0)
    {
        if (!WriteTagsToConfig(ConfigPath, ExistingTags))
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::FileWriteFailed,
                TEXT("Failed to write config file"));
        }
    }

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
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

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleListGameplayTags(const TSharedPtr<FJsonObject>& Params)
{
    FString FilterPrefix;
    Params->TryGetStringField(TEXT("filter_prefix"), FilterPrefix);
    FString ConfigPath = GetGameplayTagsConfigPath();

    TArray<TPair<FString, FString>> AllTags = ParseExistingTags(ConfigPath);

    TArray<TSharedPtr<FJsonValue>> TagsArray;
    for (const auto& TagPair : AllTags)
    {
        if (!FilterPrefix.IsEmpty() && !TagPair.Key.StartsWith(FilterPrefix))
        {
            continue;
        }

        TSharedPtr<FJsonObject> TagObj = MakeShareable(new FJsonObject);
        TagObj->SetStringField(TEXT("tag"), TagPair.Key);
        TagObj->SetStringField(TEXT("comment"), TagPair.Value);
        TagsArray.Add(MakeShareable(new FJsonValueObject(TagObj)));
    }

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetArrayField(TEXT("tags"), TagsArray);
    Response->SetNumberField(TEXT("count"), TagsArray.Num());
    Response->SetStringField(TEXT("file_path"), ConfigPath);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleRemoveGameplayTag(const TSharedPtr<FJsonObject>& Params)
{
    FString TagToRemove;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("tag"), TagToRemove))
    {
        return Error;
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
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::FileWriteFailed,
                TEXT("Failed to write config file"));
        }
    }

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetBoolField(TEXT("removed"), bFound);
    Response->SetStringField(TEXT("message"), bFound ? FString::Printf(TEXT("Removed tag: %s"), *TagToRemove) : FString::Printf(TEXT("Tag not found: %s"), *TagToRemove));
    Response->SetStringField(TEXT("file_path"), ConfigPath);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleListGASAssets(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetType, PathFilter;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("asset_type"), AssetType, TEXT("all"));
    Params->TryGetStringField(TEXT("path_filter"), PathFilter);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<TSharedPtr<FJsonValue>> EffectsArray;
    TArray<TSharedPtr<FJsonValue>> AbilitiesArray;
    TArray<TSharedPtr<FJsonValue>> CuesArray;
    TArray<TSharedPtr<FJsonValue>> AttributeSetsArray;

    auto CreateAssetInfo = [](const FAssetData& Asset) -> TSharedPtr<FJsonObject> {
        TSharedPtr<FJsonObject> AssetObj = MakeShareable(new FJsonObject);
        AssetObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
        AssetObj->SetStringField(TEXT("class"), Asset.AssetClassPath.GetAssetName().ToString());
        return AssetObj;
    };

    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint")));
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;

    if (!PathFilter.IsEmpty())
    {
        Filter.PackagePaths.Add(FName(*PathFilter));
    }

    TArray<FAssetData> BlueprintAssets;
    AssetRegistry.GetAssets(Filter, BlueprintAssets);

    for (const FAssetData& Asset : BlueprintAssets)
    {
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

        bool bIsEffect = false;
        bool bIsAbility = false;
        bool bIsCue = false;
        bool bIsAttributeSet = false;

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

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetArrayField(TEXT("effects"), EffectsArray);
    Response->SetArrayField(TEXT("abilities"), AbilitiesArray);
    Response->SetArrayField(TEXT("cues"), CuesArray);
    Response->SetArrayField(TEXT("attribute_sets"), AttributeSetsArray);

    int32 TotalCount = EffectsArray.Num() + AbilitiesArray.Num() + CuesArray.Num() + AttributeSetsArray.Num();
    Response->SetNumberField(TEXT("total_count"), TotalCount);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleCreateGameplayEffect(const TSharedPtr<FJsonObject>& Params)
{
    FString Name;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return Error;
    }

    FString DurationPolicyStr, Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("duration_policy"), DurationPolicyStr, TEXT("Instant"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/GAS/Effects"));

    double DurationMagnitude = 0.0, Period = 0.0;
    FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("duration_magnitude"), DurationMagnitude, 0.0);
    FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("period"), Period, 0.0);

    const TArray<TSharedPtr<FJsonValue>>* ModifiersArray = nullptr;
    Params->TryGetArrayField(TEXT("modifiers"), ModifiersArray);

    const TArray<TSharedPtr<FJsonValue>>* ApplicationTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("application_tags"), ApplicationTagsArray);

    const TArray<TSharedPtr<FJsonValue>>* RemovalTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("removal_tags"), RemovalTagsArray);

    FString PackagePath = Path / Name;
    FString AssetPath = PackagePath + TEXT(".") + Name;

    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Asset already exists: %s"), *AssetPath));
    }

    UPackage* ExistingPackage = FindPackage(nullptr, *PackagePath);
    if (ExistingPackage)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Package already exists: %s"), *PackagePath));
    }

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package"));
    }

    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    Factory->ParentClass = UGameplayEffect::StaticClass();

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
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create GameplayEffect Blueprint"));
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    if (!Blueprint->GeneratedClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidBlueprint,
            TEXT("Blueprint has no GeneratedClass after compile."));
    }
    UGameplayEffect* EffectCDO = Cast<UGameplayEffect>(Blueprint->GeneratedClass->GetDefaultObject());
    if (!EffectCDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::OperationFailed,
            TEXT("Failed to get GameplayEffect CDO"));
    }

    if (DurationPolicyStr == TEXT("Instant"))
    {
        EffectCDO->DurationPolicy = EGameplayEffectDurationType::Instant;
    }
    else if (DurationPolicyStr == TEXT("HasDuration"))
    {
        EffectCDO->DurationPolicy = EGameplayEffectDurationType::HasDuration;
        EffectCDO->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationMagnitude));
    }
    else if (DurationPolicyStr == TEXT("Infinite"))
    {
        EffectCDO->DurationPolicy = EGameplayEffectDurationType::Infinite;
    }

    if (Period > 0.0)
    {
        EffectCDO->Period = FScalableFloat(Period);
    }

    if (ModifiersArray && ModifiersArray->Num() > 0)
    {
        for (const TSharedPtr<FJsonValue>& ModValue : *ModifiersArray)
        {
            const TSharedPtr<FJsonObject>* ModObj = nullptr;
            if (!ModValue->TryGetObject(ModObj) || !ModObj) continue;

            FString AttributeName = (*ModObj)->GetStringField(TEXT("attribute"));
            FString OperationStr = (*ModObj)->GetStringField(TEXT("operation"));
            double Magnitude = (*ModObj)->GetNumberField(TEXT("magnitude"));

            FGameplayModifierInfo ModInfo;

            if (OperationStr == TEXT("Add"))
                ModInfo.ModifierOp = EGameplayModOp::Additive;
            else if (OperationStr == TEXT("Multiply"))
                ModInfo.ModifierOp = EGameplayModOp::Multiplicitive;
            else if (OperationStr == TEXT("Divide"))
                ModInfo.ModifierOp = EGameplayModOp::Division;
            else if (OperationStr == TEXT("Override"))
                ModInfo.ModifierOp = EGameplayModOp::Override;
            else
                ModInfo.ModifierOp = EGameplayModOp::Additive;

            ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));
            EffectCDO->Modifiers.Add(ModInfo);
        }
    }

    if (ApplicationTagsArray && ApplicationTagsArray->Num() > 0)
    {
        UTargetTagsGameplayEffectComponent& TargetTagsComponent = EffectCDO->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
        FInheritedTagContainer TagContainer;

        for (const TSharedPtr<FJsonValue>& TagValue : *ApplicationTagsArray)
        {
            FString TagString = TagValue->AsString();
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
            if (Tag.IsValid())
            {
                TagContainer.Added.AddTag(Tag);
                TagContainer.CombinedTags.AddTag(Tag);
            }
        }

        TargetTagsComponent.SetAndApplyTargetTagChanges(TagContainer);
    }

    if (RemovalTagsArray && RemovalTagsArray->Num() > 0)
    {
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        for (const TSharedPtr<FJsonValue>& TagValue : *RemovalTagsArray)
        {
            FString TagString = TagValue->AsString();
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
            if (Tag.IsValid())
            {
                EffectCDO->RemoveGameplayEffectsWithTags.Added.AddTag(Tag);
            }
        }
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }

    Package->MarkPackageDirty();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

    FAssetRegistryModule::AssetCreated(Blueprint);

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("name"), Name);
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("duration_policy"), DurationPolicyStr);
    Response->SetNumberField(TEXT("modifier_count"), ModifiersArray ? ModifiersArray->Num() : 0);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleCreateGASCharacter(const TSharedPtr<FJsonObject>& Params)
{
    FString Name;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return Error;
    }

    FString ParentClassStr, ASCComponentName, ReplicationMode, PathStr;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("parent_class"), ParentClassStr, TEXT("Character"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("asc_component_name"), ASCComponentName, TEXT("AbilitySystemComponent"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("replication_mode"), ReplicationMode, TEXT("Mixed"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), PathStr, TEXT("/Game/GAS/Characters"));

    const TArray<TSharedPtr<FJsonValue>>* DefaultAbilitiesArray = nullptr;
    Params->TryGetArrayField(TEXT("default_abilities"), DefaultAbilitiesArray);

    const TArray<TSharedPtr<FJsonValue>>* DefaultEffectsArray = nullptr;
    Params->TryGetArrayField(TEXT("default_effects"), DefaultEffectsArray);

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
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::ClassNotFound,
                FString::Printf(TEXT("Parent class not found: %s"), *ParentClassStr));
        }
    }

    FString PackageName = PathStr / Name;
    FString AssetPath = PackageName + TEXT(".") + Name;

    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Asset already exists: %s"), *AssetPath));
    }

    UPackage* ExistingPackage = FindPackage(nullptr, *PackageName);
    if (ExistingPackage)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Package already exists: %s"), *PackageName));
    }

    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package"));
    }

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
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create Blueprint"));
    }

    USimpleConstructionScript* SCS = NewBlueprint->SimpleConstructionScript;
    if (!SCS)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no SimpleConstructionScript"));
    }

    USCS_Node* NewNode = SCS->CreateNode(UAbilitySystemComponent::StaticClass(), FName(*ASCComponentName));
    if (!NewNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ComponentCreationFailed,
            TEXT("Failed to create AbilitySystemComponent node"));
    }

    SCS->AddNode(NewNode);

    UAbilitySystemComponent* ASCTemplate = Cast<UAbilitySystemComponent>(NewNode->ComponentTemplate);
    if (ASCTemplate)
    {
        if (ReplicationMode == TEXT("Full"))
            ASCTemplate->SetReplicationMode(EGameplayEffectReplicationMode::Full);
        else if (ReplicationMode == TEXT("Mixed"))
            ASCTemplate->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
        else if (ReplicationMode == TEXT("Minimal"))
            ASCTemplate->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
    }

    FKismetEditorUtilities::CompileBlueprint(NewBlueprint);
    FAssetRegistryModule::AssetCreated(NewBlueprint);
    Package->MarkPackageDirty();

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("name"), Name);
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("parent_class"), ParentClassStr);
    Response->SetStringField(TEXT("asc_component_name"), ASCComponentName);
    Response->SetStringField(TEXT("replication_mode"), ReplicationMode);

    return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleSetAbilitySystemDefaults(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }

    FString ASCComponentName, PathStr;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("asc_component_name"), ASCComponentName, TEXT("AbilitySystemComponent"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), PathStr, TEXT("/Game/Blueprints"));

    const TArray<TSharedPtr<FJsonValue>>* DefaultAbilitiesArray = nullptr;
    Params->TryGetArrayField(TEXT("default_abilities"), DefaultAbilitiesArray);

    const TArray<TSharedPtr<FJsonValue>>* DefaultEffectsArray = nullptr;
    Params->TryGetArrayField(TEXT("default_effects"), DefaultEffectsArray);

    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateBlueprint(BlueprintName, PathStr, Blueprint))
    {
        return Error;
    }

    USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
    if (!SCS)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Blueprint has no SimpleConstructionScript"));
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
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ComponentNotFound,
            FString::Printf(TEXT("AbilitySystemComponent '%s' not found in Blueprint"), *ASCComponentName));
    }

    UAbilitySystemComponent* ASCTemplate = Cast<UAbilitySystemComponent>(ASCNode->ComponentTemplate);
    if (!ASCTemplate)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            TEXT("Component is not an AbilitySystemComponent"));
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("blueprint_name"), BlueprintName);
    Response->SetStringField(TEXT("asc_component_name"), ASCComponentName);
    Response->SetNumberField(TEXT("default_abilities_count"), DefaultAbilitiesArray ? DefaultAbilitiesArray->Num() : 0);
    Response->SetNumberField(TEXT("default_effects_count"), DefaultEffectsArray ? DefaultEffectsArray->Num() : 0);

    return Response;
}

void FSpirrowBridgeGASCommands::SetGameplayTagContainerFromArray(FGameplayTagContainer& Container, const TArray<TSharedPtr<FJsonValue>>* TagsArray)
{
    if (!TagsArray) return;

    for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
    {
        FString TagString = TagValue->AsString();
        FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
        if (Tag.IsValid())
        {
            Container.AddTag(Tag);
        }
    }
}

static void SetGameplayTagContainerPropertyByName(UObject* Object, const FName& PropertyName, const TArray<TSharedPtr<FJsonValue>>* TagsArray)
{
    if (!Object || !TagsArray) return;

    FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
    if (!Property) return;

    void* PropertyValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
    if (!PropertyValuePtr) return;

    FGameplayTagContainer* Container = static_cast<FGameplayTagContainer*>(PropertyValuePtr);
    for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
    {
        FString TagString = TagValue->AsString();
        FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
        if (Tag.IsValid())
        {
            Container->AddTag(Tag);
        }
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeGASCommands::HandleCreateGameplayAbility(const TSharedPtr<FJsonObject>& Params)
{
    FString Name;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return Error;
    }

    FString ParentClassName, CostEffectPath, CooldownEffectPath, InstancingPolicyStr, NetExecutionPolicyStr, Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("parent_class"), ParentClassName, TEXT("GameplayAbility"));
    Params->TryGetStringField(TEXT("cost_effect"), CostEffectPath);
    Params->TryGetStringField(TEXT("cooldown_effect"), CooldownEffectPath);
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("instancing_policy"), InstancingPolicyStr, TEXT("InstancedPerActor"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("net_execution_policy"), NetExecutionPolicyStr, TEXT("LocalPredicted"));
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/GAS/Abilities"));

    const TArray<TSharedPtr<FJsonValue>>* AbilityTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("ability_tags"), AbilityTagsArray);

    const TArray<TSharedPtr<FJsonValue>>* CancelTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("cancel_abilities_with_tags"), CancelTagsArray);

    const TArray<TSharedPtr<FJsonValue>>* BlockTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("block_abilities_with_tags"), BlockTagsArray);

    const TArray<TSharedPtr<FJsonValue>>* ActivationOwnedTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("activation_owned_tags"), ActivationOwnedTagsArray);

    const TArray<TSharedPtr<FJsonValue>>* ActivationRequiredTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("activation_required_tags"), ActivationRequiredTagsArray);

    const TArray<TSharedPtr<FJsonValue>>* ActivationBlockedTagsArray = nullptr;
    Params->TryGetArrayField(TEXT("activation_blocked_tags"), ActivationBlockedTagsArray);

    FString PackagePath = Path / Name;
    FString AssetPath = PackagePath + TEXT(".") + Name;

    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Asset already exists: %s"), *AssetPath));
    }

    UPackage* ExistingPackage = FindPackage(nullptr, *PackagePath);
    if (ExistingPackage)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetAlreadyExists,
            FString::Printf(TEXT("Package already exists: %s"), *PackagePath));
    }

    UClass* ParentClass = nullptr;
    if (ParentClassName == TEXT("GameplayAbility") || ParentClassName.IsEmpty())
    {
        ParentClass = UGameplayAbility::StaticClass();
    }
    else
    {
        ParentClass = FindObject<UClass>(nullptr, *ParentClassName);
        if (!ParentClass)
        {
            ParentClass = LoadClass<UGameplayAbility>(nullptr, *ParentClassName);
        }
        if (!ParentClass)
        {
            ParentClass = UGameplayAbility::StaticClass();
        }
    }

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package"));
    }

    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    Factory->ParentClass = ParentClass;

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
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create GameplayAbility Blueprint"));
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    if (!Blueprint->GeneratedClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidBlueprint,
            TEXT("Blueprint has no GeneratedClass after compile."));
    }
    UGameplayAbility* AbilityCDO = Cast<UGameplayAbility>(Blueprint->GeneratedClass->GetDefaultObject());
    if (!AbilityCDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::OperationFailed,
            TEXT("Failed to get GameplayAbility CDO"));
    }

    SetGameplayTagContainerPropertyByName(AbilityCDO, FName("AbilityTags"), AbilityTagsArray);
    SetGameplayTagContainerPropertyByName(AbilityCDO, FName("CancelAbilitiesWithTag"), CancelTagsArray);
    SetGameplayTagContainerPropertyByName(AbilityCDO, FName("BlockAbilitiesWithTag"), BlockTagsArray);
    SetGameplayTagContainerPropertyByName(AbilityCDO, FName("ActivationOwnedTags"), ActivationOwnedTagsArray);
    SetGameplayTagContainerPropertyByName(AbilityCDO, FName("ActivationRequiredTags"), ActivationRequiredTagsArray);
    SetGameplayTagContainerPropertyByName(AbilityCDO, FName("ActivationBlockedTags"), ActivationBlockedTagsArray);

    if (!CostEffectPath.IsEmpty())
    {
        UClass* CostEffectClass = LoadClass<UGameplayEffect>(nullptr, *CostEffectPath);
        if (CostEffectClass)
        {
            FProperty* Property = AbilityCDO->GetClass()->FindPropertyByName(FName("CostGameplayEffectClass"));
            if (Property)
            {
                FClassProperty* ClassProperty = CastField<FClassProperty>(Property);
                if (ClassProperty)
                {
                    void* PropertyValuePtr = Property->ContainerPtrToValuePtr<void>(AbilityCDO);
                    ClassProperty->SetObjectPropertyValue(PropertyValuePtr, CostEffectClass);
                }
            }
        }
    }

    if (!CooldownEffectPath.IsEmpty())
    {
        UClass* CooldownEffectClass = LoadClass<UGameplayEffect>(nullptr, *CooldownEffectPath);
        if (CooldownEffectClass)
        {
            FProperty* Property = AbilityCDO->GetClass()->FindPropertyByName(FName("CooldownGameplayEffectClass"));
            if (Property)
            {
                FClassProperty* ClassProperty = CastField<FClassProperty>(Property);
                if (ClassProperty)
                {
                    void* PropertyValuePtr = Property->ContainerPtrToValuePtr<void>(AbilityCDO);
                    ClassProperty->SetObjectPropertyValue(PropertyValuePtr, CooldownEffectClass);
                }
            }
        }
    }

    FProperty* InstancingProp = AbilityCDO->GetClass()->FindPropertyByName(FName("InstancingPolicy"));
    if (InstancingProp)
    {
        void* InstancingPropPtr = InstancingProp->ContainerPtrToValuePtr<void>(AbilityCDO);
        if (InstancingPolicyStr == TEXT("NonInstanced"))
            *static_cast<TEnumAsByte<EGameplayAbilityInstancingPolicy::Type>*>(InstancingPropPtr) = EGameplayAbilityInstancingPolicy::InstancedPerActor;
        else if (InstancingPolicyStr == TEXT("InstancedPerActor"))
            *static_cast<TEnumAsByte<EGameplayAbilityInstancingPolicy::Type>*>(InstancingPropPtr) = EGameplayAbilityInstancingPolicy::InstancedPerActor;
        else if (InstancingPolicyStr == TEXT("InstancedPerExecution"))
            *static_cast<TEnumAsByte<EGameplayAbilityInstancingPolicy::Type>*>(InstancingPropPtr) = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    }

    FProperty* NetExecProp = AbilityCDO->GetClass()->FindPropertyByName(FName("NetExecutionPolicy"));
    if (NetExecProp)
    {
        void* NetExecPropPtr = NetExecProp->ContainerPtrToValuePtr<void>(AbilityCDO);
        if (NetExecutionPolicyStr == TEXT("LocalPredicted"))
            *static_cast<TEnumAsByte<EGameplayAbilityNetExecutionPolicy::Type>*>(NetExecPropPtr) = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
        else if (NetExecutionPolicyStr == TEXT("LocalOnly"))
            *static_cast<TEnumAsByte<EGameplayAbilityNetExecutionPolicy::Type>*>(NetExecPropPtr) = EGameplayAbilityNetExecutionPolicy::LocalOnly;
        else if (NetExecutionPolicyStr == TEXT("ServerInitiated"))
            *static_cast<TEnumAsByte<EGameplayAbilityNetExecutionPolicy::Type>*>(NetExecPropPtr) = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
        else if (NetExecutionPolicyStr == TEXT("ServerOnly"))
            *static_cast<TEnumAsByte<EGameplayAbilityNetExecutionPolicy::Type>*>(NetExecPropPtr) = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    }

    Package->MarkPackageDirty();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);

    FAssetRegistryModule::AssetCreated(Blueprint);

    int32 TotalTagsSet = 0;
    if (AbilityTagsArray) TotalTagsSet += AbilityTagsArray->Num();
    if (CancelTagsArray) TotalTagsSet += CancelTagsArray->Num();
    if (BlockTagsArray) TotalTagsSet += BlockTagsArray->Num();
    if (ActivationOwnedTagsArray) TotalTagsSet += ActivationOwnedTagsArray->Num();
    if (ActivationRequiredTagsArray) TotalTagsSet += ActivationRequiredTagsArray->Num();
    if (ActivationBlockedTagsArray) TotalTagsSet += ActivationBlockedTagsArray->Num();

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("name"), Name);
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("parent_class"), ParentClass->GetName());
    Response->SetStringField(TEXT("instancing_policy"), InstancingPolicyStr);
    Response->SetStringField(TEXT("net_execution_policy"), NetExecutionPolicyStr);
    Response->SetNumberField(TEXT("tags_configured"), TotalTagsSet);
    Response->SetBoolField(TEXT("has_cost"), !CostEffectPath.IsEmpty());
    Response->SetBoolField(TEXT("has_cooldown"), !CooldownEffectPath.IsEmpty());

    return Response;
}
