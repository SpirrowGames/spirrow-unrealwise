#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// BehaviorTree includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"

// Asset management includes
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"

// ===== Helper Functions =====

UBehaviorTree* FSpirrowBridgeAICommands::FindBehaviorTreeAsset(const FString& Name, const FString& Path)
{
	FString FullPath = Path / Name + TEXT(".") + Name;
	return Cast<UBehaviorTree>(UEditorAssetLibrary::LoadAsset(FullPath));
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
