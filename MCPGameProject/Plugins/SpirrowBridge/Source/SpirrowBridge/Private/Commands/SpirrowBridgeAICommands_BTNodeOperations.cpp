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

// ===== Phase G: BT Node Operation Handlers =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleConnectBTNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString ParentNodeId;
	TSharedPtr<FJsonObject> ParentError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("parent_node_id"), ParentNodeId);
	if (ParentError) return ParentError;

	FString ChildNodeId;
	TSharedPtr<FJsonObject> ChildError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("child_node_id"), ChildNodeId);
	if (ChildError) return ChildError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	double ChildIndexDouble = -1.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(
		Params, TEXT("child_index"), ChildIndexDouble, -1.0);
	int32 ChildIndex = static_cast<int32>(ChildIndexDouble);

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// 子ノード取得
	UBTNode* ChildNode = FindBTNodeById(BehaviorTree, ChildNodeId);
	if (!ChildNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Child node not found: %s"), *ChildNodeId));
	}

	// Root接続の場合
	if (ParentNodeId.Equals(TEXT("Root"), ESearchCase::IgnoreCase))
	{
		UBTCompositeNode* RootComposite = Cast<UBTCompositeNode>(ChildNode);
		if (!RootComposite)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::InvalidOperation,
				TEXT("Root node must be a composite node"));
		}
		BehaviorTree->RootNode = RootComposite;
	}
	else
	{
		// 親ノード取得
		UBTNode* ParentNode = FindBTNodeById(BehaviorTree, ParentNodeId);
		if (!ParentNode)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::NodeNotFound,
				FString::Printf(TEXT("Parent node not found: %s"), *ParentNodeId));
		}

		UBTCompositeNode* ParentComposite = Cast<UBTCompositeNode>(ParentNode);
		if (!ParentComposite)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::InvalidOperation,
				TEXT("Parent node must be a composite node (Selector/Sequence)"));
		}

		// 子として追加
		FBTCompositeChild NewChild;
		NewChild.ChildComposite = Cast<UBTCompositeNode>(ChildNode);
		NewChild.ChildTask = Cast<UBTTaskNode>(ChildNode);

		if (ChildIndex >= 0 && ChildIndex < ParentComposite->Children.Num())
		{
			ParentComposite->Children.Insert(NewChild, ChildIndex);
		}
		else
		{
			ParentComposite->Children.Add(NewChild);
		}
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
	Result->SetStringField(TEXT("parent_node_id"), ParentNodeId);
	Result->SetStringField(TEXT("child_node_id"), ChildNodeId);
	if (ChildIndex >= 0)
	{
		Result->SetNumberField(TEXT("child_index"), ChildIndex);
	}
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleSetBTNodeProperty(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString NodeId;
	TSharedPtr<FJsonObject> NodeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("node_id"), NodeId);
	if (NodeError) return NodeError;

	FString PropertyName;
	TSharedPtr<FJsonObject> PropError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("property_name"), PropertyName);
	if (PropError) return PropError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	// property_valueを取得 (UE 5.6+: TryGetField returns TSharedPtr directly)
	TSharedPtr<FJsonValue> PropertyValuePtr = Params->TryGetField(TEXT("property_value"));
	if (!PropertyValuePtr.IsValid())
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::MissingRequiredParam,
			TEXT("Missing required parameter: property_value"));
	}

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// ノード取得
	UBTNode* TargetNode = FindBTNodeById(BehaviorTree, NodeId);
	if (!TargetNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	// プロパティ設定
	FString ErrorMessage;
	bool bSuccess = FSpirrowBridgeCommonUtils::SetObjectProperty(
		TargetNode, PropertyName, PropertyValuePtr, ErrorMessage);

	if (!bSuccess)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::PropertySetFailed,
			FString::Printf(TEXT("Failed to set property %s: %s"), *PropertyName, *ErrorMessage));
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
	Result->SetStringField(TEXT("node_id"), NodeId);
	Result->SetStringField(TEXT("property_name"), PropertyName);
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleDeleteBTNode(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString NodeId;
	TSharedPtr<FJsonObject> NodeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("node_id"), NodeId);
	if (NodeError) return NodeError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// ノード取得
	UBTNode* TargetNode = FindBTNodeById(BehaviorTree, NodeId);
	if (!TargetNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	// Rootノードの場合
	if (BehaviorTree->RootNode == TargetNode)
	{
		BehaviorTree->RootNode = nullptr;
	}

	// 親Compositeから削除する再帰関数
	TFunction<bool(UBTCompositeNode*)> RemoveFromParent = [&](UBTCompositeNode* Parent) -> bool
	{
		if (!Parent) return false;

		// デコレータから削除 (UE 5.6+: decorators are in Children[].Decorators)
		for (FBTCompositeChild& Child : Parent->Children)
		{
			for (int32 i = Child.Decorators.Num() - 1; i >= 0; --i)
			{
				if (Child.Decorators[i] == TargetNode)
				{
					Child.Decorators.RemoveAt(i);
					return true;
				}
			}
		}

		// サービスから削除
		for (int32 i = Parent->Services.Num() - 1; i >= 0; --i)
		{
			if (Parent->Services[i] == TargetNode)
			{
				Parent->Services.RemoveAt(i);
				return true;
			}
		}

		// 子から削除
		for (int32 i = Parent->Children.Num() - 1; i >= 0; --i)
		{
			FBTCompositeChild& Child = Parent->Children[i];
			if (Child.ChildComposite == TargetNode || Child.ChildTask == TargetNode)
			{
				Parent->Children.RemoveAt(i);
				return true;
			}

			// タスクのデコレータ (decorators are in Child.Decorators, not on the task itself)
			// Already checked above in the Child.Decorators loop

			// 再帰的に子Compositeを検索
			if (Child.ChildComposite && RemoveFromParent(Child.ChildComposite))
			{
				return true;
			}
		}

		return false;
	};

	RemoveFromParent(BehaviorTree->RootNode);

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
	Result->SetStringField(TEXT("deleted_node_id"), NodeId);
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleListBTNodeTypes(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Category;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("category"), Category, TEXT("all"));

	TArray<TSharedPtr<FJsonValue>> CompositeTypes;
	TArray<TSharedPtr<FJsonValue>> TaskTypes;
	TArray<TSharedPtr<FJsonValue>> DecoratorTypes;
	TArray<TSharedPtr<FJsonValue>> ServiceTypes;

	// Composite Types
	if (Category == TEXT("all") || Category == TEXT("composite"))
	{
		TArray<FString> Composites = {TEXT("Selector"), TEXT("Sequence"), TEXT("SimpleParallel")};
		for (const FString& Type : Composites)
		{
			TSharedPtr<FJsonObject> TypeObj = MakeShareable(new FJsonObject());
			TypeObj->SetStringField(TEXT("type"), Type);
			TypeObj->SetStringField(TEXT("description"), GetCompositeDescription(Type));
			CompositeTypes.Add(MakeShareable(new FJsonValueObject(TypeObj)));
		}
	}

	// Task Types
	if (Category == TEXT("all") || Category == TEXT("task"))
	{
		TArray<FString> Tasks = {
			TEXT("BTTask_MoveTo"),
			TEXT("BTTask_MoveDirectlyToward"),
			TEXT("BTTask_Wait"),
			TEXT("BTTask_WaitBlackboardTime"),
			TEXT("BTTask_PlaySound"),
			TEXT("BTTask_PlayAnimation"),
			TEXT("BTTask_RotateToFaceBBEntry"),
			TEXT("BTTask_RunBehavior"),
			TEXT("BTTask_RunBehaviorDynamic")
		};
		for (const FString& Type : Tasks)
		{
			TSharedPtr<FJsonObject> TypeObj = MakeShareable(new FJsonObject());
			TypeObj->SetStringField(TEXT("type"), Type);
			TypeObj->SetStringField(TEXT("description"), GetTaskDescription(Type));
			TaskTypes.Add(MakeShareable(new FJsonValueObject(TypeObj)));
		}
	}

	// Decorator Types
	if (Category == TEXT("all") || Category == TEXT("decorator"))
	{
		TArray<FString> Decorators = {
			TEXT("BTDecorator_Blackboard"),
			TEXT("BTDecorator_CompareBBEntries"),
			TEXT("BTDecorator_Cooldown"),
			TEXT("BTDecorator_DoesPathExist"),
			TEXT("BTDecorator_ForceSuccess"),
			TEXT("BTDecorator_IsAtLocation"),
			TEXT("BTDecorator_Loop"),
			TEXT("BTDecorator_TagCooldown"),
			TEXT("BTDecorator_TimeLimit")
		};
		for (const FString& Type : Decorators)
		{
			TSharedPtr<FJsonObject> TypeObj = MakeShareable(new FJsonObject());
			TypeObj->SetStringField(TEXT("type"), Type);
			TypeObj->SetStringField(TEXT("description"), GetDecoratorDescription(Type));
			DecoratorTypes.Add(MakeShareable(new FJsonValueObject(TypeObj)));
		}
	}

	// Service Types
	if (Category == TEXT("all") || Category == TEXT("service"))
	{
		TArray<FString> Services = {
			TEXT("BTService_DefaultFocus"),
			TEXT("BTService_RunEQS"),
			TEXT("BTService_BlackboardBase")
		};
		for (const FString& Type : Services)
		{
			TSharedPtr<FJsonObject> TypeObj = MakeShareable(new FJsonObject());
			TypeObj->SetStringField(TEXT("type"), Type);
			TypeObj->SetStringField(TEXT("description"), GetServiceDescription(Type));
			ServiceTypes.Add(MakeShareable(new FJsonValueObject(TypeObj)));
		}
	}

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("composite_types"), CompositeTypes);
	Result->SetArrayField(TEXT("task_types"), TaskTypes);
	Result->SetArrayField(TEXT("decorator_types"), DecoratorTypes);
	Result->SetArrayField(TEXT("service_types"), ServiceTypes);
	return Result;
}
