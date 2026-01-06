#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// BehaviorTree includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"

// Asset management includes
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

// ===== Phase G: BT Node Creation Handlers =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleAddBTCompositeNode(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString NodeType;
	TSharedPtr<FJsonObject> TypeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("node_type"), NodeType);
	if (TypeError) return TypeError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	FString NodeName;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("node_name"), NodeName, TEXT(""));

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// ノードクラス取得
	UClass* NodeClass = GetBTCompositeNodeClass(NodeType);
	if (!NodeClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid composite node type: %s"), *NodeType));
	}

	// ノード作成
	UBTCompositeNode* NewNode = NewObject<UBTCompositeNode>(BehaviorTree, NodeClass);
	if (!NewNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to create composite node"));
	}

	// ノード名設定
	if (!NodeName.IsEmpty())
	{
		NewNode->NodeName = NodeName;
	}

	// 保存
	BehaviorTree->MarkPackageDirty();
	UPackage* Package = BehaviorTree->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NewNode->GetName());
	Result->SetStringField(TEXT("node_type"), NodeType);
	Result->SetStringField(TEXT("node_class"), NodeClass->GetName());
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleAddBTTaskNode(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString TaskType;
	TSharedPtr<FJsonObject> TypeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("task_type"), TaskType);
	if (TypeError) return TypeError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	FString NodeName;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("node_name"), NodeName, TEXT(""));

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// タスククラス取得
	UClass* TaskClass = GetBTTaskNodeClass(TaskType);
	if (!TaskClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid task type: %s"), *TaskType));
	}

	// タスクノード作成
	UBTTaskNode* NewTask = NewObject<UBTTaskNode>(BehaviorTree, TaskClass);
	if (!NewTask)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to create task node"));
	}

	// ノード名設定
	if (!NodeName.IsEmpty())
	{
		NewTask->NodeName = NodeName;
	}

	// 保存
	BehaviorTree->MarkPackageDirty();
	UPackage* Package = BehaviorTree->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NewTask->GetName());
	Result->SetStringField(TEXT("task_type"), TaskType);
	Result->SetStringField(TEXT("node_class"), TaskClass->GetName());
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleAddBTDecoratorNode(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString DecoratorType;
	TSharedPtr<FJsonObject> TypeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("decorator_type"), DecoratorType);
	if (TypeError) return TypeError;

	FString TargetNodeId;
	TSharedPtr<FJsonObject> TargetError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("target_node_id"), TargetNodeId);
	if (TargetError) return TargetError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	FString NodeName;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("node_name"), NodeName, TEXT(""));

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// デコレータクラス取得
	UClass* DecoratorClass = GetBTDecoratorClass(DecoratorType);
	if (!DecoratorClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid decorator type: %s"), *DecoratorType));
	}

	// 対象ノード検索
	UBTNode* TargetNode = FindBTNodeById(BehaviorTree, TargetNodeId);
	if (!TargetNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Target node not found: %s"), *TargetNodeId));
	}

	// デコレータ作成
	UBTDecorator* NewDecorator = NewObject<UBTDecorator>(BehaviorTree, DecoratorClass);
	if (!NewDecorator)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to create decorator node"));
	}

	// ノード名設定
	if (!NodeName.IsEmpty())
	{
		NewDecorator->NodeName = NodeName;
	}

	// 対象ノードにデコレータを追加 (UE 5.6+: decorators are stored in FBTCompositeChild)
	// Find the parent composite and add decorator to the appropriate child entry
	bool bDecoratorAdded = false;
	TFunction<bool(UBTCompositeNode*)> AddDecoratorToChild = [&](UBTCompositeNode* Parent) -> bool
	{
		if (!Parent) return false;

		for (FBTCompositeChild& Child : Parent->Children)
		{
			// Check if this child is our target node
			if (Child.ChildComposite == TargetNode || Child.ChildTask == TargetNode)
			{
				Child.Decorators.Add(NewDecorator);
				return true;
			}

			// Recursively search child composites
			if (Child.ChildComposite && AddDecoratorToChild(Child.ChildComposite))
			{
				return true;
			}
		}
		return false;
	};

	bDecoratorAdded = AddDecoratorToChild(BehaviorTree->RootNode);

	if (!bDecoratorAdded)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidOperation,
			TEXT("Could not find target node in behavior tree to attach decorator"));
	}

	// 保存
	BehaviorTree->MarkPackageDirty();
	UPackage* Package = BehaviorTree->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NewDecorator->GetName());
	Result->SetStringField(TEXT("decorator_type"), DecoratorType);
	Result->SetStringField(TEXT("target_node_id"), TargetNodeId);
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleAddBTServiceNode(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString ServiceType;
	TSharedPtr<FJsonObject> TypeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("service_type"), ServiceType);
	if (TypeError) return TypeError;

	FString TargetNodeId;
	TSharedPtr<FJsonObject> TargetError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("target_node_id"), TargetNodeId);
	if (TargetError) return TargetError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	FString NodeName;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("node_name"), NodeName, TEXT(""));

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// サービスクラス取得
	UClass* ServiceClass = GetBTServiceClass(ServiceType);
	if (!ServiceClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid service type: %s"), *ServiceType));
	}

	// 対象ノード検索（Compositeノードのみ）
	UBTNode* TargetNode = FindBTNodeById(BehaviorTree, TargetNodeId);
	UBTCompositeNode* TargetComposite = Cast<UBTCompositeNode>(TargetNode);

	if (!TargetComposite)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidOperation,
			FString::Printf(TEXT("Services can only be added to composite nodes. Target: %s"), *TargetNodeId));
	}

	// サービス作成
	UBTService* NewService = NewObject<UBTService>(BehaviorTree, ServiceClass);
	if (!NewService)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to create service node"));
	}

	// ノード名設定
	if (!NodeName.IsEmpty())
	{
		NewService->NodeName = NodeName;
	}

	// サービスを追加
	TargetComposite->Services.Add(NewService);

	// 保存
	BehaviorTree->MarkPackageDirty();
	UPackage* Package = BehaviorTree->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NewService->GetName());
	Result->SetStringField(TEXT("service_type"), ServiceType);
	Result->SetStringField(TEXT("target_node_id"), TargetNodeId);
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	return Result;
}
