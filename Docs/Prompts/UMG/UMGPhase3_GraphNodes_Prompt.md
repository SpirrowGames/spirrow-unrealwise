# UMG Phase 3: Widget Graph ノード操作

## 概要

Phase 2（変数・関数操作）完了後、Widget BP の Graph 内でノードを追加・接続できるようにする。
これにより、関数やイベント内のロジックを MCP 経由で構築可能になる。

## 前提

- Phase 1: Designer 操作完全化 ✅
- Phase 2: 変数・関数操作 ✅
- 本 Phase: Graph ノード操作

## 実装するツール

| # | ツール名 | 機能 | 優先度 |
|---|----------|------|--------|
| 1 | `add_widget_function_node` | 関数呼び出しノード追加 | 高 |
| 2 | `add_widget_variable_get_node` | 変数 Get ノード追加 | 高 |
| 3 | `add_widget_variable_set_node` | 変数 Set ノード追加 | 高 |
| 4 | `add_widget_self_reference_node` | Self 参照ノード追加 | 中 |
| 5 | `add_widget_element_reference_node` | Widget 要素参照ノード追加 | 高 |
| 6 | `connect_widget_nodes` | ノード接続 | 高 |
| 7 | `find_widget_nodes` | ノード検索 | 中 |

---

## 1. add_widget_function_node

### Python 側

```python
@mcp.tool()
def add_widget_function_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,              # "EventGraph" or カスタム関数名
    target: str,                  # "self", コンポーネント名, またはライブラリ名
    function_name: str,           # 関数名
    position: List[float] = [0, 0],
    params: Dict[str, Any] = None,  # 関数パラメータのプリセット値
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a function call node to a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph ("EventGraph" or custom function name)
        target: Target for the function call
        function_name: Name of the function to call
        position: [X, Y] position in the graph
        params: Optional parameter values to preset
        path: Content browser path to the widget

    Returns:
        Dict containing node_id and success status

    Example:
        # SetText on a TextBlock
        add_widget_function_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="UpdateTrapSelection",
            target="TrapNameText",
            function_name="SetText",
            position=[300, 0],
            path="/Game/TrapxTrap/UI"
        )

        # SetVisibility on self
        add_widget_function_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="EventGraph",
            target="self",
            function_name="SetVisibility",
            position=[300, 0],
            path="/Game/TrapxTrap/UI"
        )

        # PrintString (Kismet library)
        add_widget_function_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="EventGraph",
            target="KismetSystemLibrary",
            function_name="PrintString",
            position=[300, 0],
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装ポイント

```cpp
TSharedPtr<FJsonObject> HandleAddWidgetFunctionNode(const TSharedPtr<FJsonObject>& Params)
{
    // 1. Widget Blueprint をロード
    // 2. graph_name で対象グラフを取得
    //    - "EventGraph" → UbergraphPages[0]
    //    - カスタム関数名 → FunctionGraphs から検索
    // 3. UK2Node_CallFunction ノードを作成
    // 4. Target と Function を設定
    // 5. 位置を設定
    // 6. ノード ID を返す
}
```

---

## 2. add_widget_variable_get_node

### Python 側

```python
@mcp.tool()
def add_widget_variable_get_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    variable_name: str,
    position: List[float] = [0, 0],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a variable Get node to a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        variable_name: Name of the variable to get
        position: [X, Y] position in the graph
        path: Content browser path to the widget

    Returns:
        Dict containing node_id and success status

    Example:
        add_widget_variable_get_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="UpdateTrapSelection",
            variable_name="CurrentTrapName",
            position=[0, 100],
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装ポイント

```cpp
// UK2Node_VariableGet を作成
UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(Graph);
GetNode->VariableReference.SetSelfMember(FName(*VariableName));
```

---

## 3. add_widget_variable_set_node

### Python 側

```python
@mcp.tool()
def add_widget_variable_set_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    variable_name: str,
    position: List[float] = [0, 0],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a variable Set node to a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        variable_name: Name of the variable to set
        position: [X, Y] position in the graph
        path: Content browser path to the widget

    Returns:
        Dict containing node_id and success status
    """
```

### C++ 実装ポイント

```cpp
// UK2Node_VariableSet を作成
UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(Graph);
SetNode->VariableReference.SetSelfMember(FName(*VariableName));
```

---

## 4. add_widget_self_reference_node

### Python 側

```python
@mcp.tool()
def add_widget_self_reference_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    position: List[float] = [0, 0],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a Self reference node to a Widget Blueprint's graph.

    Returns:
        Dict containing node_id and success status
    """
```

### C++ 実装ポイント

```cpp
// UK2Node_Self を作成
UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
```

---

## 5. add_widget_element_reference_node

### Python 側

```python
@mcp.tool()
def add_widget_element_reference_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    element_name: str,           # Widget 要素名（TrapNameText 等）
    position: List[float] = [0, 0],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a widget element reference node to the graph.
    This is equivalent to dragging a widget from the hierarchy to the graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        element_name: Name of the widget element to reference
        position: [X, Y] position in the graph
        path: Content browser path to the widget

    Returns:
        Dict containing node_id and success status

    Example:
        # Get reference to TrapNameText for calling SetText
        add_widget_element_reference_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="UpdateTrapSelection",
            element_name="TrapNameText",
            position=[0, 0],
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装ポイント

Widget 要素への参照は `UK2Node_VariableGet` でバインド変数を取得する形になる。
Widget BP では、デザイナーで追加した要素は内部的に変数として扱われる。

```cpp
// Widget 要素は内部変数として存在する
// IsVariable = true の Widget を探して参照ノードを作成
UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(Graph);
GetNode->VariableReference.SetSelfMember(FName(*ElementName));
```

---

## 6. connect_widget_nodes

### Python 側

```python
@mcp.tool()
def connect_widget_nodes(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    source_node_id: str,
    source_pin: str,
    target_node_id: str,
    target_pin: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Connect two nodes in a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        source_node_id: ID of the source node
        source_pin: Name of the output pin on source
        target_node_id: ID of the target node
        target_pin: Name of the input pin on target
        path: Content browser path to the widget

    Returns:
        Dict containing success status

    Common pin names:
        - Execution: "then" (output), "execute" (input)
        - Return value: "ReturnValue"
        - Parameters: パラメータ名そのまま

    Example:
        connect_widget_nodes(
            widget_name="WBP_TT_TrapSelector",
            graph_name="UpdateTrapSelection",
            source_node_id="K2Node_FunctionEntry_0",
            source_pin="then",
            target_node_id="K2Node_CallFunction_0",
            target_pin="execute",
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装ポイント

既存の `connect_blueprint_nodes` 実装を参考に：

```cpp
// ピンを検索して接続
UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName);
UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName);
if (SourcePin && TargetPin)
{
    SourcePin->MakeLinkTo(TargetPin);
}
```

---

## 7. find_widget_nodes

### Python 側

```python
@mcp.tool()
def find_widget_nodes(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    node_type: str = None,        # "Event", "Function", "Variable", etc.
    node_name: str = None,        # 特定のノード名
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Find nodes in a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        node_type: Optional filter by node type
        node_name: Optional filter by node name
        path: Content browser path to the widget

    Returns:
        Dict containing list of matching nodes with their IDs and types

    Example:
        # Find all nodes in UpdateTrapSelection function
        find_widget_nodes(
            widget_name="WBP_TT_TrapSelector",
            graph_name="UpdateTrapSelection",
            path="/Game/TrapxTrap/UI"
        )

        # Find function entry node
        find_widget_nodes(
            widget_name="WBP_TT_TrapSelector",
            graph_name="UpdateTrapSelection",
            node_type="FunctionEntry",
            path="/Game/TrapxTrap/UI"
        )
    """
```

---

## 使用例: UpdateTrapSelection 関数の構築

```python
# 前提: Phase 2 で関数を作成済み
# add_widget_function("WBP_TT_TrapSelector", "UpdateTrapSelection", 
#                     inputs=[{"name": "TrapName", "type": "String"}, ...])

# 1. 関数エントリを探す
entry = find_widget_nodes(
    widget_name="WBP_TT_TrapSelector",
    graph_name="UpdateTrapSelection",
    node_type="FunctionEntry"
)
entry_node_id = entry["nodes"][0]["id"]

# 2. TrapNameText への参照を取得
text_ref = add_widget_element_reference_node(
    widget_name="WBP_TT_TrapSelector",
    graph_name="UpdateTrapSelection",
    element_name="TrapNameText",
    position=[0, 0]
)

# 3. SetText ノードを追加
set_text = add_widget_function_node(
    widget_name="WBP_TT_TrapSelector",
    graph_name="UpdateTrapSelection",
    target="TrapNameText",
    function_name="SetText",
    position=[300, 0]
)

# 4. 接続: Entry → SetText (実行ピン)
connect_widget_nodes(
    widget_name="WBP_TT_TrapSelector",
    graph_name="UpdateTrapSelection",
    source_node_id=entry_node_id,
    source_pin="then",
    target_node_id=set_text["node_id"],
    target_pin="execute"
)

# 5. 接続: TrapNameText参照 → SetText (Target ピン)
connect_widget_nodes(
    widget_name="WBP_TT_TrapSelector",
    graph_name="UpdateTrapSelection",
    source_node_id=text_ref["node_id"],
    source_pin="TrapNameText",
    target_node_id=set_text["node_id"],
    target_pin="self"
)

# 6. 接続: Entry の TrapName パラメータ → SetText の InText
connect_widget_nodes(
    widget_name="WBP_TT_TrapSelector",
    graph_name="UpdateTrapSelection",
    source_node_id=entry_node_id,
    source_pin="TrapName",
    target_node_id=set_text["node_id"],
    target_pin="InText"
)
```

---

## 実装順序

```
1. find_widget_nodes       ← デバッグに必須
2. add_widget_element_reference_node  ← Widget 要素操作の基本
3. add_widget_function_node
4. add_widget_variable_get_node
5. add_widget_variable_set_node
6. connect_widget_nodes
7. add_widget_self_reference_node
```

---

## 参考: 既存の Blueprint ノード操作

`SpirrowBridgeBlueprintCommands.cpp` に類似の実装がある：

- `HandleAddBlueprintFunctionNode`
- `HandleConnectBlueprintNodes`
- `HandleFindBlueprintNodes`
- `HandleAddBlueprintGetSelfComponentReference`

これらを参考に Widget BP 向けに調整する。

---

## Phase 3 完了条件

以下ができるようになること：

1. Widget BP のカスタム関数内にノードを追加
2. Widget 要素（TextBlock 等）への参照を取得
3. SetText, SetVisibility 等の関数呼び出しノードを追加
4. 変数の Get/Set ノードを追加
5. ノード同士を接続してロジックを構築

---

## 修正日

2025-12-22
