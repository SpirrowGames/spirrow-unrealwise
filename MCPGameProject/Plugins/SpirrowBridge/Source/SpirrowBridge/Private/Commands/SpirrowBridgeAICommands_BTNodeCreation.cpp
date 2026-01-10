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
 * Generate a unique node name based on class name and existing node count
 */
static FName GenerateUniqueNodeName(UBehaviorTreeGraph* BTGraph, UClass* NodeClass)
{
	FString BaseClassName = NodeClass->GetName();

	// Count existing nodes of the same class
	int32 Index = 0;
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(Node);
		if (BTGraphNode && BTGraphNode->NodeInstance &&
			BTGraphNode->NodeInstance->GetClass() == NodeClass)
		{
			Index++;
		}
	}

	// Generate unique name like "BTComposite_Selector_1"
	return FName(*FString::Printf(TEXT("%s_%d"), *BaseClassName, Index));
}

// ===== Auto Position Calculation =====

// レイアウト定数
static constexpr int32 BT_HORIZONTAL_SPACING = 300;
static constexpr int32 BT_VERTICAL_SPACING = 150;
static constexpr int32 BT_NODE_WIDTH = 200;  // ノードの概算幅

/**
 * 親ノードの既存の子ノード数を取得
 */
static int32 GetExistingChildCount(UBehaviorTreeGraphNode* ParentNode)
{
	int32 Count = 0;
	for (UEdGraphPin* Pin : ParentNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			Count = Pin->LinkedTo.Num();
			break;
		}
	}
	return Count;
}

/**
 * 子ノードの自動位置を計算
 * 親ノードの位置と既存の兄弟ノード数から、新しいノードの位置を計算
 */
static void CalculateChildNodePosition(
	UBehaviorTreeGraphNode* ParentNode,
	int32& OutPosX,
	int32& OutPosY)
{
	if (!ParentNode)
	{
		OutPosX = 0;
		OutPosY = BT_VERTICAL_SPACING;
		return;
	}

	// 親の位置を基準にする
	int32 ParentX = ParentNode->NodePosX;
	int32 ParentY = ParentNode->NodePosY;

	// 既存の子ノード数を取得
	int32 ExistingChildren = GetExistingChildCount(ParentNode);

	// Y位置: 親の下
	OutPosY = ParentY + BT_VERTICAL_SPACING;

	// X位置: 子の数に応じて横にずらす
	// 0番目の子 → 親と同じX
	// 1番目以降 → 右にオフセット
	if (ExistingChildren == 0)
	{
		// 最初の子は親の真下
		OutPosX = ParentX;
	}
	else
	{
		// 2番目以降は右にずらす
		// 最終的にはauto_layout_btで再配置されることを想定
		OutPosX = ParentX + (ExistingChildren * BT_HORIZONTAL_SPACING);
	}
}

/**
 * ランタイムツリーのDecorator状態をログ出力（デバッグ用）
 * ★注意: ランタイムでは、DecoratorはFBTCompositeChild::Decoratorsに格納される★
 */
static void LogRuntimeTreeDecorators(UBTCompositeNode* CompositeNode, int32 Depth = 0)
{
	if (!CompositeNode) return;

	FString Indent = FString::ChrN(Depth * 2, TEXT(' '));

	// このCompositeノードの情報
	UE_LOG(LogTemp, Warning, TEXT("%s[Runtime] %s: Children=%d"),
		*Indent,
		*CompositeNode->GetName(),
		CompositeNode->Children.Num());

	// 子ノードを再帰的に処理
	for (int32 i = 0; i < CompositeNode->Children.Num(); i++)
	{
		const FBTCompositeChild& Child = CompositeNode->Children[i];

		// 子ノードにアタッチされたDecorators（★ランタイムではここに格納される★）
		UE_LOG(LogTemp, Warning, TEXT("%s  Child[%d]: ChildNode=%s, Decorators=%d"),
			*Indent, i,
			Child.ChildTask ? *Child.ChildTask->GetName() :
			(Child.ChildComposite ? *Child.ChildComposite->GetName() : TEXT("NULL")),
			Child.Decorators.Num());

		for (int32 j = 0; j < Child.Decorators.Num(); j++)
		{
			UBTDecorator* Dec = Child.Decorators[j];
			UE_LOG(LogTemp, Warning, TEXT("%s    Decorator[%d]: %s"),
				*Indent, j,
				Dec ? *Dec->GetName() : TEXT("NULL"));
		}

		// 子がCompositeなら再帰
		if (Child.ChildComposite)
		{
			LogRuntimeTreeDecorators(Child.ChildComposite, Depth + 2);
		}
	}
}

/**
 * Finalize and save the BehaviorTree graph
 */
static void FinalizeAndSaveBTGraph(UBehaviorTreeGraph* BTGraph, UBehaviorTree* BehaviorTree)
{
	// ★ デバッグログ: UpdateAsset前のグラフ状態 ★
	UE_LOG(LogTemp, Warning, TEXT("=== Before UpdateAsset (Graph) ==="));
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(Node);
		if (BTNode)
		{
			UE_LOG(LogTemp, Warning, TEXT("GraphNode: %s, Decorators: %d"),
				BTNode->NodeInstance ? *BTNode->NodeInstance->GetName() : TEXT("null"),
				BTNode->Decorators.Num());

			// 各Decoratorの詳細
			for (int32 i = 0; i < BTNode->Decorators.Num(); i++)
			{
				UBehaviorTreeGraphNode* DecGraphNode = BTNode->Decorators[i];
				UE_LOG(LogTemp, Warning, TEXT("  Decorator[%d]: GraphNode=%s, NodeInstance=%s"),
					i,
					DecGraphNode ? *DecGraphNode->GetName() : TEXT("NULL"),
					(DecGraphNode && DecGraphNode->NodeInstance) ? *DecGraphNode->NodeInstance->GetName() : TEXT("NULL"));
			}
		}
	}

	// Notify graph changed
	BTGraph->NotifyGraphChanged();

	// Rebuild runtime tree
	BTGraph->UpdateAsset();

	// ★ デバッグログ: UpdateAsset後のグラフ状態 ★
	UE_LOG(LogTemp, Warning, TEXT("=== After UpdateAsset (Graph) ==="));
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(Node);
		if (BTNode)
		{
			UE_LOG(LogTemp, Warning, TEXT("GraphNode: %s, Decorators: %d"),
				BTNode->NodeInstance ? *BTNode->NodeInstance->GetName() : TEXT("null"),
				BTNode->Decorators.Num());

			// 各Decoratorの詳細
			for (int32 i = 0; i < BTNode->Decorators.Num(); i++)
			{
				UBehaviorTreeGraphNode* DecGraphNode = BTNode->Decorators[i];
				UE_LOG(LogTemp, Warning, TEXT("  Decorator[%d]: GraphNode=%s, NodeInstance=%s"),
					i,
					DecGraphNode ? *DecGraphNode->GetName() : TEXT("NULL"),
					(DecGraphNode && DecGraphNode->NodeInstance) ? *DecGraphNode->NodeInstance->GetName() : TEXT("NULL"));
			}
		}
	}

	// ★ デバッグログ: ランタイムツリーの状態 ★
	UE_LOG(LogTemp, Warning, TEXT("=== Runtime Tree Structure ==="));
	if (BehaviorTree->RootNode)
	{
		LogRuntimeTreeDecorators(BehaviorTree->RootNode);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BehaviorTree->RootNode is NULL!"));
	}

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

	// ★ parent_node_id パラメータ（問題1修正）★
	FString ParentNodeId;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("parent_node_id"), ParentNodeId, TEXT(""));

	double ChildIndexDouble = -1.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(
		Params, TEXT("child_index"), ChildIndexDouble, -1.0);
	int32 ChildIndex = static_cast<int32>(ChildIndexDouble);

	// node_position パラメータ（オプション）
	int32 NodePosX = 0;
	int32 NodePosY = 0;
	bool bHasPosition = false;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray = nullptr;
	if (Params->TryGetArrayField(TEXT("node_position"), PositionArray) && PositionArray && PositionArray->Num() >= 2)
	{
		NodePosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		NodePosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
		bHasPosition = true;
	}

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

		// ★ ユニークな名前を生成（問題3修正）★
		FName UniqueName = GenerateUniqueNodeName(BTGraph, NodeClass);

		// ランタイムノード作成（★Outer は GraphNode に★）
		UBTCompositeNode* RuntimeNode = NewObject<UBTCompositeNode>(
			ParallelNode,           // ★重要: GraphNode を Outer に★
			NodeClass,
			UniqueName,             // ★ユニークな名前を使用★
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

		// ★ ユニークな名前を生成（問題3修正）★
		FName UniqueName = GenerateUniqueNodeName(BTGraph, NodeClass);

		// ランタイムノード作成
		UBTCompositeNode* RuntimeNode = NewObject<UBTCompositeNode>(
			CompositeNode,
			NodeClass,
			UniqueName,             // ★ユニークな名前を使用★
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

	// ★ 親ノード取得（位置計算と接続の両方で使う）★
	UBehaviorTreeGraphNode* ParentGraphNode = nullptr;
	if (!ParentNodeId.IsEmpty())
	{
		if (ParentNodeId.Equals(TEXT("Root"), ESearchCase::IgnoreCase))
		{
			ParentGraphNode = FindRootGraphNode(BTGraph);
		}
		else
		{
			ParentGraphNode = FindGraphNodeById(BTGraph, ParentNodeId);
		}
	}

	// ★ ノード位置を設定（自動計算または手動指定）★
	if (bHasPosition)
	{
		// ユーザー指定の位置を使用
		GraphNode->NodePosX = NodePosX;
		GraphNode->NodePosY = NodePosY;
	}
	else if (ParentGraphNode)
	{
		// ★ 自動位置計算: 親ノードから相対位置を計算 ★
		CalculateChildNodePosition(ParentGraphNode, NodePosX, NodePosY);
		GraphNode->NodePosX = NodePosX;
		GraphNode->NodePosY = NodePosY;
		bHasPosition = true;  // レスポンスに含めるため
	}

	// ★ 親ノードへの自動接続（問題1修正）★
	bool bConnected = false;
	if (ParentGraphNode && GraphNode)
	{
		bConnected = ConnectGraphNodes(ParentGraphNode, GraphNode);
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
	if (bHasPosition)
	{
		TArray<TSharedPtr<FJsonValue>> PosArray;
		PosArray.Add(MakeShareable(new FJsonValueNumber(NodePosX)));
		PosArray.Add(MakeShareable(new FJsonValueNumber(NodePosY)));
		Result->SetArrayField(TEXT("node_position"), PosArray);
	}
	if (!ParentNodeId.IsEmpty())
	{
		Result->SetStringField(TEXT("parent_node_id"), ParentNodeId);
		Result->SetBoolField(TEXT("connected"), bConnected);
	}
	Result->SetStringField(TEXT("message"), bConnected ? TEXT("Composite node created and connected to parent.") : TEXT("Composite node created. Use connect_bt_nodes to attach to parent."));
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

	// ★ parent_node_id パラメータ（問題1修正）★
	FString ParentNodeId;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("parent_node_id"), ParentNodeId, TEXT(""));

	double ChildIndexDouble = -1.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(
		Params, TEXT("child_index"), ChildIndexDouble, -1.0);
	int32 ChildIndex = static_cast<int32>(ChildIndexDouble);

	// node_position パラメータ（オプション）
	int32 NodePosX = 0;
	int32 NodePosY = 0;
	bool bHasPosition = false;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray = nullptr;
	if (Params->TryGetArrayField(TEXT("node_position"), PositionArray) && PositionArray && PositionArray->Num() >= 2)
	{
		NodePosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		NodePosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
		bHasPosition = true;
	}

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

		// ★ ユニークな名前を生成（問題3修正）★
		FName UniqueName = GenerateUniqueNodeName(BTGraph, TaskClass);

		// ランタイムノード作成
		UBTTaskNode* RuntimeNode = NewObject<UBTTaskNode>(
			SubtreeNode,
			TaskClass,
			UniqueName,             // ★ユニークな名前を使用★
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

		// ★ ユニークな名前を生成（問題3修正）★
		FName UniqueName = GenerateUniqueNodeName(BTGraph, TaskClass);

		// ランタイムノード作成
		UBTTaskNode* RuntimeNode = NewObject<UBTTaskNode>(
			TaskNode,
			TaskClass,
			UniqueName,             // ★ユニークな名前を使用★
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

	// ★ 親ノード取得（位置計算と接続の両方で使う）★
	UBehaviorTreeGraphNode* ParentGraphNode = nullptr;
	if (!ParentNodeId.IsEmpty())
	{
		if (ParentNodeId.Equals(TEXT("Root"), ESearchCase::IgnoreCase))
		{
			ParentGraphNode = FindRootGraphNode(BTGraph);
		}
		else
		{
			ParentGraphNode = FindGraphNodeById(BTGraph, ParentNodeId);
		}
	}

	// ★ ノード位置を設定（自動計算または手動指定）★
	if (bHasPosition)
	{
		// ユーザー指定の位置を使用
		GraphNode->NodePosX = NodePosX;
		GraphNode->NodePosY = NodePosY;
	}
	else if (ParentGraphNode)
	{
		// ★ 自動位置計算: 親ノードから相対位置を計算 ★
		CalculateChildNodePosition(ParentGraphNode, NodePosX, NodePosY);
		GraphNode->NodePosX = NodePosX;
		GraphNode->NodePosY = NodePosY;
		bHasPosition = true;  // レスポンスに含めるため
	}

	// ★ 親ノードへの自動接続（問題1修正）★
	bool bConnected = false;
	if (ParentGraphNode && GraphNode)
	{
		bConnected = ConnectGraphNodes(ParentGraphNode, GraphNode);
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
	if (bHasPosition)
	{
		TArray<TSharedPtr<FJsonValue>> PosArray;
		PosArray.Add(MakeShareable(new FJsonValueNumber(NodePosX)));
		PosArray.Add(MakeShareable(new FJsonValueNumber(NodePosY)));
		Result->SetArrayField(TEXT("node_position"), PosArray);
	}
	if (!ParentNodeId.IsEmpty())
	{
		Result->SetStringField(TEXT("parent_node_id"), ParentNodeId);
		Result->SetBoolField(TEXT("connected"), bConnected);
	}
	Result->SetStringField(TEXT("message"), bConnected ? TEXT("Task node created and connected to parent.") : TEXT("Task node created. Use connect_bt_nodes to attach to parent."));
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

	// ★ デコレータグラフノード作成（FGraphNodeCreator不使用）★
	// DecoratorはTargetGraphNode->Decorators配列にのみ存在すべき
	// BTGraph->Nodesに追加するとUpdateAsset()で重複処理される
	UBehaviorTreeGraphNode_Decorator* DecoratorGraphNode = NewObject<UBehaviorTreeGraphNode_Decorator>(
		BTGraph,
		UBehaviorTreeGraphNode_Decorator::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	// ★ ユニークな名前を生成（問題3修正）★
	FName UniqueName = GenerateUniqueNodeName(BTGraph, DecoratorClass);

	// ★ ランタイムノード作成（OuterはBehaviorTree - UpdateAsset互換性のため）★
	UBTDecorator* RuntimeDecorator = NewObject<UBTDecorator>(
		BehaviorTree,           // ★重要: BehaviorTreeをOuterに（UpdateAsset互換性）★
		DecoratorClass,
		UniqueName,
		RF_Transactional
	);

	DecoratorGraphNode->NodeInstance = RuntimeDecorator;
	DecoratorGraphNode->ClassData = FGraphNodeClassData(DecoratorClass, TEXT(""));

	if (!NodeName.IsEmpty())
	{
		RuntimeDecorator->NodeName = NodeName;
	}

	// ★ グラフノードの初期化（FGraphNodeCreatorのFinalize相当）★
	DecoratorGraphNode->CreateNewGuid();
	DecoratorGraphNode->PostPlacedNewNode();
	DecoratorGraphNode->AllocateDefaultPins();

	// ★ ターゲットノードのDecorators配列にのみ追加（BTGraph->Nodesには追加しない）★
	TargetGraphNode->Decorators.Add(DecoratorGraphNode);
	DecoratorGraphNode->ParentNode = Cast<UAIGraphNode>(TargetGraphNode);

	FString NodeId = RuntimeDecorator->GetName();

	// ★ デバッグログ追加（問題1調査）★
	UE_LOG(LogTemp, Display, TEXT("Added decorator %s to graph node %s (Decorators count: %d)"),
		*NodeId, *TargetNodeId, TargetGraphNode->Decorators.Num());

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

	// ★ サービスグラフノード作成（FGraphNodeCreator不使用）★
	// ServiceはTargetCompositeGraphNode->Services配列にのみ存在すべき
	// BTGraph->Nodesに追加するとUpdateAsset()で重複処理される
	UBehaviorTreeGraphNode_Service* ServiceGraphNode = NewObject<UBehaviorTreeGraphNode_Service>(
		BTGraph,
		UBehaviorTreeGraphNode_Service::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	// ★ ユニークな名前を生成（問題3修正）★
	FName UniqueName = GenerateUniqueNodeName(BTGraph, ServiceClass);

	// ★ ランタイムノード作成（OuterはBehaviorTree - UpdateAsset互換性のため）★
	UBTService* RuntimeService = NewObject<UBTService>(
		BehaviorTree,           // ★重要: BehaviorTreeをOuterに（UpdateAsset互換性）★
		ServiceClass,
		UniqueName,
		RF_Transactional
	);

	ServiceGraphNode->NodeInstance = RuntimeService;
	ServiceGraphNode->ClassData = FGraphNodeClassData(ServiceClass, TEXT(""));

	if (!NodeName.IsEmpty())
	{
		RuntimeService->NodeName = NodeName;
	}

	// ★ グラフノードの初期化（FGraphNodeCreatorのFinalize相当）★
	ServiceGraphNode->CreateNewGuid();
	ServiceGraphNode->PostPlacedNewNode();
	ServiceGraphNode->AllocateDefaultPins();

	// ★ ターゲットノードのServices配列にのみ追加（BTGraph->Nodesには追加しない）★
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
