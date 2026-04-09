#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// BehaviorTree runtime includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"      // FBlackboardKeySelector
#include "BehaviorTree/BlackboardData.h"

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

// 匿名名前空間で囲み、Unity ビルド時に BTNodeCreation.cpp の同名 static 関数と衝突しないようにする
namespace
{
/**
 * Generate a unique node name based on class name and existing node count
 */
FName GenerateUniqueNodeName(UBehaviorTreeGraph* BTGraph, UClass* NodeClass)
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
} // namespace

/**
 * Get the BehaviorTreeGraph from a BehaviorTree asset
 */
static UBehaviorTreeGraph* GetBTGraph(UBehaviorTree* BehaviorTree)
{
	return Cast<UBehaviorTreeGraph>(BehaviorTree->BTGraph);
}

/**
 * Find a graph node by its runtime node ID
 * ★ Decorator/Serviceも検索対象に含める（BTGraph->Nodesには含まれない）★
 */
static UBehaviorTreeGraphNode* FindGraphNodeByIdInternal(UBehaviorTreeGraph* BTGraph, const FString& NodeId)
{
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(Node);
		if (BTGraphNode && BTGraphNode->NodeInstance)
		{
			// ★ 通常ノード（Root/Composite/Task）のチェック ★
			if (BTGraphNode->NodeInstance->GetName() == NodeId)
			{
				return BTGraphNode;
			}

			// ★ Decorators配列を検索 ★
			for (UBehaviorTreeGraphNode* DecoratorNode : BTGraphNode->Decorators)
			{
				if (DecoratorNode && DecoratorNode->NodeInstance)
				{
					if (DecoratorNode->NodeInstance->GetName() == NodeId)
					{
						return DecoratorNode;
					}
				}
			}

			// ★ Services配列を検索（Compositeノードのみ）★
			if (UBehaviorTreeGraphNode_Composite* CompositeNode = Cast<UBehaviorTreeGraphNode_Composite>(BTGraphNode))
			{
				for (UBehaviorTreeGraphNode* ServiceNode : CompositeNode->Services)
				{
					if (ServiceNode && ServiceNode->NodeInstance)
					{
						if (ServiceNode->NodeInstance->GetName() == NodeId)
						{
							return ServiceNode;
						}
					}
				}
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

	// ★ BlackboardKey の特別処理（問題2修正）★
	if (PropertyName == TEXT("BlackboardKey") || PropertyName.EndsWith(TEXT("BlackboardKey")))
	{
		// FBlackboardKeySelector を直接設定
		FStructProperty* StructProp = CastField<FStructProperty>(
			TargetNode->GetClass()->FindPropertyByName(*PropertyName));

		if (StructProp && StructProp->Struct == FBlackboardKeySelector::StaticStruct())
		{
			FBlackboardKeySelector* KeySelector = StructProp->ContainerPtrToValuePtr<FBlackboardKeySelector>(TargetNode);

			if (KeySelector)
			{
				// キー名を取得
				FString KeyName;
				if (PropertyValuePtr->Type == EJson::String)
				{
					KeyName = PropertyValuePtr->AsString();
				}
				else if (PropertyValuePtr->Type == EJson::Object)
				{
					TSharedPtr<FJsonObject> KeyObj = PropertyValuePtr->AsObject();
					KeyObj->TryGetStringField(TEXT("SelectedKeyName"), KeyName);
				}

				if (!KeyName.IsEmpty())
				{
					// BehaviorTreeからBlackboardを取得
					UBlackboardData* Blackboard = BehaviorTree->BlackboardAsset;
					if (Blackboard)
					{
						// キーIDを検索
						FBlackboard::FKey KeyID = Blackboard->GetKeyID(FName(*KeyName));

						if (KeyID != FBlackboard::InvalidKey)
						{
							// キー設定
							KeySelector->SelectedKeyName = FName(*KeyName);

							// ★重要: NeedsResolving() = true の場合、ResolveSelectedKey() を呼ぶ
							KeySelector->ResolveSelectedKey(*Blackboard);

							UE_LOG(LogTemp, Display, TEXT("Set BlackboardKey: %s -> KeyID: %d"),
								*KeyName, static_cast<int32>(KeyID));

							// グラフ更新と保存
							FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

							// レスポンス
							TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
							Result->SetBoolField(TEXT("success"), true);
							Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
							Result->SetStringField(TEXT("node_id"), NodeId);
							Result->SetStringField(TEXT("property_name"), PropertyName);
							Result->SetStringField(TEXT("key_name"), KeyName);
							return Result;
						}
						else
						{
							// 利用可能なキーをリスト
							TArray<FString> AvailableKeys;
							for (const FBlackboardEntry& Key : Blackboard->Keys)
							{
								AvailableKeys.Add(Key.EntryName.ToString());
							}

							return FSpirrowBridgeCommonUtils::CreateErrorResponse(
								ESpirrowErrorCode::PropertySetFailed,
								FString::Printf(TEXT("Blackboard key not found: %s. Available keys: %s"),
									*KeyName, *FString::Join(AvailableKeys, TEXT(", "))));
						}
					}
					else
					{
						return FSpirrowBridgeCommonUtils::CreateErrorResponse(
							ESpirrowErrorCode::AssetNotFound,
							TEXT("BehaviorTree has no Blackboard asset assigned"));
					}
				}
			}
		}
	}

	// プロパティ設定（汎用）
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

	// ★ 修正1: ランタイムノードの参照を保持 ★
	UBTNode* RuntimeNode = Cast<UBTNode>(GraphNode->NodeInstance);

	// ★ 修正2: 子ノードの入力ピン接続も解除 ★
	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			// 子ノードへの接続を全て解除
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				LinkedPin->BreakLinkTo(Pin);
			}
		}
		Pin->BreakAllPinLinks();
	}

	// ★ 修正3: デコレータ/サービスの削除処理を強化 ★
	UBehaviorTreeGraphNode_Decorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
	UBehaviorTreeGraphNode_Service* ServiceNode = Cast<UBehaviorTreeGraphNode_Service>(GraphNode);

	if (DecoratorNode)
	{
		// 親ノードのDecorators配列から削除
		if (DecoratorNode->ParentNode)
		{
			UBehaviorTreeGraphNode* ParentBTNode = Cast<UBehaviorTreeGraphNode>(DecoratorNode->ParentNode);
			if (ParentBTNode)
			{
				ParentBTNode->Decorators.Remove(DecoratorNode);
			}
		}
		// ParentNode参照をクリア
		DecoratorNode->ParentNode = nullptr;
	}
	else if (ServiceNode)
	{
		// 親ノードのServices配列から削除
		if (ServiceNode->ParentNode)
		{
			UBehaviorTreeGraphNode_Composite* ParentComposite =
				Cast<UBehaviorTreeGraphNode_Composite>(ServiceNode->ParentNode);
			if (ParentComposite)
			{
				ParentComposite->Services.Remove(ServiceNode);
			}
		}
		ServiceNode->ParentNode = nullptr;
	}
	else
	{
		// ★ 修正4: Composite/Taskノードの場合、付属するDecorator/Serviceも削除 ★
		// Decorators削除
		for (UBehaviorTreeGraphNode* Decorator : GraphNode->Decorators)
		{
			if (Decorator)
			{
				BTGraph->RemoveNode(Decorator);
			}
		}
		GraphNode->Decorators.Empty();

		// Services削除（Compositeノードの場合）
		UBehaviorTreeGraphNode_Composite* CompositeNode =
			Cast<UBehaviorTreeGraphNode_Composite>(GraphNode);
		if (CompositeNode)
		{
			for (UBehaviorTreeGraphNode* SvcGraphNode : CompositeNode->Services)
			{
				if (SvcGraphNode)
				{
					BTGraph->RemoveNode(SvcGraphNode);
				}
			}
			CompositeNode->Services.Empty();
		}
	}

	// ★ 修正5: NodeInstanceの参照をクリア ★
	GraphNode->NodeInstance = nullptr;

	// ★ グラフからノードを削除 ★
	BTGraph->RemoveNode(GraphNode);

	// ★ 修正6: ランタイムノードをマークして次のUpdateAssetで除外されるようにする ★
	if (RuntimeNode)
	{
		RuntimeNode->MarkAsGarbage();
	}

	// ★ グラフ更新と保存 ★
	FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("deleted_node_id"), NodeId);
	return Result;
}

// ===== BT Node Position Handlers =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleSetBTNodePosition(
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

	// position配列を取得
	const TArray<TSharedPtr<FJsonValue>>* PositionArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("position"), PositionArray) || !PositionArray || PositionArray->Num() < 2)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::MissingRequiredParam,
			TEXT("Missing or invalid required parameter: position (must be [X, Y] array)"));
	}

	int32 PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
	int32 PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// Graph取得
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// グラフノード取得
	UBehaviorTreeGraphNode* GraphNode = FindGraphNodeByIdInternal(BTGraph, NodeId);
	if (!GraphNode)
	{
		// "Root"の場合はRootノードを取得
		if (NodeId.Equals(TEXT("Root"), ESearchCase::IgnoreCase))
		{
			GraphNode = FindRootGraphNodeInternal(BTGraph);
		}

		if (!GraphNode)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::NodeNotFound,
				FString::Printf(TEXT("Graph node not found: %s"), *NodeId));
		}
	}

	// 位置を設定
	GraphNode->NodePosX = PosX;
	GraphNode->NodePosY = PosY;

	// グラフ更新と保存
	FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetStringField(TEXT("node_id"), NodeId);

	TArray<TSharedPtr<FJsonValue>> PosArray;
	PosArray.Add(MakeShareable(new FJsonValueNumber(PosX)));
	PosArray.Add(MakeShareable(new FJsonValueNumber(PosY)));
	Result->SetArrayField(TEXT("position"), PosArray);

	return Result;
}

// ===== Auto Layout Helper Functions =====

/**
 * Get all child graph nodes connected to a parent node
 */
static void GetConnectedChildGraphNodes(
	UBehaviorTreeGraphNode* ParentNode,
	TArray<UBehaviorTreeGraphNode*>& OutChildren)
{
	for (UEdGraphPin* Pin : ParentNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (UBehaviorTreeGraphNode* ChildNode = Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode()))
				{
					OutChildren.Add(ChildNode);
				}
			}
		}
	}
}

/**
 * Calculate subtree width recursively for proper spacing
 */
static int32 CalculateSubtreeWidth(UBehaviorTreeGraphNode* Node, int32 HorizontalSpacing)
{
	TArray<UBehaviorTreeGraphNode*> Children;
	GetConnectedChildGraphNodes(Node, Children);

	if (Children.Num() == 0)
	{
		return HorizontalSpacing; // Leaf node width
	}

	int32 TotalWidth = 0;
	for (UBehaviorTreeGraphNode* Child : Children)
	{
		TotalWidth += CalculateSubtreeWidth(Child, HorizontalSpacing);
	}

	return FMath::Max(TotalWidth, HorizontalSpacing);
}

/**
 * Layout child nodes recursively
 */
static int32 LayoutChildNodesRecursive(
	UBehaviorTreeGraphNode* ParentNode,
	int32 StartX,
	int32 CurrentY,
	int32 HorizontalSpacing,
	int32 VerticalSpacing,
	int32& OutNodesLayouted)
{
	TArray<UBehaviorTreeGraphNode*> Children;
	GetConnectedChildGraphNodes(ParentNode, Children);

	if (Children.Num() == 0)
	{
		return StartX + HorizontalSpacing;
	}

	int32 ChildY = CurrentY + VerticalSpacing;
	int32 CurrentX = StartX;

	for (UBehaviorTreeGraphNode* Child : Children)
	{
		// Calculate this child's subtree width
		int32 SubtreeWidth = CalculateSubtreeWidth(Child, HorizontalSpacing);

		// Position child at center of its subtree
		int32 ChildX = CurrentX + SubtreeWidth / 2 - HorizontalSpacing / 2;
		Child->NodePosX = ChildX;
		Child->NodePosY = ChildY;
		OutNodesLayouted++;

		// Layout decorators above the node
		int32 DecoratorY = ChildY - 60;
		for (int32 d = 0; d < Child->Decorators.Num(); ++d)
		{
			Child->Decorators[d]->NodePosX = ChildX;
			Child->Decorators[d]->NodePosY = DecoratorY - d * 50;
			OutNodesLayouted++;
		}

		// Layout services (if this is a composite node)
		UBehaviorTreeGraphNode_Composite* CompositeNode = Cast<UBehaviorTreeGraphNode_Composite>(Child);
		if (CompositeNode)
		{
			int32 ServiceY = ChildY - 40;
			for (int32 s = 0; s < CompositeNode->Services.Num(); ++s)
			{
				CompositeNode->Services[s]->NodePosX = ChildX + 150;
				CompositeNode->Services[s]->NodePosY = ServiceY - s * 40;
				OutNodesLayouted++;
			}
		}

		// Recursively layout grandchildren
		LayoutChildNodesRecursive(Child, CurrentX, ChildY, HorizontalSpacing, VerticalSpacing, OutNodesLayouted);

		// Move to next position
		CurrentX += SubtreeWidth;
	}

	return CurrentX;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleAutoLayoutBT(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

	double HorizontalSpacingDouble = 300.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(
		Params, TEXT("horizontal_spacing"), HorizontalSpacingDouble, 300.0);
	int32 HorizontalSpacing = static_cast<int32>(HorizontalSpacingDouble);

	double VerticalSpacingDouble = 150.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(
		Params, TEXT("vertical_spacing"), VerticalSpacingDouble, 150.0);
	int32 VerticalSpacing = static_cast<int32>(VerticalSpacingDouble);

	// BehaviorTree取得
	UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
	if (!BehaviorTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
	}

	// Graph取得
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// Rootノード取得
	UBehaviorTreeGraphNode_Root* RootNode = FindRootGraphNodeInternal(BTGraph);
	if (!RootNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			TEXT("Root node not found in graph"));
	}

	// Calculate total tree width to center it
	int32 TotalWidth = CalculateSubtreeWidth(RootNode, HorizontalSpacing);
	int32 StartX = -TotalWidth / 2;

	// Rootの位置を設定（ツリーの中央上部）
	RootNode->NodePosX = 0;
	RootNode->NodePosY = 0;

	int32 NodesLayouted = 1; // Root counted

	// 再帰的に子ノードをレイアウト
	LayoutChildNodesRecursive(RootNode, StartX, 0, HorizontalSpacing, VerticalSpacing, NodesLayouted);

	// グラフ更新と保存
	FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetNumberField(TEXT("nodes_layouted"), NodesLayouted);
	Result->SetNumberField(TEXT("horizontal_spacing"), HorizontalSpacing);
	Result->SetNumberField(TEXT("vertical_spacing"), VerticalSpacing);
	return Result;
}

// ===== Helper: Build node info JSON =====

static TSharedPtr<FJsonObject> BuildNodeInfoJson(
	UBehaviorTreeGraphNode* GraphNode,
	const FString& ParentId = TEXT(""))
{
	TSharedPtr<FJsonObject> NodeInfo = MakeShareable(new FJsonObject());

	if (!GraphNode || !GraphNode->NodeInstance)
	{
		return NodeInfo;
	}

	UBTNode* RuntimeNode = Cast<UBTNode>(GraphNode->NodeInstance);

	// 基本情報
	NodeInfo->SetStringField(TEXT("id"), RuntimeNode->GetName());
	NodeInfo->SetStringField(TEXT("class"), RuntimeNode->GetClass()->GetName());

	// ノード名（カスタム名がある場合）
	if (!RuntimeNode->NodeName.IsEmpty())
	{
		NodeInfo->SetStringField(TEXT("name"), RuntimeNode->NodeName);
	}

	// 位置
	TArray<TSharedPtr<FJsonValue>> PosArray;
	PosArray.Add(MakeShareable(new FJsonValueNumber(GraphNode->NodePosX)));
	PosArray.Add(MakeShareable(new FJsonValueNumber(GraphNode->NodePosY)));
	NodeInfo->SetArrayField(TEXT("position"), PosArray);

	// タイプ判定
	if (Cast<UBehaviorTreeGraphNode_Composite>(GraphNode))
	{
		NodeInfo->SetStringField(TEXT("type"), TEXT("Composite"));
	}
	else if (Cast<UBehaviorTreeGraphNode_Task>(GraphNode))
	{
		NodeInfo->SetStringField(TEXT("type"), TEXT("Task"));
	}
	else if (Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode))
	{
		NodeInfo->SetStringField(TEXT("type"), TEXT("Decorator"));

		// attached_to
		UBehaviorTreeGraphNode_Decorator* DecNode =
			Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
		if (DecNode->ParentNode)
		{
			UBehaviorTreeGraphNode* ParentBTNode =
				Cast<UBehaviorTreeGraphNode>(DecNode->ParentNode);
			if (ParentBTNode && ParentBTNode->NodeInstance)
			{
				NodeInfo->SetStringField(TEXT("attached_to"),
					ParentBTNode->NodeInstance->GetName());
			}
		}
		return NodeInfo; // Decoratorは子を持たない
	}
	else if (Cast<UBehaviorTreeGraphNode_Service>(GraphNode))
	{
		NodeInfo->SetStringField(TEXT("type"), TEXT("Service"));

		// attached_to
		UBehaviorTreeGraphNode_Service* SvcNode =
			Cast<UBehaviorTreeGraphNode_Service>(GraphNode);
		if (SvcNode->ParentNode)
		{
			UBehaviorTreeGraphNode* ParentBTNode =
				Cast<UBehaviorTreeGraphNode>(SvcNode->ParentNode);
			if (ParentBTNode && ParentBTNode->NodeInstance)
			{
				NodeInfo->SetStringField(TEXT("attached_to"),
					ParentBTNode->NodeInstance->GetName());
			}
		}
		return NodeInfo; // Serviceは子を持たない
	}

	// 親ノードID
	if (!ParentId.IsEmpty())
	{
		NodeInfo->SetStringField(TEXT("parent"), ParentId);
	}

	// 子ノードID一覧
	TArray<TSharedPtr<FJsonValue>> ChildrenArray;
	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				UBehaviorTreeGraphNode* ChildNode =
					Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode());
				if (ChildNode && ChildNode->NodeInstance)
				{
					ChildrenArray.Add(MakeShareable(
						new FJsonValueString(ChildNode->NodeInstance->GetName())));
				}
			}
		}
	}
	NodeInfo->SetArrayField(TEXT("children"), ChildrenArray);

	// Decorator一覧
	TArray<TSharedPtr<FJsonValue>> DecoratorsArray;
	for (UBehaviorTreeGraphNode* Decorator : GraphNode->Decorators)
	{
		if (Decorator && Decorator->NodeInstance)
		{
			DecoratorsArray.Add(MakeShareable(
				new FJsonValueString(Decorator->NodeInstance->GetName())));
		}
	}
	NodeInfo->SetArrayField(TEXT("decorators"), DecoratorsArray);

	// Service一覧（Compositeノードのみ）
	TArray<TSharedPtr<FJsonValue>> ServicesArray;
	UBehaviorTreeGraphNode_Composite* CompositeNode =
		Cast<UBehaviorTreeGraphNode_Composite>(GraphNode);
	if (CompositeNode)
	{
		for (UBehaviorTreeGraphNode* Service : CompositeNode->Services)
		{
			if (Service && Service->NodeInstance)
			{
				ServicesArray.Add(MakeShareable(
					new FJsonValueString(Service->NodeInstance->GetName())));
			}
		}
	}
	NodeInfo->SetArrayField(TEXT("services"), ServicesArray);

	return NodeInfo;
}

// ===== list_bt_nodes Handler =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleListBTNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

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

	// Graph取得
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// Rootノード情報
	TSharedPtr<FJsonObject> RootInfo = MakeShareable(new FJsonObject());
	UBehaviorTreeGraphNode_Root* RootNode = FindRootGraphNodeInternal(BTGraph);
	if (RootNode)
	{
		RootInfo->SetStringField(TEXT("id"), TEXT("Root"));
		RootInfo->SetStringField(TEXT("type"), TEXT("Root"));

		TArray<TSharedPtr<FJsonValue>> RootPosArray;
		RootPosArray.Add(MakeShareable(new FJsonValueNumber(RootNode->NodePosX)));
		RootPosArray.Add(MakeShareable(new FJsonValueNumber(RootNode->NodePosY)));
		RootInfo->SetArrayField(TEXT("position"), RootPosArray);

		// Rootの子ノード
		TArray<TSharedPtr<FJsonValue>> RootChildrenArray;
		for (UEdGraphPin* Pin : RootNode->Pins)
		{
			if (Pin->Direction == EGPD_Output)
			{
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					UBehaviorTreeGraphNode* ChildNode =
						Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode());
					if (ChildNode && ChildNode->NodeInstance)
					{
						RootChildrenArray.Add(MakeShareable(
							new FJsonValueString(ChildNode->NodeInstance->GetName())));
					}
				}
			}
		}
		RootInfo->SetArrayField(TEXT("children"), RootChildrenArray);
	}

	// 全ノード情報を収集
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	TMap<UBehaviorTreeGraphNode*, FString> NodeParentMap;

	// まず親子関係をマッピング
	for (UEdGraphNode* EdNode : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(EdNode);
		if (!BTNode) continue;

		// Rootノードの子は親が"Root"
		if (RootNode)
		{
			for (UEdGraphPin* Pin : RootNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
					{
						if (LinkedPin->GetOwningNode() == BTNode)
						{
							NodeParentMap.Add(BTNode, TEXT("Root"));
						}
					}
				}
			}
		}

		// 通常の親子関係
		for (UEdGraphPin* Pin : BTNode->Pins)
		{
			if (Pin->Direction == EGPD_Output)
			{
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					UBehaviorTreeGraphNode* ChildNode =
						Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode());
					if (ChildNode && BTNode->NodeInstance)
					{
						NodeParentMap.Add(ChildNode, BTNode->NodeInstance->GetName());
					}
				}
			}
		}
	}

	// ノード情報をJSON化
	int32 TotalNodes = 0;
	for (UEdGraphNode* EdNode : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(EdNode);
		if (!BTNode || !BTNode->NodeInstance) continue;

		// Rootノードはスキップ（別途出力済み）
		if (Cast<UBehaviorTreeGraphNode_Root>(BTNode)) continue;

		// ★ Decorator/Serviceはスキップ（親ノードのDecorators/Services配列から追加するため）★
		if (Cast<UBehaviorTreeGraphNode_Decorator>(BTNode)) continue;
		if (Cast<UBehaviorTreeGraphNode_Service>(BTNode)) continue;

		FString ParentId = NodeParentMap.FindRef(BTNode);
		TSharedPtr<FJsonObject> NodeInfo = BuildNodeInfoJson(BTNode, ParentId);
		NodesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
		TotalNodes++;

		// Decoratorも追加（親ノードのDecorators配列から）
		for (UBehaviorTreeGraphNode* Decorator : BTNode->Decorators)
		{
			if (Decorator && Decorator->NodeInstance)
			{
				TSharedPtr<FJsonObject> DecInfo = BuildNodeInfoJson(Decorator);
				NodesArray.Add(MakeShareable(new FJsonValueObject(DecInfo)));
				TotalNodes++;
			}
		}

		// Serviceも追加（Compositeノードの場合、Services配列から）
		UBehaviorTreeGraphNode_Composite* CompositeNode =
			Cast<UBehaviorTreeGraphNode_Composite>(BTNode);
		if (CompositeNode)
		{
			for (UBehaviorTreeGraphNode* Service : CompositeNode->Services)
			{
				if (Service && Service->NodeInstance)
				{
					TSharedPtr<FJsonObject> SvcInfo = BuildNodeInfoJson(Service);
					NodesArray.Add(MakeShareable(new FJsonValueObject(SvcInfo)));
					TotalNodes++;
				}
			}
		}
	}

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetObjectField(TEXT("root_node"), RootInfo);
	Result->SetArrayField(TEXT("nodes"), NodesArray);
	Result->SetNumberField(TEXT("total_nodes"), TotalNodes);
	return Result;
}

// ===== List BT Node Types =====

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

// ===== detect_broken_bt_nodes Handler =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleDetectBrokenBTNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

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

	// Graph取得
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// 壊れたノードを検出
	TArray<TSharedPtr<FJsonValue>> BrokenNodesArray;
	int32 BrokenCount = 0;

	for (UEdGraphNode* EdNode : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(EdNode);
		if (!BTNode) continue;

		// Rootノードはスキップ
		if (Cast<UBehaviorTreeGraphNode_Root>(BTNode)) continue;

		// NodeInstanceがnullの場合、壊れたノードとして記録
		if (!BTNode->NodeInstance)
		{
			TSharedPtr<FJsonObject> BrokenInfo = MakeShareable(new FJsonObject());

			// ノードタイプを判定
			FString NodeType = TEXT("Unknown");
			if (Cast<UBehaviorTreeGraphNode_Composite>(BTNode))
			{
				NodeType = TEXT("Composite");
			}
			else if (Cast<UBehaviorTreeGraphNode_Task>(BTNode))
			{
				NodeType = TEXT("Task");
			}
			else if (Cast<UBehaviorTreeGraphNode_Decorator>(BTNode))
			{
				NodeType = TEXT("Decorator");
			}
			else if (Cast<UBehaviorTreeGraphNode_Service>(BTNode))
			{
				NodeType = TEXT("Service");
			}

			BrokenInfo->SetStringField(TEXT("node_type"), NodeType);
			BrokenInfo->SetStringField(TEXT("node_class"), BTNode->GetClass()->GetName());

			// Position情報
			TArray<TSharedPtr<FJsonValue>> PosArray;
			PosArray.Add(MakeShareable(new FJsonValueNumber(BTNode->NodePosX)));
			PosArray.Add(MakeShareable(new FJsonValueNumber(BTNode->NodePosY)));
			BrokenInfo->SetArrayField(TEXT("position"), PosArray);

			// GraphNode自体のポインタ情報（削除用の識別子として使用）
			BrokenInfo->SetStringField(TEXT("graph_node_ptr"),
				FString::Printf(TEXT("%p"), BTNode));

			BrokenNodesArray.Add(MakeShareable(new FJsonValueObject(BrokenInfo)));
			BrokenCount++;
		}

		// Decoratorの中にも壊れたものがないかチェック
		for (UBehaviorTreeGraphNode* Decorator : BTNode->Decorators)
		{
			if (Decorator && !Decorator->NodeInstance)
			{
				TSharedPtr<FJsonObject> BrokenInfo = MakeShareable(new FJsonObject());
				BrokenInfo->SetStringField(TEXT("node_type"), TEXT("Decorator"));
				BrokenInfo->SetStringField(TEXT("node_class"), Decorator->GetClass()->GetName());

				TArray<TSharedPtr<FJsonValue>> PosArray;
				PosArray.Add(MakeShareable(new FJsonValueNumber(Decorator->NodePosX)));
				PosArray.Add(MakeShareable(new FJsonValueNumber(Decorator->NodePosY)));
				BrokenInfo->SetArrayField(TEXT("position"), PosArray);

				BrokenInfo->SetStringField(TEXT("graph_node_ptr"),
					FString::Printf(TEXT("%p"), Decorator));
				BrokenInfo->SetStringField(TEXT("attached_to"),
					BTNode->NodeInstance ? BTNode->NodeInstance->GetName() : TEXT("Unknown"));

				BrokenNodesArray.Add(MakeShareable(new FJsonValueObject(BrokenInfo)));
				BrokenCount++;
			}
		}

		// Serviceの中にも壊れたものがないかチェック
		UBehaviorTreeGraphNode_Composite* CompositeNode = Cast<UBehaviorTreeGraphNode_Composite>(BTNode);
		if (CompositeNode)
		{
			for (UBehaviorTreeGraphNode* Service : CompositeNode->Services)
			{
				if (Service && !Service->NodeInstance)
				{
					TSharedPtr<FJsonObject> BrokenInfo = MakeShareable(new FJsonObject());
					BrokenInfo->SetStringField(TEXT("node_type"), TEXT("Service"));
					BrokenInfo->SetStringField(TEXT("node_class"), Service->GetClass()->GetName());

					TArray<TSharedPtr<FJsonValue>> PosArray;
					PosArray.Add(MakeShareable(new FJsonValueNumber(Service->NodePosX)));
					PosArray.Add(MakeShareable(new FJsonValueNumber(Service->NodePosY)));
					BrokenInfo->SetArrayField(TEXT("position"), PosArray);

					BrokenInfo->SetStringField(TEXT("graph_node_ptr"),
						FString::Printf(TEXT("%p"), Service));
					BrokenInfo->SetStringField(TEXT("attached_to"),
						BTNode->NodeInstance ? BTNode->NodeInstance->GetName() : TEXT("Unknown"));

					BrokenNodesArray.Add(MakeShareable(new FJsonValueObject(BrokenInfo)));
					BrokenCount++;
				}
			}
		}
	}

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetArrayField(TEXT("broken_nodes"), BrokenNodesArray);
	Result->SetNumberField(TEXT("broken_count"), BrokenCount);

	if (BrokenCount > 0)
	{
		Result->SetStringField(TEXT("message"),
			FString::Printf(TEXT("Found %d broken node(s) with null NodeInstance"), BrokenCount));
	}
	else
	{
		Result->SetStringField(TEXT("message"), TEXT("No broken nodes found"));
	}

	return Result;
}

// ===== delete_broken_bt_nodes Handler =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleDeleteBrokenBTNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

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

	// Graph取得
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// 壊れたノードを収集
	TArray<UBehaviorTreeGraphNode*> NodesToDelete;

	for (UEdGraphNode* EdNode : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(EdNode);
		if (!BTNode) continue;

		// Rootノードはスキップ
		if (Cast<UBehaviorTreeGraphNode_Root>(BTNode)) continue;

		// NodeInstanceがnullの場合、削除対象
		if (!BTNode->NodeInstance)
		{
			NodesToDelete.Add(BTNode);
		}

		// Decoratorの中にも壊れたものがないかチェック
		for (int32 i = BTNode->Decorators.Num() - 1; i >= 0; --i)
		{
			UBehaviorTreeGraphNode* Decorator = BTNode->Decorators[i];
			if (Decorator && !Decorator->NodeInstance)
			{
				NodesToDelete.Add(Decorator);
			}
		}

		// Serviceの中にも壊れたものがないかチェック
		UBehaviorTreeGraphNode_Composite* CompositeNode = Cast<UBehaviorTreeGraphNode_Composite>(BTNode);
		if (CompositeNode)
		{
			for (int32 i = CompositeNode->Services.Num() - 1; i >= 0; --i)
			{
				UBehaviorTreeGraphNode* Service = CompositeNode->Services[i];
				if (Service && !Service->NodeInstance)
				{
					NodesToDelete.Add(Service);
				}
			}
		}
	}

	// 削除実行
	int32 DeletedCount = 0;
	TArray<TSharedPtr<FJsonValue>> DeletedNodesArray;

	for (UBehaviorTreeGraphNode* NodeToDelete : NodesToDelete)
	{
		TSharedPtr<FJsonObject> DeletedInfo = MakeShareable(new FJsonObject());

		// ノードタイプを判定
		FString NodeType = TEXT("Unknown");
		if (Cast<UBehaviorTreeGraphNode_Composite>(NodeToDelete))
		{
			NodeType = TEXT("Composite");
		}
		else if (Cast<UBehaviorTreeGraphNode_Task>(NodeToDelete))
		{
			NodeType = TEXT("Task");
		}
		else if (Cast<UBehaviorTreeGraphNode_Decorator>(NodeToDelete))
		{
			NodeType = TEXT("Decorator");
		}
		else if (Cast<UBehaviorTreeGraphNode_Service>(NodeToDelete))
		{
			NodeType = TEXT("Service");
		}

		DeletedInfo->SetStringField(TEXT("node_type"), NodeType);
		DeletedInfo->SetStringField(TEXT("node_class"), NodeToDelete->GetClass()->GetName());

		// ★ 重要: HandleDeleteBTNodeと同じ削除処理を適用 ★

		// RuntimeNodeの参照を保持
		UBTNode* RuntimeNode = Cast<UBTNode>(NodeToDelete->NodeInstance);

		// ピン接続を解除
		for (UEdGraphPin* Pin : NodeToDelete->Pins)
		{
			if (Pin->Direction == EGPD_Output)
			{
				// 子ノードへの接続を全て解除
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					LinkedPin->BreakLinkTo(Pin);
				}
			}
			Pin->BreakAllPinLinks();
		}

		// Decorator/Serviceの場合、親配列から削除
		UBehaviorTreeGraphNode_Decorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(NodeToDelete);
		UBehaviorTreeGraphNode_Service* ServiceNode = Cast<UBehaviorTreeGraphNode_Service>(NodeToDelete);

		if (DecoratorNode)
		{
			// 親ノードのDecorators配列から削除
			if (DecoratorNode->ParentNode)
			{
				UBehaviorTreeGraphNode* ParentBTNode = Cast<UBehaviorTreeGraphNode>(DecoratorNode->ParentNode);
				if (ParentBTNode)
				{
					ParentBTNode->Decorators.Remove(DecoratorNode);
				}
			}
			DecoratorNode->ParentNode = nullptr;
		}
		else if (ServiceNode)
		{
			// 親ノードのServices配列から削除
			if (ServiceNode->ParentNode)
			{
				UBehaviorTreeGraphNode_Composite* ParentComposite =
					Cast<UBehaviorTreeGraphNode_Composite>(ServiceNode->ParentNode);
				if (ParentComposite)
				{
					ParentComposite->Services.Remove(ServiceNode);
				}
			}
			ServiceNode->ParentNode = nullptr;
		}

		// NodeInstanceの参照をクリア（壊れたノードは既にnullだが念のため）
		NodeToDelete->NodeInstance = nullptr;

		// グラフからノードを削除
		BTGraph->RemoveNode(NodeToDelete);

		// RuntimeNodeをMarkAsGarbageして次のUpdateAssetで除外
		if (RuntimeNode)
		{
			RuntimeNode->MarkAsGarbage();
		}

		DeletedNodesArray.Add(MakeShareable(new FJsonValueObject(DeletedInfo)));
		DeletedCount++;
	}

	// グラフ更新と保存
	if (DeletedCount > 0)
	{
		FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);
	}

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetArrayField(TEXT("deleted_nodes"), DeletedNodesArray);
	Result->SetNumberField(TEXT("deleted_count"), DeletedCount);

	if (DeletedCount > 0)
	{
		Result->SetStringField(TEXT("message"),
			FString::Printf(TEXT("Deleted %d broken node(s)"), DeletedCount));
	}
	else
	{
		Result->SetStringField(TEXT("message"), TEXT("No broken nodes to delete"));
	}

	return Result;
}

// ===== repair_broken_bt_nodes Handler =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleRepairBrokenBTNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ取得
	FString BehaviorTreeName;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("behavior_tree_name"), BehaviorTreeName);
	if (NameError) return NameError;

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

	// Graph取得
	UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
	if (!BTGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeCreationFailed,
			TEXT("BehaviorTree has no graph"));
	}

	// 壊れたノードを収集して修復を試みる
	int32 RepairedCount = 0;
	int32 FailedCount = 0;
	TArray<TSharedPtr<FJsonValue>> RepairedNodesArray;
	TArray<TSharedPtr<FJsonValue>> FailedNodesArray;

	// ヘルパーラムダ: 壊れたノードの修復を試みる
	auto TryRepairNode = [&](UBehaviorTreeGraphNode* BrokenNode, const FString& NodeType, const FString& AttachedTo)
	{
		if (!BrokenNode || BrokenNode->NodeInstance)
		{
			return; // nullまたは壊れていない
		}

		TSharedPtr<FJsonObject> NodeInfo = MakeShareable(new FJsonObject());
		NodeInfo->SetStringField(TEXT("node_type"), NodeType);
		NodeInfo->SetStringField(TEXT("graph_node_class"), BrokenNode->GetClass()->GetName());
		if (!AttachedTo.IsEmpty())
		{
			NodeInfo->SetStringField(TEXT("attached_to"), AttachedTo);
		}

		// ClassDataからクラスを再解決
		FGraphNodeClassData& ClassData = BrokenNode->ClassData;
		FString ClassName = ClassData.GetClassName();

		if (ClassName.IsEmpty())
		{
			NodeInfo->SetStringField(TEXT("error"), TEXT("ClassData has no class name - cannot repair"));
			FailedNodesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
			FailedCount++;
			return;
		}

		// _C サフィックスを除去（Blueprint生成クラス名対応）
		FString CleanClassName = ClassName;
		CleanClassName.RemoveFromEnd(TEXT("_C"));

		NodeInfo->SetStringField(TEXT("class_name"), CleanClassName);

		// クラス検索: まずClassDataに保存されているクラスを試す
		UClass* ResolvedClass = nullptr;

		// 方法1: ClassData自体からクラスを取得（アセットパスが保存されている場合）
		// GetClass()でクラス参照を取得
		const UClass* StoredClass = ClassData.GetClass();
		if (StoredClass)
		{
			ResolvedClass = const_cast<UClass*>(StoredClass);
		}

		// 方法2: クラス名から検索（エンジン標準クラス）
		if (!ResolvedClass)
		{
			// Decorator クラスの場合
			if (NodeType == TEXT("Decorator"))
			{
				ResolvedClass = GetBTDecoratorClass(CleanClassName);
				// BTDecorator_ プレフィックスなしでも試行
				if (!ResolvedClass && !CleanClassName.StartsWith(TEXT("BTDecorator_")))
				{
					ResolvedClass = GetBTDecoratorClass(FString::Printf(TEXT("BTDecorator_%s"), *CleanClassName));
				}
			}
			// Service クラスの場合
			else if (NodeType == TEXT("Service"))
			{
				ResolvedClass = GetBTServiceClass(CleanClassName);
				if (!ResolvedClass && !CleanClassName.StartsWith(TEXT("BTService_")))
				{
					ResolvedClass = GetBTServiceClass(FString::Printf(TEXT("BTService_%s"), *CleanClassName));
				}
			}
			// Task クラスの場合
			else if (NodeType == TEXT("Task"))
			{
				ResolvedClass = GetBTTaskNodeClass(CleanClassName);
				if (!ResolvedClass && !CleanClassName.StartsWith(TEXT("BTTask_")))
				{
					ResolvedClass = GetBTTaskNodeClass(FString::Printf(TEXT("BTTask_%s"), *CleanClassName));
				}
			}
			// Composite クラスの場合
			else if (NodeType == TEXT("Composite"))
			{
				ResolvedClass = GetBTCompositeNodeClass(CleanClassName);
				if (!ResolvedClass && !CleanClassName.StartsWith(TEXT("BTComposite_")))
				{
					ResolvedClass = GetBTCompositeNodeClass(FString::Printf(TEXT("BTComposite_%s"), *CleanClassName));
				}
			}
		}

		// 方法3: FindFirstObject によるグローバル検索
		if (!ResolvedClass)
		{
			ResolvedClass = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::None);
		}
		if (!ResolvedClass)
		{
			ResolvedClass = FindFirstObject<UClass>(*CleanClassName, EFindFirstObjectOptions::None);
		}

		if (!ResolvedClass)
		{
			NodeInfo->SetStringField(TEXT("error"),
				FString::Printf(TEXT("Could not resolve class: %s"), *ClassName));
			FailedNodesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
			FailedCount++;
			return;
		}

		NodeInfo->SetStringField(TEXT("resolved_class"), ResolvedClass->GetName());

		// ★ NodeInstanceを再生成 ★
		// Outerの決定: Decorator/ServiceはBehaviorTree、Composite/TaskはGraphNode
		UObject* Outer = nullptr;
		if (NodeType == TEXT("Decorator") || NodeType == TEXT("Service"))
		{
			Outer = BehaviorTree;  // UpdateAsset()互換性のため
		}
		else
		{
			Outer = BrokenNode;    // 構造階層のため
		}

		FName UniqueName = GenerateUniqueNodeName(BTGraph, ResolvedClass);

		UBTNode* NewNodeInstance = NewObject<UBTNode>(
			Outer,
			ResolvedClass,
			UniqueName,
			RF_Transactional
		);

		if (!NewNodeInstance)
		{
			NodeInfo->SetStringField(TEXT("error"),
				FString::Printf(TEXT("NewObject failed for class: %s"), *ResolvedClass->GetName()));
			FailedNodesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
			FailedCount++;
			return;
		}

		// NodeInstance設定
		BrokenNode->NodeInstance = NewNodeInstance;

		// ClassData更新
		BrokenNode->ClassData = FGraphNodeClassData(ResolvedClass, TEXT(""));

		// ピンを再初期化
		BrokenNode->AllocateDefaultPins();

		NodeInfo->SetStringField(TEXT("status"), TEXT("repaired"));
		NodeInfo->SetStringField(TEXT("new_node_id"), NewNodeInstance->GetName());
		RepairedNodesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
		RepairedCount++;

		UE_LOG(LogTemp, Display, TEXT("Repaired broken %s node: class=%s, new_instance=%s"),
			*NodeType, *ResolvedClass->GetName(), *NewNodeInstance->GetName());
	};

	// BTGraph->Nodesを走査
	for (UEdGraphNode* EdNode : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(EdNode);
		if (!BTNode) continue;

		// Rootノードはスキップ
		if (Cast<UBehaviorTreeGraphNode_Root>(BTNode)) continue;

		// メインノード（Composite/Task）の修復
		if (!BTNode->NodeInstance)
		{
			FString NodeType = TEXT("Unknown");
			if (Cast<UBehaviorTreeGraphNode_Composite>(BTNode))
			{
				NodeType = TEXT("Composite");
			}
			else if (Cast<UBehaviorTreeGraphNode_Task>(BTNode))
			{
				NodeType = TEXT("Task");
			}
			TryRepairNode(BTNode, NodeType, TEXT(""));
		}

		// ノード名を取得（Decorator/Serviceのattached_to表示用）
		FString ParentNodeName = BTNode->NodeInstance ? BTNode->NodeInstance->GetName() : TEXT("Unknown");

		// Decoratorの修復
		for (UBehaviorTreeGraphNode* Decorator : BTNode->Decorators)
		{
			if (Decorator && !Decorator->NodeInstance)
			{
				TryRepairNode(Decorator, TEXT("Decorator"), ParentNodeName);
			}
		}

		// Serviceの修復（Compositeノードのみ）
		UBehaviorTreeGraphNode_Composite* CompositeNode = Cast<UBehaviorTreeGraphNode_Composite>(BTNode);
		if (CompositeNode)
		{
			for (UBehaviorTreeGraphNode* Service : CompositeNode->Services)
			{
				if (Service && !Service->NodeInstance)
				{
					TryRepairNode(Service, TEXT("Service"), ParentNodeName);
				}
			}
		}
	}

	// 修復があった場合、グラフ更新と保存
	if (RepairedCount > 0)
	{
		FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);
	}

	// レスポンス
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
	Result->SetArrayField(TEXT("repaired_nodes"), RepairedNodesArray);
	Result->SetNumberField(TEXT("repaired_count"), RepairedCount);
	Result->SetArrayField(TEXT("failed_nodes"), FailedNodesArray);
	Result->SetNumberField(TEXT("failed_count"), FailedCount);

	if (RepairedCount > 0 || FailedCount > 0)
	{
		Result->SetStringField(TEXT("message"),
			FString::Printf(TEXT("Repaired %d node(s), %d failed"), RepairedCount, FailedCount));
	}
	else
	{
		Result->SetStringField(TEXT("message"), TEXT("No broken nodes found"));
	}

	return Result;
}
