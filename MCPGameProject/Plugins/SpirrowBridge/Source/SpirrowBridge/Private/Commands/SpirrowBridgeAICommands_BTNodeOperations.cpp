#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// BehaviorTree runtime includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"

// ★ Graph-based includes (UE5.7) ★
#include "EdGraph/EdGraph.h"
#include "AIGraphTypes.h"
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Service.h"
#include "EdGraphSchema_BehaviorTree.h"

// Asset management includes
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

// ===== Helper Functions for Graph-Based Node Operations =====

/**
 * Get the BehaviorTreeGraph from a BehaviorTree asset
 */
static UBehaviorTreeGraph* GetBTGraph(UBehaviorTree* BehaviorTree)
{
	return Cast<UBehaviorTreeGraph>(BehaviorTree->BTGraph);
}

/**
 * Find a graph node by its runtime node ID
 */
static UBehaviorTreeGraphNode* FindGraphNodeByIdInternal(UBehaviorTreeGraph* BTGraph, const FString& NodeId)
{
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(Node);
		if (BTGraphNode && BTGraphNode->NodeInstance)
		{
			if (BTGraphNode->NodeInstance->GetName() == NodeId)
			{
				return BTGraphNode;
			}
		}
	}
	return nullptr;
}

/**
 * Find the Root graph node
 */
static UBehaviorTreeGraphNode_Root* FindRootGraphNodeInternal(UBehaviorTreeGraph* BTGraph)
{
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		if (UBehaviorTreeGraphNode_Root* RootNode = Cast<UBehaviorTreeGraphNode_Root>(Node))
		{
			return RootNode;
		}
	}
	return nullptr;
}

/**
 * Connect two graph nodes via pins
 */
static bool ConnectGraphNodesInternal(UBehaviorTreeGraphNode* ParentGraphNode, UBehaviorTreeGraphNode* ChildGraphNode)
{
	// Get output pin from parent
	UEdGraphPin* ParentOutputPin = nullptr;
	for (UEdGraphPin* Pin : ParentGraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			ParentOutputPin = Pin;
			break;
		}
	}

	// Get input pin from child
	UEdGraphPin* ChildInputPin = nullptr;
	for (UEdGraphPin* Pin : ChildGraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			ChildInputPin = Pin;
			break;
		}
	}

	if (!ParentOutputPin || !ChildInputPin)
	{
		return false;
	}

	// Connect using MakeLinkTo
	ParentOutputPin->MakeLinkTo(ChildInputPin);

	return true;
}

/**
 * Disconnect a graph node from its parent
 */
static void DisconnectGraphNode(UBehaviorTreeGraphNode* GraphNode)
{
	// Break all input pin links
	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			Pin->BreakAllPinLinks();
		}
	}
}

/**
 * Finalize and save the BehaviorTree graph
 */
static void FinalizeAndSaveBTGraphInternal(UBehaviorTreeGraph* BTGraph, UBehaviorTree* BehaviorTree)
{
	// Notify graph changed
	BTGraph->NotifyGraphChanged();

	// Rebuild runtime tree
	BTGraph->UpdateAsset();

	// Mark package dirty
	BehaviorTree->MarkPackageDirty();

	// Save package
	UPackage* Package = BehaviorTree->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);
}

// ===== Phase G: BT Node Operation Handlers (Graph-Based) =====

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

	// ★ Graph取得 ★
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph. Create nodes first using add_bt_composite_node or add_bt_task_node."));
	}

	// ★ 子グラフノード取得 ★
	UBehaviorTreeGraphNode* ChildGraphNode = FindGraphNodeByIdInternal(BTGraph, ChildNodeId);
	if (!ChildGraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Child graph node not found: %s"), *ChildNodeId));
	}

	// Root接続の場合
	if (ParentNodeId.Equals(TEXT("Root"), ESearchCase::IgnoreCase))
	{
		UBehaviorTreeGraphNode_Root* RootGraphNode = FindRootGraphNodeInternal(BTGraph);
		if (!RootGraphNode)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::NodeNotFound,
				TEXT("Root node not found in graph"));
		}

		// 既存の接続を解除
		DisconnectGraphNode(ChildGraphNode);

		// ★ ピン接続でRoot->Childを接続 ★
		bool bConnected = ConnectGraphNodesInternal(RootGraphNode, ChildGraphNode);
		if (!bConnected)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::InvalidOperation,
				TEXT("Failed to connect child to Root node via pins"));
		}
	}
	else
	{
		// ★ 親グラフノード取得 ★
		UBehaviorTreeGraphNode* ParentGraphNode = FindGraphNodeByIdInternal(BTGraph, ParentNodeId);
		if (!ParentGraphNode)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::NodeNotFound,
				FString::Printf(TEXT("Parent graph node not found: %s"), *ParentNodeId));
		}

		// Compositeノードかどうか確認
		UBehaviorTreeGraphNode_Composite* ParentCompositeGraphNode = Cast<UBehaviorTreeGraphNode_Composite>(ParentGraphNode);
		if (!ParentCompositeGraphNode)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::InvalidOperation,
				TEXT("Parent node must be a composite node (Selector/Sequence)"));
		}

		// 既存の接続を解除
		DisconnectGraphNode(ChildGraphNode);

		// ★ ピン接続で Parent->Child を接続 ★
		bool bConnected = ConnectGraphNodesInternal(ParentGraphNode, ChildGraphNode);
		if (!bConnected)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::InvalidOperation,
				TEXT("Failed to connect nodes via pins"));
		}
	}

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

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

	// ★ Graph取得 ★
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// ★ グラフノード取得 ★
	UBehaviorTreeGraphNode* GraphNode = FindGraphNodeByIdInternal(BTGraph, NodeId);
	if (!GraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Graph node not found: %s"), *NodeId));
	}

	// ★ ランタイムノード取得 ★
	UBTNode* TargetNode = Cast<UBTNode>(GraphNode->NodeInstance);
	if (!TargetNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Runtime node not found for graph node: %s"), *NodeId));
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

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

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

	// ★ Graph取得 ★
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// ★ グラフノード取得 ★
	UBehaviorTreeGraphNode* GraphNode = FindGraphNodeByIdInternal(BTGraph, NodeId);
	if (!GraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Graph node not found: %s"), *NodeId));
	}

	// ★ 全てのピン接続を解除 ★
	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		Pin->BreakAllPinLinks();
	}

	// ★ デコレータ/サービスの場合は親ノードから削除 ★
	UBehaviorTreeGraphNode_Decorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
	UBehaviorTreeGraphNode_Service* ServiceNode = Cast<UBehaviorTreeGraphNode_Service>(GraphNode);

	if (DecoratorNode && DecoratorNode->ParentNode)
	{
		UBehaviorTreeGraphNode* ParentBTNode = Cast<UBehaviorTreeGraphNode>(DecoratorNode->ParentNode);
		if (ParentBTNode)
		{
			ParentBTNode->Decorators.Remove(DecoratorNode);
		}
	}
	else if (ServiceNode && ServiceNode->ParentNode)
	{
		UBehaviorTreeGraphNode_Composite* ParentComposite = Cast<UBehaviorTreeGraphNode_Composite>(ServiceNode->ParentNode);
		if (ParentComposite)
		{
			ParentComposite->Services.Remove(ServiceNode);
		}
	}

	// ★ グラフからノードを削除 ★
	BTGraph->RemoveNode(GraphNode);

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

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
