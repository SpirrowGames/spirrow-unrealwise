# BehaviorTree ノード接続バグ修正指示書

## 概要

MCPでBehaviorTreeノードを作成・接続すると、ノードが一瞬表示された後に消える問題を修正する。

## 問題の原因

現在の実装では:
1. ノード作成時に `NewObject<UBTTaskNode>(BehaviorTree, TaskClass)` で作成
2. キャッシュ (`PendingBTNodes`) に登録して後から接続
3. 保存時にノードがまだツリーに接続されていないため、シリアライズされない
4. 接続時にノードがGCされているか、ファイルに保存されていない

## 修正方針

**ノード作成と同時にツリーに接続し、即座に保存する**

キャッシュ方式を廃止し、ノード作成APIに `parent_node_id` パラメータを追加して、作成と接続を1回で行う。

---

## タスク1: HandleAddBTCompositeNode の修正

### ファイル
`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands_BTNodeCreation.cpp`

### 修正内容

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

	// 【追加】親ノードID（オプション、指定なしの場合はRootに設定）
	FString ParentNodeId;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("parent_node_id"), ParentNodeId, TEXT(""));

	// 【追加】子インデックス（オプション）
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

	// ノードクラス取得
	UClass* NodeClass = GetBTCompositeNodeClass(NodeType);
	if (!NodeClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid composite node type: %s"), *NodeType));
	}

	// 【修正】ノード作成 - RF_Transactional フラグを追加してGC回避
	UBTCompositeNode* NewNode = NewObject<UBTCompositeNode>(
		BehaviorTree, NodeClass, NAME_None, RF_Transactional);
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

	// ノードIDを取得
	FString NodeId = NewNode->GetName();

	// 【修正】即座にツリーに接続
	if (ParentNodeId.IsEmpty() || ParentNodeId.Equals(TEXT("Root"), ESearchCase::IgnoreCase))
	{
		// Rootとして設定
		BehaviorTree->RootNode = NewNode;
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
		NewChild.ChildComposite = NewNode;

		if (ChildIndex >= 0 && ChildIndex < ParentComposite->Children.Num())
		{
			ParentComposite->Children.Insert(NewChild, ChildIndex);
		}
		else
		{
			ParentComposite->Children.Add(NewChild);
		}
	}

	// 【削除】キャッシュ登録を削除
	// PendingBTNodes への登録を削除

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
	Result->SetStringField(TEXT("node_type"), NodeType);
	Result->SetStringField(TEXT("node_class"), NodeClass->GetName());
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	if (!ParentNodeId.IsEmpty())
	{
		Result->SetStringField(TEXT("parent_node_id"), ParentNodeId);
	}
	return Result;
}
```

---

## タスク2: HandleAddBTTaskNode の修正

### 同じファイル内

### 修正内容

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

	// 【追加】親ノードID（必須に変更）
	FString ParentNodeId;
	TSharedPtr<FJsonObject> ParentError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
		Params, TEXT("parent_node_id"), ParentNodeId);
	if (ParentError) return ParentError;

	// 【追加】子インデックス（オプション）
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

	// タスククラス取得
	UClass* TaskClass = GetBTTaskNodeClass(TaskType);
	if (!TaskClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid task type: %s"), *TaskType));
	}

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

	// 【修正】タスクノード作成 - RF_Transactional フラグを追加
	UBTTaskNode* NewTask = NewObject<UBTTaskNode>(
		BehaviorTree, TaskClass, NAME_None, RF_Transactional);
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

	// ノードIDを取得
	FString NodeId = NewTask->GetName();

	// 【修正】即座に親に接続
	FBTCompositeChild NewChild;
	NewChild.ChildTask = NewTask;

	if (ChildIndex >= 0 && ChildIndex < ParentComposite->Children.Num())
	{
		ParentComposite->Children.Insert(NewChild, ChildIndex);
	}
	else
	{
		ParentComposite->Children.Add(NewChild);
	}

	// 【削除】キャッシュ登録を削除

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
	Result->SetStringField(TEXT("task_type"), TaskType);
	Result->SetStringField(TEXT("node_class"), TaskClass->GetName());
	Result->SetStringField(TEXT("parent_node_id"), ParentNodeId);
	if (!NodeName.IsEmpty())
	{
		Result->SetStringField(TEXT("node_name"), NodeName);
	}
	return Result;
}
```

---

## タスク3: HandleConnectBTNodes の廃止または簡略化

### 修正内容

`connect_bt_nodes` は互換性のため残すが、既存ノードの再配置のみに使用するよう警告を追加。

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

	// 子ノード取得（ツリー内から検索）
	UBTNode* ChildNode = FindBTNodeById(BehaviorTree, ChildNodeId);
	
	// 【修正】キャッシュからの検索を削除し、ツリー内のノードのみを対象にする
	if (!ChildNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::NodeNotFound,
			FString::Printf(TEXT("Child node not found in tree: %s. Use add_bt_composite_node or add_bt_task_node with parent_node_id to create and connect nodes."), *ChildNodeId));
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

		// 【追加】既存の親から切り離す
		RemoveNodeFromParent(BehaviorTree, ChildNode);

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

	// 【削除】キャッシュからの削除処理を削除

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

---

## タスク4: ヘルパー関数追加

### ファイル
`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands_BTNodeHelpers.cpp`

### 追加関数

```cpp
// ノードを親から切り離すヘルパー関数
void FSpirrowBridgeAICommands::RemoveNodeFromParent(UBehaviorTree* BehaviorTree, UBTNode* TargetNode)
{
	if (!BehaviorTree || !BehaviorTree->RootNode || !TargetNode)
	{
		return;
	}

	TFunction<bool(UBTCompositeNode*)> RemoveFromComposite = [&](UBTCompositeNode* Parent) -> bool
	{
		if (!Parent) return false;

		for (int32 i = Parent->Children.Num() - 1; i >= 0; --i)
		{
			FBTCompositeChild& Child = Parent->Children[i];
			if (Child.ChildComposite == TargetNode || Child.ChildTask == TargetNode)
			{
				Parent->Children.RemoveAt(i);
				return true;
			}

			// 再帰的に検索
			if (Child.ChildComposite && RemoveFromComposite(Child.ChildComposite))
			{
				return true;
			}
		}
		return false;
	};

	RemoveFromComposite(BehaviorTree->RootNode);
}
```

### ヘッダーに宣言追加

`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeAICommands.h`

```cpp
private:
	// ヘルパー関数
	static void RemoveNodeFromParent(UBehaviorTree* BehaviorTree, UBTNode* TargetNode);
```

---

## タスク5: PendingBTNodes キャッシュの削除

### ファイル
`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeAICommands.h`

### 削除

```cpp
// 【削除】以下のメンバー変数を削除
// static TMap<FString, TMap<FString, UBTNode*>> PendingBTNodes;
```

### ファイル
`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands_BTNodeHelpers.cpp`

### 削除

```cpp
// 【削除】以下の初期化を削除
// TMap<FString, TMap<FString, UBTNode*>> FSpirrowBridgeAICommands::PendingBTNodes;
```

### FindBTNodeById の修正

キャッシュ検索を削除し、ツリー内のノードのみを検索するように修正。

```cpp
UBTNode* FSpirrowBridgeAICommands::FindBTNodeById(UBehaviorTree* BehaviorTree, const FString& NodeId)
{
	if (!BehaviorTree) return nullptr;

	// 【削除】キャッシュからの検索を削除
	// PendingBTNodes からの検索コードを削除

	// ツリー内を検索
	if (!BehaviorTree->RootNode) return nullptr;

	// RootNode自体をチェック
	if (BehaviorTree->RootNode->GetName() == NodeId)
	{
		return BehaviorTree->RootNode;
	}

	// 再帰的に検索
	TFunction<UBTNode*(UBTCompositeNode*)> SearchInComposite = [&](UBTCompositeNode* Composite) -> UBTNode*
	{
		if (!Composite) return nullptr;

		// サービスを検索
		for (UBTService* Service : Composite->Services)
		{
			if (Service && Service->GetName() == NodeId)
			{
				return Service;
			}
		}

		// 子を検索
		for (const FBTCompositeChild& Child : Composite->Children)
		{
			// デコレータを検索
			for (UBTDecorator* Decorator : Child.Decorators)
			{
				if (Decorator && Decorator->GetName() == NodeId)
				{
					return Decorator;
				}
			}

			// 子Compositeを検索
			if (Child.ChildComposite)
			{
				if (Child.ChildComposite->GetName() == NodeId)
				{
					return Child.ChildComposite;
				}
				if (UBTNode* Found = SearchInComposite(Child.ChildComposite))
				{
					return Found;
				}
			}

			// 子Taskを検索
			if (Child.ChildTask && Child.ChildTask->GetName() == NodeId)
			{
				return Child.ChildTask;
			}
		}

		return nullptr;
	};

	return SearchInComposite(BehaviorTree->RootNode);
}
```

---

## タスク6: MCP Python側の修正

### ファイル
`spirrow-unrealwise/src/spirrow_unrealwise/tools/ai_tools.py`

### 修正内容

`add_bt_task_node` と `add_bt_composite_node` に `parent_node_id` パラメータを追加。

```python
@mcp.tool()
async def add_bt_composite_node(
    ctx: Context,
    behavior_tree_name: str,
    node_type: str,
    parent_node_id: str = "",  # 追加: 空文字列の場合はRoot
    path: str = "/Game/AI/BehaviorTrees",
    node_name: str | None = None,
    child_index: int = -1,  # 追加
) -> dict:
    """
    Add a composite node (Selector/Sequence/SimpleParallel) to a BehaviorTree.

    Args:
        behavior_tree_name: Name of the target BehaviorTree
        node_type: Type of composite node: "Selector", "Sequence", "SimpleParallel"
        parent_node_id: ID of parent node to connect to. Empty or "Root" to set as root node.
        path: Content browser path where the BehaviorTree is located
        node_name: Optional display name for the node
        child_index: Index to insert at (-1 = append to end)

    Returns:
        Dict containing node_id and success status
    """
    # ... 実装
```

```python
@mcp.tool()
async def add_bt_task_node(
    ctx: Context,
    behavior_tree_name: str,
    task_type: str,
    parent_node_id: str,  # 必須に変更
    path: str = "/Game/AI/BehaviorTrees",
    node_name: str | None = None,
    child_index: int = -1,  # 追加
) -> dict:
    """
    Add a task node to a BehaviorTree.

    Args:
        behavior_tree_name: Name of the target BehaviorTree
        task_type: Type of task node (e.g., "BTTask_MoveTo", "BTTask_Wait")
        parent_node_id: ID of parent composite node to connect to (required)
        path: Content browser path where the BehaviorTree is located
        node_name: Optional display name for the node
        child_index: Index to insert at (-1 = append to end)

    Returns:
        Dict containing node_id and success status
    """
    # ... 実装
```

---

## 完了条件チェックリスト

- [ ] HandleAddBTCompositeNode に parent_node_id パラメータ追加
- [ ] HandleAddBTCompositeNode でノード作成と同時に接続
- [ ] HandleAddBTTaskNode に parent_node_id パラメータ追加（必須）
- [ ] HandleAddBTTaskNode でノード作成と同時に接続
- [ ] HandleConnectBTNodes をツリー内ノードの再配置専用に修正
- [ ] RemoveNodeFromParent ヘルパー関数追加
- [ ] PendingBTNodes キャッシュ削除
- [ ] FindBTNodeById からキャッシュ検索削除
- [ ] MCP Python側の add_bt_composite_node 修正
- [ ] MCP Python側の add_bt_task_node 修正
- [ ] SpirrowBridge ビルド成功
- [ ] UE Editor再起動
- [ ] テスト: Selector作成 → Task追加 → 保存後も表示される

---

## テスト手順

1. UE Editorで新しいBehaviorTree `BT_Test` を作成
2. MCPで以下を実行:

```python
# Rootとして Selector を作成
add_bt_composite_node(
    behavior_tree_name="BT_Test",
    node_type="Selector",
    parent_node_id="Root",
    node_name="Main Selector"
)
# → node_id: "BTComposite_Selector_0"

# Selectorの下にTaskを追加
add_bt_task_node(
    behavior_tree_name="BT_Test",
    task_type="BTTask_Wait",
    parent_node_id="BTComposite_Selector_0",
    node_name="Wait 3 Seconds"
)
```

3. UE Editorで BT_Test を開いて、ノードが表示されることを確認
4. UE Editorを再起動して、ノードが保持されていることを確認
