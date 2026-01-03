# 機能名: ForLoopWithBreak ノード追加

## 概要

`add_foreach_loop_node` がマクロのため動作しない問題を解決するため、代替として `add_forloop_with_break_node` を実装する。

## 背景

- `ForEachLoop` はBlueprintマクロとして実装されており、通常のノード追加方法では作成できない
- `ForLoopWithBreak` は通常のK2Nodeとして実装されており、既存パターンで追加可能
- `ForLoopWithBreak` は `ForLoop` の上位互換（Breakを使わなければ同じ動作）

## 実装内容

### 1. Python側 (`Python/tools/node_tools.py`)

#### add_forloop_with_break_node の追加

```python
@mcp.tool()
def add_forloop_with_break_node(
    ctx: Context,
    blueprint_name: str,
    first_index: int = 0,
    last_index: int = 10,
    node_position: Optional[str] = None,
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    Add a ForLoopWithBreak node for iterating a specified number of times.

    Args:
        blueprint_name: Name of the target Blueprint
        first_index: Starting index (default: 0)
        last_index: Ending index (default: 10)
        node_position: Optional [X, Y] position in the graph
        path: Content browser path where the blueprint is located

    Returns:
        Response containing the node ID and success status

    Pins:
        Input:
            - execute: Execution input
            - FirstIndex: Starting index
            - LastIndex: Ending index
        Output:
            - LoopBody: Fires for each iteration
            - Index: Current loop index
            - Completed: Fires when loop finishes
            - Break: Connect to break out of the loop

    Example:
        add_forloop_with_break_node(
            blueprint_name="BP_Test",
            first_index=0,
            last_index=5,
            node_position=[200, 0]
        )
    """
    try:
        params = {
            "blueprint_name": blueprint_name,
            "first_index": first_index,
            "last_index": last_index,
            "path": path
        }
        if node_position:
            params["node_position"] = node_position

        response = send_command("add_forloop_with_break_node", params)
        return {"success": True, "result": response}

    except Exception as e:
        error_msg = f"Failed to add ForLoopWithBreak node: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

#### add_foreach_loop_node の修正（非推奨化）

既存の `add_foreach_loop_node` を修正して、呼び出し時にエラーメッセージを返すようにする：

```python
@mcp.tool()
def add_foreach_loop_node(
    ctx: Context,
    blueprint_name: str,
    node_position: Optional[str] = None,
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    [DEPRECATED] ForEachLoop is a macro and cannot be added programmatically.
    Use add_forloop_with_break_node instead.

    Args:
        blueprint_name: Name of the target Blueprint
        node_position: Optional [X, Y] position in the graph
        path: Content browser path where the blueprint is located

    Returns:
        Error message with alternative suggestion
    """
    return {
        "success": False,
        "message": "ForEachLoop is a Blueprint macro and cannot be added programmatically. "
                   "Use add_forloop_with_break_node instead for iteration logic.",
        "alternative": "add_forloop_with_break_node"
    }
```

### 2. C++側 (`SpirrowBridgeBlueprintNodeCommands.cpp`)

#### ヘッダー確認

```cpp
#include "K2Node_ForEachElementInEnum.h"  // 不要なら削除
#include "K2Node_MacroInstance.h"  // ForLoopWithBreak用
```

#### HandleAddForLoopWithBreakNode の実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddForLoopWithBreakNode(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    int32 FirstIndex = Params->HasField(TEXT("first_index")) ? 
        static_cast<int32>(Params->GetNumberField(TEXT("first_index"))) : 0;
    int32 LastIndex = Params->HasField(TEXT("last_index")) ? 
        static_cast<int32>(Params->GetNumberField(TEXT("last_index"))) : 10;
    FString Path = Params->HasField(TEXT("path")) ? 
        Params->GetStringField(TEXT("path")) : TEXT("/Game/Blueprints");
    
    // 位置
    int32 PosX = 0, PosY = 0;
    if (Params->HasField(TEXT("node_position")))
    {
        const TArray<TSharedPtr<FJsonValue>>& PosArray = Params->GetArrayField(TEXT("node_position"));
        if (PosArray.Num() >= 2)
        {
            PosX = static_cast<int32>(PosArray[0]->AsNumber());
            PosY = static_cast<int32>(PosArray[1]->AsNumber());
        }
    }
    
    // Blueprint取得
    FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *FullPath);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Blueprint not found: %s"), *FullPath));
    }
    
    // EventGraph取得
    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Event graph not found"));
    }
    
    // ForLoopWithBreakマクロを取得
    // エンジンのマクロライブラリから取得
    UBlueprint* MacroLibrary = LoadObject<UBlueprint>(nullptr, 
        TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
    if (!MacroLibrary)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            TEXT("Failed to load StandardMacros library"));
    }
    
    // ForLoopWithBreakマクロを検索
    UEdGraph* MacroGraph = nullptr;
    for (UEdGraph* Graph : MacroLibrary->MacroGraphs)
    {
        if (Graph && Graph->GetFName() == FName(TEXT("ForLoopWithBreak")))
        {
            MacroGraph = Graph;
            break;
        }
    }
    
    if (!MacroGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            TEXT("Failed to find ForLoopWithBreak macro"));
    }
    
    // UK2Node_MacroInstance を作成
    UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(EventGraph);
    MacroNode->SetMacroGraph(MacroGraph);
    MacroNode->NodePosX = PosX;
    MacroNode->NodePosY = PosY;
    MacroNode->AllocateDefaultPins();
    EventGraph->AddNode(MacroNode, false, false);
    
    // FirstIndex と LastIndex のデフォルト値を設定
    UEdGraphPin* FirstIndexPin = MacroNode->FindPin(TEXT("FirstIndex"));
    if (FirstIndexPin)
    {
        FirstIndexPin->DefaultValue = FString::FromInt(FirstIndex);
    }
    
    UEdGraphPin* LastIndexPin = MacroNode->FindPin(TEXT("LastIndex"));
    if (LastIndexPin)
    {
        LastIndexPin->DefaultValue = FString::FromInt(LastIndex);
    }
    
    // Blueprintをマーク
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // 結果を返す
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetStringField(TEXT("node_id"), MacroNode->NodeGuid.ToString());
    ResultJson->SetNumberField(TEXT("first_index"), FirstIndex);
    ResultJson->SetNumberField(TEXT("last_index"), LastIndex);
    
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(ResultJson);
}
```

#### コマンドハンドラーの登録

```cpp
// SpirrowBridgeBlueprintNodeCommands.cpp のコマンド登録部分
CommandHandlers.Add(TEXT("add_forloop_with_break_node"), 
    FCommandHandler::CreateRaw(this, &FSpirrowBridgeBlueprintNodeCommands::HandleAddForLoopWithBreakNode));
```

#### ヘッダーファイルに関数宣言を追加

```cpp
// SpirrowBridgeBlueprintNodeCommands.h
TSharedPtr<FJsonObject> HandleAddForLoopWithBreakNode(const TSharedPtr<FJsonObject>& Params);
```

### 3. 必要なインクルード

```cpp
#include "K2Node_MacroInstance.h"
```

## テスト手順

### 1. プラグインをリビルド

### 2. ForLoopWithBreak ノードのテスト

```python
# 基本テスト
add_forloop_with_break_node(
    blueprint_name="BP_LoopTest",
    first_index=0,
    last_index=10,
    node_position=[200, 0],
    path="/Game/Test"
)
```

### 3. 非推奨の ForEachLoop を呼び出してエラー確認

```python
# エラーが返ることを確認
add_foreach_loop_node(
    blueprint_name="BP_LoopTest",
    path="/Game/Test"
)
# 期待結果: success=False, alternative="add_forloop_with_break_node"
```

### 4. 接続テスト

```python
# BeginPlayイベントからForLoopWithBreakを実行
begin_play = add_blueprint_event_node(
    blueprint_name="BP_LoopTest",
    event_name="ReceiveBeginPlay",
    path="/Game/Test"
)

forloop = add_forloop_with_break_node(
    blueprint_name="BP_LoopTest",
    first_index=0,
    last_index=5,
    node_position=[300, 0],
    path="/Game/Test"
)

print_node = add_print_string_node(
    blueprint_name="BP_LoopTest",
    message="Loop iteration",
    node_position=[500, 0],
    path="/Game/Test"
)

# 接続
connect_blueprint_nodes(
    blueprint_name="BP_LoopTest",
    source_node_id=begin_play["result"]["node_id"],
    source_pin="then",
    target_node_id=forloop["result"]["node_id"],
    target_pin="execute",
    path="/Game/Test"
)

connect_blueprint_nodes(
    blueprint_name="BP_LoopTest",
    source_node_id=forloop["result"]["node_id"],
    source_pin="LoopBody",
    target_node_id=print_node["result"]["node_id"],
    target_pin="execute",
    path="/Game/Test"
)
```

## ピン名一覧

ForLoopWithBreakノードのピン名：

| ピン | 方向 | 型 | 説明 |
|------|------|-----|------|
| execute | Input | Exec | 実行入力 |
| FirstIndex | Input | Int | 開始インデックス |
| LastIndex | Input | Int | 終了インデックス |
| LoopBody | Output | Exec | 各イテレーションで実行 |
| Index | Output | Int | 現在のインデックス |
| Completed | Output | Exec | ループ完了時に実行 |
| Break | Input | Exec | ループを中断 |

## 注意事項

1. **マクロパスの確認**: `/Engine/EditorBlueprintResources/StandardMacros` のパスはUEバージョンによって異なる可能性がある。動作しない場合はパスを確認すること。

2. **ピン名の確認**: 実際のピン名はUEバージョンによって異なる可能性がある。`get_blueprint_graph` で確認すること。

## 優先度

中

## 関連ファイル

- `Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeBlueprintNodeCommands.cpp` - 修正対象
- `Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeBlueprintNodeCommands.h` - 関数宣言追加
- `Python/tools/node_tools.py` - 新規ツール追加、既存ツール非推奨化
