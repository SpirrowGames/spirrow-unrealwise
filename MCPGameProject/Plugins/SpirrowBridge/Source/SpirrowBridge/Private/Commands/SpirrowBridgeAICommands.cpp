#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// AI Module includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"

// Asset creation includes
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"

FSpirrowBridgeAICommands::FSpirrowBridgeAICommands()
{
}

FSpirrowBridgeAICommands::~FSpirrowBridgeAICommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// Blackboard commands
	if (CommandType == TEXT("create_blackboard"))
	{
		return HandleCreateBlackboard(Params);
	}
	else if (CommandType == TEXT("add_blackboard_key"))
	{
		return HandleAddBlackboardKey(Params);
	}
	else if (CommandType == TEXT("remove_blackboard_key"))
	{
		return HandleRemoveBlackboardKey(Params);
	}
	else if (CommandType == TEXT("list_blackboard_keys"))
	{
		return HandleListBlackboardKeys(Params);
	}
	// BehaviorTree commands
	else if (CommandType == TEXT("create_behavior_tree"))
	{
		return HandleCreateBehaviorTree(Params);
	}
	else if (CommandType == TEXT("set_behavior_tree_blackboard"))
	{
		return HandleSetBehaviorTreeBlackboard(Params);
	}
	else if (CommandType == TEXT("get_behavior_tree_structure"))
	{
		return HandleGetBehaviorTreeStructure(Params);
	}
	// Utility commands
	else if (CommandType == TEXT("list_ai_assets"))
	{
		return HandleListAIAssets(Params);
	}

	return FSpirrowBridgeCommonUtils::CreateErrorResponse(
		ESpirrowErrorCode::UnknownCommand,
		FString::Printf(TEXT("Unknown AI command: %s"), *CommandType));
}

// ===== Helper Functions =====

UBlackboardData* FSpirrowBridgeAICommands::FindBlackboardAsset(const FString& Name, const FString& Path)
{
	FString FullPath = Path / Name + TEXT(".") + Name;
	return Cast<UBlackboardData>(UEditorAssetLibrary::LoadAsset(FullPath));
}

UBehaviorTree* FSpirrowBridgeAICommands::FindBehaviorTreeAsset(const FString& Name, const FString& Path)
{
	FString FullPath = Path / Name + TEXT(".") + Name;
	return Cast<UBehaviorTree>(UEditorAssetLibrary::LoadAsset(FullPath));
}

UClass* FSpirrowBridgeAICommands::GetBlackboardKeyTypeClass(const FString& TypeString)
{
	if (TypeString == TEXT("Bool")) return UBlackboardKeyType_Bool::StaticClass();
	if (TypeString == TEXT("Int")) return UBlackboardKeyType_Int::StaticClass();
	if (TypeString == TEXT("Float")) return UBlackboardKeyType_Float::StaticClass();
	if (TypeString == TEXT("String")) return UBlackboardKeyType_String::StaticClass();
	if (TypeString == TEXT("Name")) return UBlackboardKeyType_Name::StaticClass();
	if (TypeString == TEXT("Vector")) return UBlackboardKeyType_Vector::StaticClass();
	if (TypeString == TEXT("Rotator")) return UBlackboardKeyType_Rotator::StaticClass();
	if (TypeString == TEXT("Object")) return UBlackboardKeyType_Object::StaticClass();
	if (TypeString == TEXT("Class")) return UBlackboardKeyType_Class::StaticClass();
	if (TypeString == TEXT("Enum")) return UBlackboardKeyType_Enum::StaticClass();
	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::BlackboardKeyToJson(const FBlackboardEntry& Entry)
{
	TSharedPtr<FJsonObject> KeyJson = MakeShareable(new FJsonObject());
	KeyJson->SetStringField(TEXT("name"), Entry.EntryName.ToString());

	if (Entry.KeyType)
	{
		FString TypeName = Entry.KeyType->GetClass()->GetName();
		// Remove "BlackboardKeyType_" prefix
		TypeName.RemoveFromStart(TEXT("BlackboardKeyType_"));
		KeyJson->SetStringField(TEXT("type"), TypeName);

		// Add base_class for Object/Class types
		if (UBlackboardKeyType_Object* ObjectType = Cast<UBlackboardKeyType_Object>(Entry.KeyType))
		{
			if (ObjectType->BaseClass)
			{
				KeyJson->SetStringField(TEXT("base_class"), ObjectType->BaseClass->GetName());
			}
		}
		else if (UBlackboardKeyType_Class* ClassType = Cast<UBlackboardKeyType_Class>(Entry.KeyType))
		{
			if (ClassType->BaseClass)
			{
				KeyJson->SetStringField(TEXT("base_class"), ClassType->BaseClass->GetName());
			}
		}
	}

	KeyJson->SetBoolField(TEXT("instance_synced"), Entry.bInstanceSynced);

	return KeyJson;
}

// ===== Blackboard Commands Implementation =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleCreateBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString Name;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name);
	if (NameError)
	{
		return NameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));

	// パッケージ作成
	FString PackagePath = Path / Name;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create package"));
	}

	// Blackboard Data Asset作成
	UBlackboardData* BlackboardData = NewObject<UBlackboardData>(
		Package,
		*Name,
		RF_Public | RF_Standalone);

	if (!BlackboardData)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create Blackboard Data Asset"));
	}

	// 保存
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BlackboardData);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BlackboardData, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_path"), PackagePath);
	Result->SetStringField(TEXT("name"), Name);
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleAddBlackboardKey(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString BlackboardName;
	TSharedPtr<FJsonObject> BlackboardNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName);
	if (BlackboardNameError)
	{
		return BlackboardNameError;
	}

	FString KeyName;
	TSharedPtr<FJsonObject> KeyNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_name"), KeyName);
	if (KeyNameError)
	{
		return KeyNameError;
	}

	FString KeyType;
	TSharedPtr<FJsonObject> KeyTypeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_type"), KeyType);
	if (KeyTypeError)
	{
		return KeyTypeError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));
	bool bInstanceSynced;
	FSpirrowBridgeCommonUtils::GetOptionalBool(
		Params, TEXT("instance_synced"), bInstanceSynced, false);
	FString BaseClass;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("base_class"), BaseClass, TEXT(""));

	// Blackboard検索
	UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, Path);
	if (!Blackboard)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *Path));
	}

	// キータイプ取得
	UClass* KeyTypeClass = GetBlackboardKeyTypeClass(KeyType);
	if (!KeyTypeClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid key type: %s"), *KeyType));
	}

	// 重複チェック
	for (const FBlackboardEntry& Entry : Blackboard->Keys)
	{
		if (Entry.EntryName == FName(*KeyName))
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::InvalidOperation,
				FString::Printf(TEXT("Key already exists: %s"), *KeyName));
		}
	}

	// キー追加
	FBlackboardEntry NewEntry;
	NewEntry.EntryName = FName(*KeyName);
	NewEntry.KeyType = NewObject<UBlackboardKeyType>(Blackboard, KeyTypeClass);
	NewEntry.bInstanceSynced = bInstanceSynced;

	// Object/Classタイプの場合、BaseClassを設定
	if (!BaseClass.IsEmpty())
	{
		if (UBlackboardKeyType_Object* ObjectType = Cast<UBlackboardKeyType_Object>(NewEntry.KeyType))
		{
			UClass* FoundClass = FindObject<UClass>(nullptr, *BaseClass);
			if (FoundClass)
			{
				ObjectType->BaseClass = FoundClass;
			}
		}
		else if (UBlackboardKeyType_Class* ClassType = Cast<UBlackboardKeyType_Class>(NewEntry.KeyType))
		{
			UClass* FoundClass = FindObject<UClass>(nullptr, *BaseClass);
			if (FoundClass)
			{
				ClassType->BaseClass = FoundClass;
			}
		}
	}

	Blackboard->Keys.Add(NewEntry);

	// 保存
	Blackboard->MarkPackageDirty();
	UPackage* Package = Blackboard->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blackboard, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
	Result->SetStringField(TEXT("key_name"), KeyName);
	Result->SetStringField(TEXT("key_type"), KeyType);
	Result->SetNumberField(TEXT("total_keys"), Blackboard->Keys.Num());
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleRemoveBlackboardKey(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString BlackboardName;
	TSharedPtr<FJsonObject> BlackboardNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName);
	if (BlackboardNameError)
	{
		return BlackboardNameError;
	}

	FString KeyName;
	TSharedPtr<FJsonObject> KeyNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_name"), KeyName);
	if (KeyNameError)
	{
		return KeyNameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));

	// Blackboard検索
	UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, Path);
	if (!Blackboard)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *Path));
	}

	// キー検索と削除
	int32 RemovedIndex = INDEX_NONE;
	for (int32 i = 0; i < Blackboard->Keys.Num(); ++i)
	{
		if (Blackboard->Keys[i].EntryName == FName(*KeyName))
		{
			RemovedIndex = i;
			Blackboard->Keys.RemoveAt(i);
			break;
		}
	}

	if (RemovedIndex == INDEX_NONE)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Key not found: %s"), *KeyName));
	}

	// 保存
	Blackboard->MarkPackageDirty();
	UPackage* Package = Blackboard->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blackboard, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
	Result->SetStringField(TEXT("removed_key"), KeyName);
	Result->SetNumberField(TEXT("remaining_keys"), Blackboard->Keys.Num());
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleListBlackboardKeys(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString BlackboardName;
	TSharedPtr<FJsonObject> BlackboardNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName);
	if (BlackboardNameError)
	{
		return BlackboardNameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));

	// Blackboard検索
	UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, Path);
	if (!Blackboard)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *Path));
	}

	// キー一覧取得
	TArray<TSharedPtr<FJsonValue>> KeysArray;
	for (const FBlackboardEntry& Entry : Blackboard->Keys)
	{
		KeysArray.Add(MakeShareable(new FJsonValueObject(BlackboardKeyToJson(Entry))));
	}

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
	Result->SetArrayField(TEXT("keys"), KeysArray);
	Result->SetNumberField(TEXT("count"), Blackboard->Keys.Num());
	return Result;
}

// ===== BehaviorTree Commands Implementation =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleCreateBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString Name;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name);
	if (NameError)
	{
		return NameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));
	FString BlackboardName;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("blackboard_name"), BlackboardName, TEXT(""));
	FString BlackboardPath;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("blackboard_path"), BlackboardPath, TEXT("/Game/AI/Blackboards"));

	// パッケージ作成
	FString PackagePath = Path / Name;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create package"));
	}

	// BehaviorTree Asset作成
	UBehaviorTree* BehaviorTree = NewObject<UBehaviorTree>(
		Package,
		*Name,
		RF_Public | RF_Standalone);

	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create BehaviorTree Asset"));
	}

	// Blackboard設定（指定されている場合）
	if (!BlackboardName.IsEmpty())
	{
		UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, BlackboardPath);
		if (Blackboard)
		{
			BehaviorTree->BlackboardAsset = Blackboard;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Blackboard not found: %s - BehaviorTree created without Blackboard"), *BlackboardName);
		}
	}

	// 保存
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BehaviorTree);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_path"), PackagePath);
	Result->SetStringField(TEXT("name"), Name);
	Result->SetBoolField(TEXT("has_blackboard"), BehaviorTree->BlackboardAsset != nullptr);
	if (BehaviorTree->BlackboardAsset)
	{
		Result->SetStringField(TEXT("blackboard_name"), BehaviorTree->BlackboardAsset->GetName());
	}
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleSetBehaviorTreeBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> BehaviorTreeNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (BehaviorTreeNameError)
	{
		return BehaviorTreeNameError;
	}

	FString BlackboardName;
	TSharedPtr<FJsonObject> BlackboardNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName);
	if (BlackboardNameError)
	{
		return BlackboardNameError;
	}

	FString BtPath;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("behavior_tree_path"), BtPath, TEXT("/Game/AI/BehaviorTrees"));
	FString BbPath;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("blackboard_path"), BbPath, TEXT("/Game/AI/Blackboards"));

	// BehaviorTree検索
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, BtPath);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *BtPath));
	}

	// Blackboard検索
	UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, BbPath);
	if (!Blackboard)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *BbPath));
	}

	// Blackboard設定
	BehaviorTree->BlackboardAsset = Blackboard;

	// 保存
	BehaviorTree->MarkPackageDirty();
	UPackage* Package = BehaviorTree->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleGetBehaviorTreeStructure(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString Name;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name);
	if (NameError)
	{
		return NameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	// BehaviorTree検索
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(Name, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *Name, *Path));
	}

	// 基本情報
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("name"), Name);

	// Blackboard情報
	if (BehaviorTree->BlackboardAsset)
	{
		Result->SetStringField(TEXT("blackboard_name"), BehaviorTree->BlackboardAsset->GetName());
		Result->SetNumberField(TEXT("blackboard_key_count"), BehaviorTree->BlackboardAsset->Keys.Num());
	}
	else
	{
		Result->SetStringField(TEXT("blackboard_name"), TEXT("None"));
	}

	// Root Node情報（将来的にノード構造も取得可能に）
	Result->SetBoolField(TEXT("has_root_node"), BehaviorTree->RootNode != nullptr);

	return Result;
}

// ===== Utility Commands Implementation =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleListAIAssets(
	const TSharedPtr<FJsonObject>& Params)
{
	FString AssetType;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("asset_type"), AssetType, TEXT("all"));
	FString PathFilter;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path_filter"), PathFilter, TEXT(""));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<TSharedPtr<FJsonValue>> BehaviorTrees;
	TArray<TSharedPtr<FJsonValue>> Blackboards;

	// BehaviorTree検索
	if (AssetType == TEXT("all") || AssetType == TEXT("behavior_tree"))
	{
		TArray<FAssetData> BTAssets;
		AssetRegistry.GetAssetsByClass(UBehaviorTree::StaticClass()->GetClassPathName(), BTAssets);

		for (const FAssetData& Asset : BTAssets)
		{
			FString AssetPath = Asset.GetObjectPathString();
			if (PathFilter.IsEmpty() || AssetPath.Contains(PathFilter))
			{
				TSharedPtr<FJsonObject> AssetJson = MakeShareable(new FJsonObject());
				AssetJson->SetStringField(TEXT("name"), Asset.AssetName.ToString());
				AssetJson->SetStringField(TEXT("path"), Asset.PackagePath.ToString());
				BehaviorTrees.Add(MakeShareable(new FJsonValueObject(AssetJson)));
			}
		}
	}

	// Blackboard検索
	if (AssetType == TEXT("all") || AssetType == TEXT("blackboard"))
	{
		TArray<FAssetData> BBAssets;
		AssetRegistry.GetAssetsByClass(UBlackboardData::StaticClass()->GetClassPathName(), BBAssets);

		for (const FAssetData& Asset : BBAssets)
		{
			FString AssetPath = Asset.GetObjectPathString();
			if (PathFilter.IsEmpty() || AssetPath.Contains(PathFilter))
			{
				TSharedPtr<FJsonObject> AssetJson = MakeShareable(new FJsonObject());
				AssetJson->SetStringField(TEXT("name"), Asset.AssetName.ToString());
				AssetJson->SetStringField(TEXT("path"), Asset.PackagePath.ToString());
				Blackboards.Add(MakeShareable(new FJsonValueObject(AssetJson)));
			}
		}
	}

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("behavior_trees"), BehaviorTrees);
	Result->SetArrayField(TEXT("blackboards"), Blackboards);
	Result->SetNumberField(TEXT("total_behavior_trees"), BehaviorTrees.Num());
	Result->SetNumberField(TEXT("total_blackboards"), Blackboards.Num());
	return Result;
}
