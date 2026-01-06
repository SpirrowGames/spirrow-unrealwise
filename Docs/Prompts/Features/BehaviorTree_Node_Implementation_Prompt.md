# BehaviorTree ノード操作ツール実装 (Phase G)

## 概要

Phase F で作成したBehaviorTree/Blackboardの「箱」に対して、実際のAI動作を構築するためのノード操作ツールを実装する。

## 目的

- Compositeノード（Selector, Sequence, Simple Parallel）の追加
- Taskノード（MoveTo, Wait, RotateToFaceBBEntry等）の追加
- Decoratorノード（Blackboard条件、Cooldown、Loop等）の追加
- Serviceノード（定期実行処理）の追加
- ノード間の親子関係設定
- ノードプロパティの設定（BlackboardKey参照等）

---

## アーキテクチャ

```
[MCP Tools - ai_tools.py に追加]
├── add_bt_composite_node()      # Selector/Sequence/SimpleParallel追加
├── add_bt_task_node()           # BTTask追加
├── add_bt_decorator_node()      # Decorator追加
├── add_bt_service_node()        # Service追加
├── connect_bt_nodes()           # ノード親子関係設定
├── set_bt_node_property()       # ノードプロパティ設定
├── delete_bt_node()             # ノード削除
└── list_bt_node_types()         # 利用可能ノードタイプ一覧

[C++ Commands - SpirrowBridgeAICommands に追加]
├── HandleAddBTCompositeNode()
├── HandleAddBTTaskNode()
├── HandleAddBTDecoratorNode()
├── HandleAddBTServiceNode()
├── HandleConnectBTNodes()
├── HandleSetBTNodeProperty()
├── HandleDeleteBTNode()
└── HandleListBTNodeTypes()
```

---

## Part 1: BTノードタイプ対応表

### Compositeノード

| UEクラス | MCPパラメータ | 説明 |
|----------|---------------|------|
| `UBTComposite_Selector` | `"Selector"` | 子ノードを順に試行、成功したら終了 |
| `UBTComposite_Sequence` | `"Sequence"` | 子ノードを順に実行、失敗したら終了 |
| `UBTComposite_SimpleParallel` | `"SimpleParallel"` | メインタスクとバックグラウンドを並列実行 |

### 標準Taskノード

| UEクラス | MCPパラメータ | 説明 |
|----------|---------------|------|
| `UBTTask_MoveTo` | `"BTTask_MoveTo"` | 指定位置へ移動 |
| `UBTTask_MoveDirectlyToward` | `"BTTask_MoveDirectlyToward"` | 直線移動 |
| `UBTTask_Wait` | `"BTTask_Wait"` | 指定時間待機 |
| `UBTTask_WaitBlackboardTime` | `"BTTask_WaitBlackboardTime"` | BB値の時間待機 |
| `UBTTask_PlaySound` | `"BTTask_PlaySound"` | サウンド再生 |
| `UBTTask_PlayAnimation` | `"BTTask_PlayAnimation"` | アニメーション再生 |
| `UBTTask_RotateToFaceBBEntry` | `"BTTask_RotateToFaceBBEntry"` | BB対象の方向を向く |
| `UBTTask_RunBehavior` | `"BTTask_RunBehavior"` | サブBT実行 |
| `UBTTask_RunBehaviorDynamic` | `"BTTask_RunBehaviorDynamic"` | 動的サブBT実行 |
| `UBTTask_SetTagCooldown` | `"BTTask_SetTagCooldown"` | タグクールダウン設定 |
| `UBTTask_GameplayTaskBase` | `"BTTask_GameplayTaskBase"` | GameplayTask基底 |
| `UBTTask_BlueprintBase` | カスタムBP名 | カスタムBP Task |

### 標準Decoratorノード

| UEクラス | MCPパラメータ | 説明 |
|----------|---------------|------|
| `UBTDecorator_Blackboard` | `"BTDecorator_Blackboard"` | BB値条件チェック |
| `UBTDecorator_BlackboardBase` | `"BTDecorator_BlackboardBase"` | BB条件基底 |
| `UBTDecorator_CheckGameplayTagsOnActor` | `"BTDecorator_CheckGameplayTagsOnActor"` | GameplayTag条件 |
| `UBTDecorator_CompareBBEntries` | `"BTDecorator_CompareBBEntries"` | BB値比較 |
| `UBTDecorator_ConditionalLoop` | `"BTDecorator_ConditionalLoop"` | 条件付きループ |
| `UBTDecorator_ConeCheck` | `"BTDecorator_ConeCheck"` | 視錐台チェック |
| `UBTDecorator_Cooldown` | `"BTDecorator_Cooldown"` | クールダウン |
| `UBTDecorator_DoesPathExist` | `"BTDecorator_DoesPathExist"` | パス存在チェック |
| `UBTDecorator_ForceSuccess` | `"BTDecorator_ForceSuccess"` | 強制成功 |
| `UBTDecorator_IsAtLocation` | `"BTDecorator_IsAtLocation"` | 位置到達チェック |
| `UBTDecorator_IsBBEntryOfClass` | `"BTDecorator_IsBBEntryOfClass"` | BBエントリクラスチェック |
| `UBTDecorator_KeepInCone` | `"BTDecorator_KeepInCone"` | 視錐台内維持 |
| `UBTDecorator_Loop` | `"BTDecorator_Loop"` | ループ |
| `UBTDecorator_ReachedMoveGoal` | `"BTDecorator_ReachedMoveGoal"` | 移動目標到達 |
| `UBTDecorator_SetTagCooldown` | `"BTDecorator_SetTagCooldown"` | タグクールダウン設定 |
| `UBTDecorator_TagCooldown` | `"BTDecorator_TagCooldown"` | タグクールダウンチェック |
| `UBTDecorator_TimeLimit` | `"BTDecorator_TimeLimit"` | 時間制限 |
| `UBTDecorator_BlueprintBase` | カスタムBP名 | カスタムBP Decorator |

### 標準Serviceノード

| UEクラス | MCPパラメータ | 説明 |
|----------|---------------|------|
| `UBTService_BlackboardBase` | `"BTService_BlackboardBase"` | BB更新基底 |
| `UBTService_DefaultFocus` | `"BTService_DefaultFocus"` | デフォルトフォーカス設定 |
| `UBTService_RunEQS` | `"BTService_RunEQS"` | EQSクエリ実行 |
| `UBTService_BlueprintBase` | カスタムBP名 | カスタムBP Service |

---

## Part 2: C++ ヘッダー追加 (SpirrowBridgeAICommands.h)

既存のSpirrowBridgeAICommands.hに以下を追加:

```cpp
// 既存の宣言の後に追加

private:
	// ===== BTノード操作ハンドラ =====
	
	/** Compositeノード追加 (Selector, Sequence, SimpleParallel) */
	TSharedPtr<FJsonObject> HandleAddBTCompositeNode(const TSharedPtr<FJsonObject>& Params);
	
	/** Taskノード追加 */
	TSharedPtr<FJsonObject> HandleAddBTTaskNode(const TSharedPtr<FJsonObject>& Params);
	
	/** Decoratorノード追加 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorNode(const TSharedPtr<FJsonObject>& Params);
	
	/** Serviceノード追加 */
	TSharedPtr<FJsonObject> HandleAddBTServiceNode(const TSharedPtr<FJsonObject>& Params);
	
	/** ノード親子関係設定 */
	TSharedPtr<FJsonObject> HandleConnectBTNodes(const TSharedPtr<FJsonObject>& Params);
	
	/** ノードプロパティ設定 */
	TSharedPtr<FJsonObject> HandleSetBTNodeProperty(const TSharedPtr<FJsonObject>& Params);
	
	/** ノード削除 */
	TSharedPtr<FJsonObject> HandleDeleteBTNode(const TSharedPtr<FJsonObject>& Params);
	
	/** 利用可能ノードタイプ一覧 */
	TSharedPtr<FJsonObject> HandleListBTNodeTypes(const TSharedPtr<FJsonObject>& Params);
	
	// ===== BTノード操作ヘルパー =====
	
	/** BTCompositeNodeクラス取得 */
	UClass* GetBTCompositeNodeClass(const FString& TypeString);
	
	/** BTTaskNodeクラス取得 */
	UClass* GetBTTaskNodeClass(const FString& TypeString);
	
	/** BTDecoratorクラス取得 */
	UClass* GetBTDecoratorClass(const FString& TypeString);
	
	/** BTServiceクラス取得 */
	UClass* GetBTServiceClass(const FString& TypeString);
	
	/** BehaviorTreeグラフ内でノードを検索 */
	UBTNode* FindBTNodeById(UBehaviorTree* BehaviorTree, const FString& NodeId);
	
	/** BTノードをJSON形式に変換 */
	TSharedPtr<FJsonObject> BTNodeToJson(UBTNode* Node);
	
	/** ノードタイプ説明取得 */
	FString GetCompositeDescription(const FString& Type);
	FString GetTaskDescription(const FString& Type);
	FString GetDecoratorDescription(const FString& Type);
	FString GetServiceDescription(const FString& Type);
```

---

## Part 3: C++ 実装追加 (SpirrowBridgeAICommands.cpp)

### 3.1 必要なインクルード追加

```cpp
// 既存インクルードの後に追加
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"

// Composites
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"

// Tasks
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_MoveDirectlyToward.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Tasks/BTTask_WaitBlackboardTime.h"
#include "BehaviorTree/Tasks/BTTask_PlaySound.h"
#include "BehaviorTree/Tasks/BTTask_PlayAnimation.h"
#include "BehaviorTree/Tasks/BTTask_RotateToFaceBBEntry.h"
#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"
#include "BehaviorTree/Tasks/BTTask_RunBehaviorDynamic.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"

// Decorators
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Decorators/BTDecorator_CompareBBEntries.h"
#include "BehaviorTree/Decorators/BTDecorator_ConditionalLoop.h"
#include "BehaviorTree/Decorators/BTDecorator_ConeCheck.h"
#include "BehaviorTree/Decorators/BTDecorator_Cooldown.h"
#include "BehaviorTree/Decorators/BTDecorator_DoesPathExist.h"
#include "BehaviorTree/Decorators/BTDecorator_ForceSuccess.h"
#include "BehaviorTree/Decorators/BTDecorator_IsAtLocation.h"
#include "BehaviorTree/Decorators/BTDecorator_KeepInCone.h"
#include "BehaviorTree/Decorators/BTDecorator_Loop.h"
#include "BehaviorTree/Decorators/BTDecorator_TagCooldown.h"
#include "BehaviorTree/Decorators/BTDecorator_TimeLimit.h"
#include "BehaviorTree/Decorators/BTDecorator_BlueprintBase.h"

// Services
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BehaviorTree/Services/BTService_DefaultFocus.h"
#include "BehaviorTree/Services/BTService_RunEQS.h"
#include "BehaviorTree/Services/BTService_BlueprintBase.h"
```

### 3.2 HandleCommand への追加

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// 既存のコマンド...
	
	// BTノード操作コマンド
	else if (CommandType == TEXT("add_bt_composite_node"))
	{
		return HandleAddBTCompositeNode(Params);
	}
	else if (CommandType == TEXT("add_bt_task_node"))
	{
		return HandleAddBTTaskNode(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_node"))
	{
		return HandleAddBTDecoratorNode(Params);
	}
	else if (CommandType == TEXT("add_bt_service_node"))
	{
		return HandleAddBTServiceNode(Params);
	}
	else if (CommandType == TEXT("connect_bt_nodes"))
	{
		return HandleConnectBTNodes(Params);
	}
	else if (CommandType == TEXT("set_bt_node_property"))
	{
		return HandleSetBTNodeProperty(Params);
	}
	else if (CommandType == TEXT("delete_bt_node"))
	{
		return HandleDeleteBTNode(Params);
	}
	else if (CommandType == TEXT("list_bt_node_types"))
	{
		return HandleListBTNodeTypes(Params);
	}
	
	// 既存のエラーハンドリング...
}
```

### 3.3 ヘルパー関数実装

```cpp
// ===== BTノードクラス取得ヘルパー =====

UClass* FSpirrowBridgeAICommands::GetBTCompositeNodeClass(const FString& TypeString)
{
	if (TypeString == TEXT("Selector")) return UBTComposite_Selector::StaticClass();
	if (TypeString == TEXT("Sequence")) return UBTComposite_Sequence::StaticClass();
	if (TypeString == TEXT("SimpleParallel")) return UBTComposite_SimpleParallel::StaticClass();
	return nullptr;
}

UClass* FSpirrowBridgeAICommands::GetBTTaskNodeClass(const FString& TypeString)
{
	if (TypeString == TEXT("BTTask_MoveTo")) return UBTTask_MoveTo::StaticClass();
	if (TypeString == TEXT("BTTask_MoveDirectlyToward")) return UBTTask_MoveDirectlyToward::StaticClass();
	if (TypeString == TEXT("BTTask_Wait")) return UBTTask_Wait::StaticClass();
	if (TypeString == TEXT("BTTask_WaitBlackboardTime")) return UBTTask_WaitBlackboardTime::StaticClass();
	if (TypeString == TEXT("BTTask_PlaySound")) return UBTTask_PlaySound::StaticClass();
	if (TypeString == TEXT("BTTask_PlayAnimation")) return UBTTask_PlayAnimation::StaticClass();
	if (TypeString == TEXT("BTTask_RotateToFaceBBEntry")) return UBTTask_RotateToFaceBBEntry::StaticClass();
	if (TypeString == TEXT("BTTask_RunBehavior")) return UBTTask_RunBehavior::StaticClass();
	if (TypeString == TEXT("BTTask_RunBehaviorDynamic")) return UBTTask_RunBehaviorDynamic::StaticClass();
	
	// カスタムBPタスク検索
	FString BlueprintPath = FString::Printf(TEXT("/Game/AI/Tasks/%s.%s"), *TypeString, *TypeString);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (Blueprint && Blueprint->GeneratedClass)
	{
		return Blueprint->GeneratedClass;
	}
	
	return nullptr;
}

UClass* FSpirrowBridgeAICommands::GetBTDecoratorClass(const FString& TypeString)
{
	if (TypeString == TEXT("BTDecorator_Blackboard")) return UBTDecorator_Blackboard::StaticClass();
	if (TypeString == TEXT("BTDecorator_CompareBBEntries")) return UBTDecorator_CompareBBEntries::StaticClass();
	if (TypeString == TEXT("BTDecorator_ConditionalLoop")) return UBTDecorator_ConditionalLoop::StaticClass();
	if (TypeString == TEXT("BTDecorator_ConeCheck")) return UBTDecorator_ConeCheck::StaticClass();
	if (TypeString == TEXT("BTDecorator_Cooldown")) return UBTDecorator_Cooldown::StaticClass();
	if (TypeString == TEXT("BTDecorator_DoesPathExist")) return UBTDecorator_DoesPathExist::StaticClass();
	if (TypeString == TEXT("BTDecorator_ForceSuccess")) return UBTDecorator_ForceSuccess::StaticClass();
	if (TypeString == TEXT("BTDecorator_IsAtLocation")) return UBTDecorator_IsAtLocation::StaticClass();
	if (TypeString == TEXT("BTDecorator_KeepInCone")) return UBTDecorator_KeepInCone::StaticClass();
	if (TypeString == TEXT("BTDecorator_Loop")) return UBTDecorator_Loop::StaticClass();
	if (TypeString == TEXT("BTDecorator_TagCooldown")) return UBTDecorator_TagCooldown::StaticClass();
	if (TypeString == TEXT("BTDecorator_TimeLimit")) return UBTDecorator_TimeLimit::StaticClass();
	
	// カスタムBPデコレータ検索
	FString BlueprintPath = FString::Printf(TEXT("/Game/AI/Decorators/%s.%s"), *TypeString, *TypeString);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (Blueprint && Blueprint->GeneratedClass)
	{
		return Blueprint->GeneratedClass;
	}
	
	return nullptr;
}

UClass* FSpirrowBridgeAICommands::GetBTServiceClass(const FString& TypeString)
{
	if (TypeString == TEXT("BTService_BlackboardBase")) return UBTService_BlackboardBase::StaticClass();
	if (TypeString == TEXT("BTService_DefaultFocus")) return UBTService_DefaultFocus::StaticClass();
	if (TypeString == TEXT("BTService_RunEQS")) return UBTService_RunEQS::StaticClass();
	
	// カスタムBPサービス検索
	FString BlueprintPath = FString::Printf(TEXT("/Game/AI/Services/%s.%s"), *TypeString, *TypeString);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (Blueprint && Blueprint->GeneratedClass)
	{
		return Blueprint->GeneratedClass;
	}
	
	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::BTNodeToJson(UBTNode* Node)
{
	TSharedPtr<FJsonObject> NodeJson = MakeShareable(new FJsonObject());
	
	if (!Node) return NodeJson;
	
	NodeJson->SetStringField(TEXT("node_id"), Node->GetName());
	NodeJson->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
	NodeJson->SetStringField(TEXT("node_name"), Node->NodeName.IsEmpty() ? Node->GetClass()->GetName() : Node->NodeName);
	
	return NodeJson;
}

UBTNode* FSpirrowBridgeAICommands::FindBTNodeById(UBehaviorTree* BehaviorTree, const FString& NodeId)
{
	if (!BehaviorTree || !BehaviorTree->RootNode) return nullptr;
	
	// 再帰的にノードを検索
	TFunction<UBTNode*(UBTCompositeNode*)> SearchInComposite = [&](UBTCompositeNode* Composite) -> UBTNode*
	{
		if (!Composite) return nullptr;
		
		if (Composite->GetName() == NodeId)
		{
			return Composite;
		}
		
		// デコレータ検索
		for (UBTDecorator* Decorator : Composite->Decorators)
		{
			if (Decorator && Decorator->GetName() == NodeId)
			{
				return Decorator;
			}
		}
		
		// サービス検索
		for (UBTService* Service : Composite->Services)
		{
			if (Service && Service->GetName() == NodeId)
			{
				return Service;
			}
		}
		
		// 子ノード検索
		for (const FBTCompositeChild& Child : Composite->Children)
		{
			if (Child.ChildComposite)
			{
				if (UBTNode* Found = SearchInComposite(Child.ChildComposite))
				{
					return Found;
				}
			}
			if (Child.ChildTask)
			{
				if (Child.ChildTask->GetName() == NodeId)
				{
					return Child.ChildTask;
				}
				// タスクのデコレータ
				for (UBTDecorator* Decorator : Child.ChildTask->Decorators)
				{
					if (Decorator && Decorator->GetName() == NodeId)
					{
						return Decorator;
					}
				}
			}
		}
		
		return nullptr;
	};
	
	return SearchInComposite(BehaviorTree->RootNode);
}

FString FSpirrowBridgeAICommands::GetCompositeDescription(const FString& Type)
{
	if (Type == TEXT("Selector")) return TEXT("Tries children in order until one succeeds");
	if (Type == TEXT("Sequence")) return TEXT("Runs children in order until one fails");
	if (Type == TEXT("SimpleParallel")) return TEXT("Runs main task with background tasks");
	return TEXT("");
}

FString FSpirrowBridgeAICommands::GetTaskDescription(const FString& Type)
{
	if (Type == TEXT("BTTask_MoveTo")) return TEXT("Move to a location or actor");
	if (Type == TEXT("BTTask_MoveDirectlyToward")) return TEXT("Move directly toward target");
	if (Type == TEXT("BTTask_Wait")) return TEXT("Wait for specified seconds");
	if (Type == TEXT("BTTask_WaitBlackboardTime")) return TEXT("Wait using blackboard time value");
	if (Type == TEXT("BTTask_PlaySound")) return TEXT("Play a sound");
	if (Type == TEXT("BTTask_PlayAnimation")) return TEXT("Play an animation");
	if (Type == TEXT("BTTask_RotateToFaceBBEntry")) return TEXT("Rotate to face blackboard entry");
	if (Type == TEXT("BTTask_RunBehavior")) return TEXT("Run a sub-BehaviorTree");
	if (Type == TEXT("BTTask_RunBehaviorDynamic")) return TEXT("Run a dynamic sub-BehaviorTree");
	return TEXT("");
}

FString FSpirrowBridgeAICommands::GetDecoratorDescription(const FString& Type)
{
	if (Type == TEXT("BTDecorator_Blackboard")) return TEXT("Check blackboard value condition");
	if (Type == TEXT("BTDecorator_CompareBBEntries")) return TEXT("Compare two blackboard entries");
	if (Type == TEXT("BTDecorator_Cooldown")) return TEXT("Limit execution frequency");
	if (Type == TEXT("BTDecorator_DoesPathExist")) return TEXT("Check if path exists to target");
	if (Type == TEXT("BTDecorator_ForceSuccess")) return TEXT("Force child to return success");
	if (Type == TEXT("BTDecorator_IsAtLocation")) return TEXT("Check if at location");
	if (Type == TEXT("BTDecorator_Loop")) return TEXT("Loop child execution");
	if (Type == TEXT("BTDecorator_TagCooldown")) return TEXT("Gameplay tag based cooldown");
	if (Type == TEXT("BTDecorator_TimeLimit")) return TEXT("Set time limit for child");
	return TEXT("");
}

FString FSpirrowBridgeAICommands::GetServiceDescription(const FString& Type)
{
	if (Type == TEXT("BTService_DefaultFocus")) return TEXT("Set AI focus target");
	if (Type == TEXT("BTService_RunEQS")) return TEXT("Run Environment Query");
	if (Type == TEXT("BTService_BlackboardBase")) return TEXT("Base class for blackboard updates");
	return TEXT("");
}
```

### 3.4 Compositeノード追加

```cpp
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
```

### 3.5 Taskノード追加

```cpp
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
```

### 3.6 Decoratorノード追加

```cpp
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
	
	// 対象ノードにデコレータを追加
	if (UBTCompositeNode* CompositeNode = Cast<UBTCompositeNode>(TargetNode))
	{
		CompositeNode->Decorators.Add(NewDecorator);
	}
	else if (UBTTaskNode* TaskNode = Cast<UBTTaskNode>(TargetNode))
	{
		TaskNode->Decorators.Add(NewDecorator);
	}
	else
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidOperation,
			TEXT("Decorators can only be added to composite or task nodes"));
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
```

### 3.7 Serviceノード追加

```cpp
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
```

### 3.8 ノード接続

```cpp
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
```

### 3.9 ノードプロパティ設定

```cpp
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
	
	// property_valueを取得
	const TSharedPtr<FJsonValue>* PropertyValuePtr = nullptr;
	if (!Params->TryGetField(TEXT("property_value"), PropertyValuePtr) || !PropertyValuePtr)
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
		TargetNode, PropertyName, *PropertyValuePtr, ErrorMessage);
	
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
```

### 3.10 ノード削除

```cpp
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
		
		// デコレータから削除
		for (int32 i = Parent->Decorators.Num() - 1; i >= 0; --i)
		{
			if (Parent->Decorators[i] == TargetNode)
			{
				Parent->Decorators.RemoveAt(i);
				return true;
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
			
			// タスクのデコレータ
			if (Child.ChildTask)
			{
				for (int32 j = Child.ChildTask->Decorators.Num() - 1; j >= 0; --j)
				{
					if (Child.ChildTask->Decorators[j] == TargetNode)
					{
						Child.ChildTask->Decorators.RemoveAt(j);
						return true;
					}
				}
			}
			
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
```

### 3.11 ノードタイプ一覧

```cpp
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
```

---

## Part 4: Python MCP ツール追加 (ai_tools.py)

既存のai_tools.pyの `register_ai_tools` 関数内に以下を追加:

```python
	# ===== BTノード操作ツール =====

	@mcp.tool()
	def add_bt_composite_node(
		ctx: Context,
		behavior_tree_name: str,
		node_type: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a composite node (Selector, Sequence, SimpleParallel) to a BehaviorTree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			node_type: Type of composite node:
				- "Selector": Tries children until one succeeds
				- "Sequence": Runs children until one fails
				- "SimpleParallel": Runs main task with background tasks
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the node

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- node_id: ID of the created node
			- node_type: Type of the created node

		Example:
			add_bt_composite_node(
				behavior_tree_name="BT_Enemy",
				node_type="Selector",
				node_name="MainSelector"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"node_type": node_type,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding composite node '{node_type}' to BT '{behavior_tree_name}'")
			response = unreal.send_command("add_bt_composite_node", params)

			if response and response.get("success"):
				logger.info(f"Created composite node: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding composite node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def add_bt_task_node(
		ctx: Context,
		behavior_tree_name: str,
		task_type: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a task node to a BehaviorTree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			task_type: Type of task node:
				- "BTTask_MoveTo": Move to location/actor
				- "BTTask_MoveDirectlyToward": Move in straight line
				- "BTTask_Wait": Wait for seconds
				- "BTTask_WaitBlackboardTime": Wait using BB value
				- "BTTask_PlaySound": Play a sound
				- "BTTask_PlayAnimation": Play animation
				- "BTTask_RotateToFaceBBEntry": Rotate to face BB target
				- "BTTask_RunBehavior": Run sub-BehaviorTree
				- Custom BP task name
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the node

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- node_id: ID of the created node
			- task_type: Type of the created task

		Example:
			add_bt_task_node(
				behavior_tree_name="BT_Enemy",
				task_type="BTTask_MoveTo",
				node_name="MoveToTarget"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"task_type": task_type,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding task node '{task_type}' to BT '{behavior_tree_name}'")
			response = unreal.send_command("add_bt_task_node", params)

			if response and response.get("success"):
				logger.info(f"Created task node: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding task node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def add_bt_decorator_node(
		ctx: Context,
		behavior_tree_name: str,
		decorator_type: str,
		target_node_id: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a decorator to a node in a BehaviorTree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			decorator_type: Type of decorator:
				- "BTDecorator_Blackboard": Check BB value condition
				- "BTDecorator_CompareBBEntries": Compare BB values
				- "BTDecorator_Cooldown": Limit execution frequency
				- "BTDecorator_DoesPathExist": Check path existence
				- "BTDecorator_ForceSuccess": Force success result
				- "BTDecorator_IsAtLocation": Check location reached
				- "BTDecorator_Loop": Loop execution
				- "BTDecorator_TagCooldown": Gameplay tag cooldown
				- "BTDecorator_TimeLimit": Set time limit
				- Custom BP decorator name
			target_node_id: ID of the node to attach decorator to
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the decorator

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- node_id: ID of the created decorator
			- decorator_type: Type of the created decorator

		Example:
			add_bt_decorator_node(
				behavior_tree_name="BT_Enemy",
				decorator_type="BTDecorator_Blackboard",
				target_node_id="ChaseSequence",
				node_name="HasTarget"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"decorator_type": decorator_type,
				"target_node_id": target_node_id,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding decorator '{decorator_type}' to node '{target_node_id}'")
			response = unreal.send_command("add_bt_decorator_node", params)

			if response and response.get("success"):
				logger.info(f"Created decorator: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding decorator: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def add_bt_service_node(
		ctx: Context,
		behavior_tree_name: str,
		service_type: str,
		target_node_id: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a service to a composite node in a BehaviorTree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			service_type: Type of service:
				- "BTService_DefaultFocus": Set AI focus target
				- "BTService_RunEQS": Run Environment Query
				- Custom BP service name
			target_node_id: ID of the composite node to attach service to
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the service

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- node_id: ID of the created service
			- service_type: Type of the created service

		Example:
			add_bt_service_node(
				behavior_tree_name="BT_Enemy",
				service_type="BTService_DefaultFocus",
				target_node_id="MainSelector",
				node_name="UpdateFocus"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"service_type": service_type,
				"target_node_id": target_node_id,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding service '{service_type}' to node '{target_node_id}'")
			response = unreal.send_command("add_bt_service_node", params)

			if response and response.get("success"):
				logger.info(f"Created service: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding service: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def connect_bt_nodes(
		ctx: Context,
		behavior_tree_name: str,
		parent_node_id: str,
		child_node_id: str,
		path: str = "/Game/AI/BehaviorTrees",
		child_index: int = -1
	) -> Dict[str, Any]:
		"""
		Connect two nodes in a BehaviorTree (set parent-child relationship).

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			parent_node_id: ID of the parent node ("Root" for root connection)
			child_node_id: ID of the child node to connect
			path: Content browser path where the BehaviorTree is located
			child_index: Position in parent's children (-1 for end)

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- parent_node_id: ID of the parent node
			- child_node_id: ID of the child node

		Example:
			# Connect Selector to Root
			connect_bt_nodes(
				behavior_tree_name="BT_Enemy",
				parent_node_id="Root",
				child_node_id="MainSelector"
			)
			
			# Add task as child of Sequence
			connect_bt_nodes(
				behavior_tree_name="BT_Enemy",
				parent_node_id="ChaseSequence",
				child_node_id="MoveToTask"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"parent_node_id": parent_node_id,
				"child_node_id": child_node_id,
				"path": path,
				"child_index": child_index
			}

			logger.info(f"Connecting '{child_node_id}' to '{parent_node_id}'")
			response = unreal.send_command("connect_bt_nodes", params)

			if response and response.get("success"):
				logger.info(f"Connected nodes successfully")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error connecting nodes: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def set_bt_node_property(
		ctx: Context,
		behavior_tree_name: str,
		node_id: str,
		property_name: str,
		property_value: Any,
		path: str = "/Game/AI/BehaviorTrees"
	) -> Dict[str, Any]:
		"""
		Set a property on a BehaviorTree node.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			node_id: ID of the node to modify
			property_name: Name of the property to set
			property_value: Value to set (type depends on property)
			path: Content browser path where the BehaviorTree is located

		Common Properties:
			BTTask_MoveTo:
				- AcceptableRadius: Distance to consider reached
			BTTask_Wait:
				- WaitTime: Seconds to wait
			BTDecorator_Loop:
				- NumLoops: Number of iterations (0 = infinite)
			BTDecorator_Cooldown:
				- CooldownTime: Seconds between executions

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- node_id: ID of the modified node
			- property_name: Name of the property set

		Example:
			set_bt_node_property(
				behavior_tree_name="BT_Enemy",
				node_id="MoveToTask",
				property_name="AcceptableRadius",
				property_value=100.0
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"node_id": node_id,
				"property_name": property_name,
				"property_value": property_value,
				"path": path
			}

			logger.info(f"Setting property '{property_name}' on node '{node_id}'")
			response = unreal.send_command("set_bt_node_property", params)

			if response and response.get("success"):
				logger.info(f"Property set successfully")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error setting node property: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def delete_bt_node(
		ctx: Context,
		behavior_tree_name: str,
		node_id: str,
		path: str = "/Game/AI/BehaviorTrees"
	) -> Dict[str, Any]:
		"""
		Delete a node from a BehaviorTree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			node_id: ID of the node to delete
			path: Content browser path where the BehaviorTree is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- deleted_node_id: ID of the deleted node

		Example:
			delete_bt_node(
				behavior_tree_name="BT_Enemy",
				node_id="OldTask"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"node_id": node_id,
				"path": path
			}

			logger.info(f"Deleting node '{node_id}' from BT '{behavior_tree_name}'")
			response = unreal.send_command("delete_bt_node", params)

			if response and response.get("success"):
				logger.info(f"Node deleted successfully")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error deleting node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def list_bt_node_types(
		ctx: Context,
		category: str = "all"
	) -> Dict[str, Any]:
		"""
		List available BehaviorTree node types.

		Args:
			category: Filter by category:
				- "all": All node types
				- "composite": Selector, Sequence, etc.
				- "task": Task nodes
				- "decorator": Decorator nodes
				- "service": Service nodes

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- composite_types: List of composite node types
			- task_types: List of task node types
			- decorator_types: List of decorator node types
			- service_types: List of service node types

		Example:
			list_bt_node_types(category="task")
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {"category": category}

			logger.info(f"Listing BT node types (category={category})")
			response = unreal.send_command("list_bt_node_types", params)

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error listing node types: {e}")
			return {"success": False, "error": str(e)}
```

---

## Part 5: SpirrowBridge.cpp ルーティング追加

既存のAI Commandsセクションを更新:

```cpp
// AI Commands
else if (CommandType == TEXT("create_blackboard") ||
         CommandType == TEXT("add_blackboard_key") ||
         CommandType == TEXT("remove_blackboard_key") ||
         CommandType == TEXT("list_blackboard_keys") ||
         CommandType == TEXT("create_behavior_tree") ||
         CommandType == TEXT("set_behavior_tree_blackboard") ||
         CommandType == TEXT("get_behavior_tree_structure") ||
         CommandType == TEXT("list_ai_assets") ||
         // Phase G: BTノード操作
         CommandType == TEXT("add_bt_composite_node") ||
         CommandType == TEXT("add_bt_task_node") ||
         CommandType == TEXT("add_bt_decorator_node") ||
         CommandType == TEXT("add_bt_service_node") ||
         CommandType == TEXT("connect_bt_nodes") ||
         CommandType == TEXT("set_bt_node_property") ||
         CommandType == TEXT("delete_bt_node") ||
         CommandType == TEXT("list_bt_node_types"))
{
    ResultJson = AICommands->HandleCommand(CommandType, Params);
}
```

---

## Part 6: テストシナリオ

### 6.1 基本的なBT構造作成テスト

```python
# 1. BehaviorTree作成（Phase F）
create_behavior_tree(name="BT_TestAI", blackboard_name="BB_TestEnemy")

# 2. Rootに接続するSelectorを作成
result = add_bt_composite_node(
    behavior_tree_name="BT_TestAI",
    node_type="Selector",
    node_name="MainSelector"
)
selector_id = result["node_id"]

# 3. SelectorをRootに接続
connect_bt_nodes(
    behavior_tree_name="BT_TestAI",
    parent_node_id="Root",
    child_node_id=selector_id
)

# 4. Chase Sequenceを追加
result = add_bt_composite_node(
    behavior_tree_name="BT_TestAI",
    node_type="Sequence",
    node_name="ChaseSequence"
)
chase_seq_id = result["node_id"]

# 5. SequenceをSelectorの子に
connect_bt_nodes(
    behavior_tree_name="BT_TestAI",
    parent_node_id=selector_id,
    child_node_id=chase_seq_id
)

# 6. MoveToタスクを追加
result = add_bt_task_node(
    behavior_tree_name="BT_TestAI",
    task_type="BTTask_MoveTo",
    node_name="MoveToTarget"
)
move_task_id = result["node_id"]

# 7. タスクをSequenceの子に
connect_bt_nodes(
    behavior_tree_name="BT_TestAI",
    parent_node_id=chase_seq_id,
    child_node_id=move_task_id
)

# 8. Decoratorを追加
add_bt_decorator_node(
    behavior_tree_name="BT_TestAI",
    decorator_type="BTDecorator_Blackboard",
    target_node_id=chase_seq_id,
    node_name="HasTarget"
)

# 9. プロパティ設定
set_bt_node_property(
    behavior_tree_name="BT_TestAI",
    node_id=move_task_id,
    property_name="AcceptableRadius",
    property_value=50.0
)

# 10. 構造確認
get_behavior_tree_structure(name="BT_TestAI")
```

### 6.2 期待される構造

```
BT_TestAI
└── Root
    └── MainSelector [Selector]
        └── ChaseSequence [Sequence]
            ├── [Decorator] HasTarget (BTDecorator_Blackboard)
            └── MoveToTarget [BTTask_MoveTo]
                └── AcceptableRadius: 50.0
```

---

## Part 7: 注意事項

### 7.1 BehaviorTreeノードの特殊性

- BTノードはUBTNode派生クラス
- 接続はEdGraphのピンではなく、FBTCompositeChildを使用
- Decorator/Serviceはノードに直接アタッチ

### 7.2 ノードIDについて

- ノードIDは`UObject::GetName()`で取得
- 作成時に自動生成される（例: "BTComposite_Selector_0"）
- カスタム名（NodeName）とは別

### 7.3 ビルド依存関係

`SpirrowBridge.Build.cs`に必要（Phase Fで追加済み）:
- `AIModule`
- `GameplayTasks`

---

## 実装順序

1. C++ ヘッダー更新（SpirrowBridgeAICommands.h）
2. C++ インクルード追加
3. HandleCommand更新
4. ヘルパー関数実装
5. 各ハンドラ実装（Composite → Task → Decorator → Service → Connect → Property → Delete → List）
6. SpirrowBridge.cppルーティング追加
7. Python関数追加
8. ビルド＆テスト

---

## 完了条件

- [ ] 全8ツールが動作確認済み
- [ ] テストシナリオで完全なBT構造を構築可能
- [ ] ノードプロパティ（AcceptableRadius等）が設定可能
- [ ] FEATURE_STATUS.md更新
