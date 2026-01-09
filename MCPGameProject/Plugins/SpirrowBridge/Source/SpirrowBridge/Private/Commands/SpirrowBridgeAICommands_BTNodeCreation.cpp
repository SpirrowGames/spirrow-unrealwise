#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// BehaviorTree runtime includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"

// ★ Graph-based includes (UE5.7) ★
#include "EdGraph/EdGraph.h"
#include "AIGraphTypes.h"
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_SimpleParallel.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "BehaviorTreeGraphNode_SubtreeTask.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Service.h"
#include "EdGraphSchema_BehaviorTree.h"

// Asset management includes
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

// ===== Helper Functions for Graph-Based Node Creation =====

/**
 * Get or create the BehaviorTreeGraph for a BehaviorTree asset
 */
static UBehaviorTreeGraph* GetOrCreateBTGraph(UBehaviorTree* BehaviorTree)
{
	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BehaviorTree->BTGraph);

	if (!BTGraph)
	{
		// Create new graph
		BTGraph = NewObject<UBehaviorTreeGraph>(
			BehaviorTree,
			TEXT("BTGraph"),
			RF_Transactional
		);
		BehaviorTree->BTGraph = BTGraph;

		// Set schema
		BTGraph->Schema = UEdGraphSchema_BehaviorTree::StaticClass();

		// Create default nodes (Root)
		const UEdGraphSchema* Schema = BTGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*BTGraph);
	}

	return BTGraph;
}

/**
 * Find the Root node in the BehaviorTree graph
 */
static UBehaviorTreeGraphNode_Root* FindRootGraphNode(UBehaviorTreeGraph* BTGraph)
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
 * Find a graph node by its runtime node ID
 */
static UBehaviorTreeGraphNode* FindGraphNodeById(UBehaviorTreeGraph* BTGraph, const FString& NodeId)
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
 * Connect two graph nodes via pins
 */
static bool ConnectGraphNodes(UBehaviorTreeGraphNode* ParentGraphNode, UBehaviorTreeGraphNode* ChildGraphNode)
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
 * Finalize and save the BehaviorTree graph
 */
static void FinalizeAndSaveBTGraph(UBehaviorTreeGraph* BTGraph, UBehaviorTree* BehaviorTree)
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

// ===== Phase G: BT Node Creation Handlers (Graph-Based) =====

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

	// ★ Graph取得 ★
	UBehaviorTreeGraph* BTGraph = GetOrCreateBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to get or create BehaviorTree graph"));
	}

	// ★ グラフノード作成 (FGraphNodeCreator パターン) ★
	UBehaviorTreeGraphNode_Composite* GraphNode = nullptr;
	FString NodeId;

	// SimpleParallel用の特別処理
	if (NodeClass->GetName().Contains(TEXT("SimpleParallel")))
	{
		FGraphNodeCreator<UBehaviorTreeGraphNode_SimpleParallel> NodeCreator(*BTGraph);
		UBehaviorTreeGraphNode_SimpleParallel* ParallelNode = NodeCreator.CreateNode();

		// ランタイムノード作成（★Outer は GraphNode に★）
		UBTCompositeNode* RuntimeNode = NewObject<UBTCompositeNode>(
			ParallelNode,           // ★重要: GraphNode を Outer に★
			NodeClass,
			NAME_None,
			RF_Transactional        // ★重要: RF_Transactional フラグ★
		);

		// グラフノードにランタイムノードを関連付け
		ParallelNode->NodeInstance = RuntimeNode;
		ParallelNode->ClassData = FGraphNodeClassData(NodeClass, TEXT(""));

		// ノード名設定
		if (!NodeName.IsEmpty())
		{
			RuntimeNode->NodeName = NodeName;
		}

		// ★重要: Finalize() を必ず呼ぶ（AllocateDefaultPins() が実行される）★
		NodeCreator.Finalize();

		GraphNode = ParallelNode;
		NodeId = RuntimeNode->GetName();
	}
	else
	{
		FGraphNodeCreator<UBehaviorTreeGraphNode_Composite> NodeCreator(*BTGraph);
		UBehaviorTreeGraphNode_Composite* CompositeNode = NodeCreator.CreateNode();

		// ランタイムノード作成
		UBTCompositeNode* RuntimeNode = NewObject<UBTCompositeNode>(
			CompositeNode,
			NodeClass,
			NAME_None,
			RF_Transactional
		);

		// グラフノードにランタイムノードを関連付け
		CompositeNode->NodeInstance = RuntimeNode;
		CompositeNode->ClassData = FGraphNodeClassData(NodeClass, TEXT(""));

		// ノード名設定
		if (!NodeName.IsEmpty())
		{
			RuntimeNode->NodeName = NodeName;
		}

		NodeCreator.Finalize();

		GraphNode = CompositeNode;
		NodeId = RuntimeNode->GetName();
	}

	if (!GraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to create composite graph node"));
	}

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraph(BTGraph, BehaviorTree);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NodeId);
	Result->SetStringField(TEXT("node_type"), NodeType);
	Result->SetStringField(TEXT("node_class"), NodeClass->GetName());
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	Result->SetStringField(TEXT("message"), TEXT("Composite node created. Use connect_bt_nodes to attach to parent."));
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

	// ★ Graph取得 ★
	UBehaviorTreeGraph* BTGraph = GetOrCreateBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to get or create BehaviorTree graph"));
	}

	// ★ グラフノード作成 ★
	UBehaviorTreeGraphNode_Task* GraphNode = nullptr;
	FString NodeId;

	// サブツリータスクかどうかを判定
	bool bIsSubtreeTask = TaskClass->IsChildOf(UBTTask_RunBehavior::StaticClass());

	if (bIsSubtreeTask)
	{
		FGraphNodeCreator<UBehaviorTreeGraphNode_SubtreeTask> NodeCreator(*BTGraph);
		UBehaviorTreeGraphNode_SubtreeTask* SubtreeNode = NodeCreator.CreateNode();

		// ランタイムノード作成
		UBTTaskNode* RuntimeNode = NewObject<UBTTaskNode>(
			SubtreeNode,
			TaskClass,
			NAME_None,
			RF_Transactional
		);

		SubtreeNode->NodeInstance = RuntimeNode;
		SubtreeNode->ClassData = FGraphNodeClassData(TaskClass, TEXT(""));

		if (!NodeName.IsEmpty())
		{
			RuntimeNode->NodeName = NodeName;
		}

		NodeCreator.Finalize();

		GraphNode = SubtreeNode;
		NodeId = RuntimeNode->GetName();
	}
	else
	{
		FGraphNodeCreator<UBehaviorTreeGraphNode_Task> NodeCreator(*BTGraph);
		UBehaviorTreeGraphNode_Task* TaskNode = NodeCreator.CreateNode();

		// ランタイムノード作成
		UBTTaskNode* RuntimeNode = NewObject<UBTTaskNode>(
			TaskNode,
			TaskClass,
			NAME_None,
			RF_Transactional
		);

		TaskNode->NodeInstance = RuntimeNode;
		TaskNode->ClassData = FGraphNodeClassData(TaskClass, TEXT(""));

		if (!NodeName.IsEmpty())
		{
			RuntimeNode->NodeName = NodeName;
		}

		NodeCreator.Finalize();

		GraphNode = TaskNode;
		NodeId = RuntimeNode->GetName();
	}

	if (!GraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to create task graph node"));
	}

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraph(BTGraph, BehaviorTree);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NodeId);
	Result->SetStringField(TEXT("task_type"), TaskType);
	Result->SetStringField(TEXT("node_class"), TaskClass->GetName());
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	Result->SetStringField(TEXT("message"), TEXT("Task node created. Use connect_bt_nodes to attach to parent."));
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

	// ★ Graph取得 ★
	UBehaviorTreeGraph* BTGraph = GetOrCreateBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to get or create BehaviorTree graph"));
	}

	// ★ 対象グラフノード検索 ★
	UBehaviorTreeGraphNode* TargetGraphNode = FindGraphNodeById(BTGraph, TargetNodeId);
	if (!TargetGraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Target graph node not found: %s"), *TargetNodeId));
	}

	// ★ デコレータグラフノード作成 ★
	FGraphNodeCreator<UBehaviorTreeGraphNode_Decorator> NodeCreator(*BTGraph);
	UBehaviorTreeGraphNode_Decorator* DecoratorGraphNode = NodeCreator.CreateNode();

	// ランタイムノード作成
	UBTDecorator* RuntimeDecorator = NewObject<UBTDecorator>(
		DecoratorGraphNode,
		DecoratorClass,
		NAME_None,
		RF_Transactional
	);

	DecoratorGraphNode->NodeInstance = RuntimeDecorator;
	DecoratorGraphNode->ClassData = FGraphNodeClassData(DecoratorClass, TEXT(""));

	if (!NodeName.IsEmpty())
	{
		RuntimeDecorator->NodeName = NodeName;
	}

	NodeCreator.Finalize();

	// ★ ターゲットノードのDecorators配列に追加 ★
	TargetGraphNode->Decorators.Add(DecoratorGraphNode);
	DecoratorGraphNode->ParentNode = Cast<UAIGraphNode>(TargetGraphNode);

	FString NodeId = RuntimeDecorator->GetName();

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraph(BTGraph, BehaviorTree);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NodeId);
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

	// ★ Graph取得 ★
	UBehaviorTreeGraph* BTGraph = GetOrCreateBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("Failed to get or create BehaviorTree graph"));
	}

	// ★ 対象グラフノード検索（Compositeノードのみ）★
	UBehaviorTreeGraphNode* TargetGraphNode = FindGraphNodeById(BTGraph, TargetNodeId);
	if (!TargetGraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Target graph node not found: %s"), *TargetNodeId));
	}

	// Compositeノードかどうか確認
	UBehaviorTreeGraphNode_Composite* TargetCompositeGraphNode = Cast<UBehaviorTreeGraphNode_Composite>(TargetGraphNode);
	if (!TargetCompositeGraphNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidOperation,
			FString::Printf(TEXT("Services can only be added to composite nodes. Target: %s"), *TargetNodeId));
	}

	// ★ サービスグラフノード作成 ★
	FGraphNodeCreator<UBehaviorTreeGraphNode_Service> NodeCreator(*BTGraph);
	UBehaviorTreeGraphNode_Service* ServiceGraphNode = NodeCreator.CreateNode();

	// ランタイムノード作成
	UBTService* RuntimeService = NewObject<UBTService>(
		ServiceGraphNode,
		ServiceClass,
		NAME_None,
		RF_Transactional
	);

	ServiceGraphNode->NodeInstance = RuntimeService;
	ServiceGraphNode->ClassData = FGraphNodeClassData(ServiceClass, TEXT(""));

	if (!NodeName.IsEmpty())
	{
		RuntimeService->NodeName = NodeName;
	}

	NodeCreator.Finalize();

	// ★ ターゲットノードのServices配列に追加 ★
	TargetCompositeGraphNode->Services.Add(ServiceGraphNode);
	ServiceGraphNode->ParentNode = Cast<UAIGraphNode>(TargetCompositeGraphNode);

	FString NodeId = RuntimeService->GetName();

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraph(BTGraph, BehaviorTree);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NodeId);
	Result->SetStringField(TEXT("service_type"), ServiceType);
	Result->SetStringField(TEXT("target_node_id"), TargetNodeId);
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	return Result;
}
