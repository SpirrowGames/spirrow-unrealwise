# Blueprint Graph読み取り機能強化 実装プロンプト

## 概要

`get_blueprint_graph` の出力を拡充し、既存Blueprintの解析・修正に必要な詳細情報を取得できるようにする。

## 現状の問題点

1. ピンのデフォルト値が取得できない
2. ノードタイプ分類が不十分（Cast、Macro、CustomEvent等が「Other」扱い）
3. ピンの詳細情報不足（SubCategory、PinSubCategoryObject、配列型かどうか等）
4. 関数グラフ（FunctionGraphs）が取得対象外
5. ノード固有のプロパティ（Target、FunctionReference等）が取得できない

## 仕様

### 1. `get_blueprint_graph` 拡張

#### 新しい出力フォーマット

```json
{
  "status": "success",
  "result": {
    "blueprint_name": "BP_Example",
    "parent_class": "Character",
    "graphs": [
      {
        "name": "EventGraph",
        "graph_type": "Ubergraph",
        "nodes": [...]
      },
      {
        "name": "CustomFunction",
        "graph_type": "Function",
        "nodes": [...]
      }
    ],
    "nodes": [...],  // 後方互換：EventGraphのノード
    "connections": [...],
    "variables": [...],
    "components": [...],
    "event_dispatchers": [...]
  }
}
```

#### ノード詳細情報の拡張

```json
{
  "id": "GUID",
  "class": "K2Node_CallFunction",
  "title": "Print String",
  "type": "Function",
  "subtype": "Library",  // 新規: Library/Member/Pure/Latent等
  "pos_x": 100,
  "pos_y": 200,
  "comment": "デバッグ用",  // 新規: ノードコメント
  
  // タイプ別の追加情報
  "function_reference": {  // Function/Eventノードの場合
    "member_name": "PrintString",
    "member_parent": "KismetSystemLibrary",
    "is_self_context": false
  },
  "variable_reference": {  // Variable Get/Setの場合
    "variable_name": "Health",
    "variable_guid": "...",
    "is_local": false
  },
  "cast_info": {  // Castノードの場合
    "target_class": "BP_Enemy"
  },
  
  "pins": [
    {
      "id": "PinGUID",
      "name": "InString",
      "friendly_name": "In String",  // 新規
      "direction": "Input",
      "type": {
        "category": "string",
        "sub_category": "",
        "is_array": false,
        "is_reference": false,
        "is_const": false,
        "object_type": null  // オブジェクト参照の場合はクラス名
      },
      "default_value": "Hello",  // 新規
      "is_connected": true,  // 新規
      "connections": [  // 新規: 接続先詳細
        {
          "node_id": "...",
          "pin_name": "ReturnValue"
        }
      ],
      "is_hidden": false,
      "is_orphan": false
    }
  ]
}
```

### 2. 新規ツール `get_node_details`

単一ノードの詳細情報を取得する軽量ツール。

```python
def get_node_details(
    blueprint_name: str,
    node_id: str,
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    特定ノードの詳細情報を取得。
    
    Returns:
        - ノードの全プロパティ
        - 全ピンの詳細（デフォルト値、接続先含む）
        - ノードメタデータ
    """
```

### 3. 新規ツール `get_pin_connections`

特定ピンの接続情報を取得。

```python
def get_pin_connections(
    blueprint_name: str,
    node_id: str,
    pin_name: str,
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    特定ピンの接続先を取得。
    
    Returns:
        - connected_pins: 接続先ピンのリスト
        - each pin: node_id, pin_name, node_title
    """
```

## 実装内容

### 1. C++側 (`SpirrowBridgeBlueprintCommands.cpp`)

#### HandleGetBlueprintGraph 修正

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleGetBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    // ... 既存のBlueprint読み込み処理 ...

    // === グラフ一覧の取得 ===
    TArray<TSharedPtr<FJsonValue>> GraphsArray;
    
    // Ubergraph (EventGraph)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        TSharedPtr<FJsonObject> GraphObj = BuildGraphObject(Graph, TEXT("Ubergraph"));
        GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
    }
    
    // Function Graphs
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        TSharedPtr<FJsonObject> GraphObj = BuildGraphObject(Graph, TEXT("Function"));
        GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
    }
    
    ResultData->SetArrayField(TEXT("graphs"), GraphsArray);
    
    // ... 後方互換のためnodes/connectionsも維持 ...
}

// ノード詳細情報を構築するヘルパー関数
TSharedPtr<FJsonObject> BuildDetailedNodeObject(UEdGraphNode* Node)
{
    TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
    
    // 基本情報
    NodeObj->SetStringField(TEXT("id"), Node->NodeGuid.ToString());
    NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
    NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
    NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
    NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);
    
    // ノードコメント
    if (!Node->NodeComment.IsEmpty())
    {
        NodeObj->SetStringField(TEXT("comment"), Node->NodeComment);
    }
    
    // ノードタイプ判定（拡張版）
    FString NodeType, NodeSubtype;
    DetermineNodeType(Node, NodeType, NodeSubtype);
    NodeObj->SetStringField(TEXT("type"), NodeType);
    if (!NodeSubtype.IsEmpty())
    {
        NodeObj->SetStringField(TEXT("subtype"), NodeSubtype);
    }
    
    // タイプ別の追加情報
    AddTypeSpecificInfo(Node, NodeObj);
    
    // ピン詳細情報
    TArray<TSharedPtr<FJsonValue>> PinsArray;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin || Pin->bHidden) continue;
        PinsArray.Add(MakeShared<FJsonValueObject>(BuildDetailedPinObject(Pin)));
    }
    NodeObj->SetArrayField(TEXT("pins"), PinsArray);
    
    return NodeObj;
}

// ピン詳細情報を構築
TSharedPtr<FJsonObject> BuildDetailedPinObject(UEdGraphPin* Pin)
{
    TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
    
    PinObj->SetStringField(TEXT("id"), Pin->PinId.ToString());
    PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
    PinObj->SetStringField(TEXT("friendly_name"), Pin->PinFriendlyName.IsEmpty() 
        ? Pin->PinName.ToString() 
        : Pin->PinFriendlyName.ToString());
    PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
    
    // ピンタイプ詳細
    TSharedPtr<FJsonObject> TypeObj = MakeShared<FJsonObject>();
    TypeObj->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
    TypeObj->SetStringField(TEXT("sub_category"), Pin->PinType.PinSubCategory.ToString());
    TypeObj->SetBoolField(TEXT("is_array"), Pin->PinType.IsArray());
    TypeObj->SetBoolField(TEXT("is_reference"), Pin->PinType.bIsReference);
    TypeObj->SetBoolField(TEXT("is_const"), Pin->PinType.bIsConst);
    
    if (Pin->PinType.PinSubCategoryObject.IsValid())
    {
        TypeObj->SetStringField(TEXT("object_type"), Pin->PinType.PinSubCategoryObject->GetName());
    }
    PinObj->SetObjectField(TEXT("type"), TypeObj);
    
    // デフォルト値
    if (!Pin->DefaultValue.IsEmpty())
    {
        PinObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
    }
    else if (!Pin->DefaultTextValue.IsEmpty())
    {
        PinObj->SetStringField(TEXT("default_value"), Pin->DefaultTextValue.ToString());
    }
    else if (Pin->DefaultObject)
    {
        PinObj->SetStringField(TEXT("default_value"), Pin->DefaultObject->GetPathName());
    }
    
    // 接続状態
    PinObj->SetBoolField(TEXT("is_connected"), Pin->LinkedTo.Num() > 0);
    PinObj->SetBoolField(TEXT("is_hidden"), Pin->bHidden);
    PinObj->SetBoolField(TEXT("is_orphan"), Pin->bOrphanedPin);
    
    // 接続先詳細
    if (Pin->LinkedTo.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
        {
            if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;
            
            TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
            ConnObj->SetStringField(TEXT("node_id"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
            ConnObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
            ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
        }
        PinObj->SetArrayField(TEXT("connections"), ConnectionsArray);
    }
    
    return PinObj;
}

// ノードタイプ判定
void DetermineNodeType(UEdGraphNode* Node, FString& OutType, FString& OutSubtype)
{
    OutSubtype = TEXT("");
    
    if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
    {
        OutType = TEXT("Event");
        // CustomEventかどうか
        if (EventNode->CustomFunctionName != NAME_None)
        {
            OutSubtype = TEXT("Custom");
        }
    }
    else if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
    {
        OutType = TEXT("Function");
        
        // Pure/Latent判定
        if (FuncNode->bIsPureFunc)
        {
            OutSubtype = TEXT("Pure");
        }
        else if (FuncNode->IsLatentFunction())
        {
            OutSubtype = TEXT("Latent");
        }
        
        // Self contextかLibrary呼び出しか
        if (FuncNode->FunctionReference.IsSelfContext())
        {
            if (OutSubtype.IsEmpty()) OutSubtype = TEXT("Member");
        }
        else
        {
            if (OutSubtype.IsEmpty()) OutSubtype = TEXT("Library");
        }
    }
    else if (Cast<UK2Node_VariableGet>(Node))
    {
        OutType = TEXT("VariableGet");
    }
    else if (Cast<UK2Node_VariableSet>(Node))
    {
        OutType = TEXT("VariableSet");
    }
    else if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
    {
        OutType = TEXT("Cast");
    }
    else if (Cast<UK2Node_IfThenElse>(Node))
    {
        OutType = TEXT("Branch");
    }
    else if (Cast<UK2Node_MacroInstance>(Node))
    {
        OutType = TEXT("Macro");
    }
    else if (Cast<UK2Node_InputAction>(Node))
    {
        OutType = TEXT("InputAction");
    }
    else if (Cast<UK2Node_Self>(Node))
    {
        OutType = TEXT("Self");
    }
    else if (Cast<UK2Node_Literal>(Node))
    {
        OutType = TEXT("Literal");
    }
    else if (Cast<UK2Node_MakeArray>(Node))
    {
        OutType = TEXT("MakeArray");
    }
    else if (Cast<UK2Node_GetArrayItem>(Node))
    {
        OutType = TEXT("GetArrayItem");
    }
    else
    {
        OutType = TEXT("Other");
        OutSubtype = Node->GetClass()->GetName();
    }
}
```

### 2. 新規コマンド追加

#### HandleGetNodeDetails

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleGetNodeDetails(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString NodeId = Params->GetStringField(TEXT("node_id"));
    FString Path = Params->GetStringField(TEXT("path"));
    
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprintByName(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Blueprint not found"));
    }
    
    FGuid TargetGuid;
    FGuid::Parse(NodeId, TargetGuid);
    
    // 全グラフからノードを検索
    UEdGraphNode* FoundNode = nullptr;
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node && Node->NodeGuid == TargetGuid)
            {
                FoundNode = Node;
                break;
            }
        }
        if (FoundNode) break;
    }
    
    // FunctionGraphsも検索
    if (!FoundNode)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (Node && Node->NodeGuid == TargetGuid)
                {
                    FoundNode = Node;
                    break;
                }
            }
            if (FoundNode) break;
        }
    }
    
    if (!FoundNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Node not found"));
    }
    
    TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
    ResultJson->SetStringField(TEXT("status"), TEXT("success"));
    ResultJson->SetObjectField(TEXT("node"), BuildDetailedNodeObject(FoundNode));
    
    return ResultJson;
}
```

#### HandleGetPinConnections

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleGetPinConnections(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString NodeId = Params->GetStringField(TEXT("node_id"));
    FString PinName = Params->GetStringField(TEXT("pin_name"));
    FString Path = Params->GetStringField(TEXT("path"));
    
    // ノード検索（HandleGetNodeDetailsと同様）...
    
    // ピン検索
    UEdGraphPin* TargetPin = nullptr;
    for (UEdGraphPin* Pin : FoundNode->Pins)
    {
        if (Pin && Pin->PinName.ToString() == PinName)
        {
            TargetPin = Pin;
            break;
        }
    }
    
    if (!TargetPin)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Pin not found"));
    }
    
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
    for (UEdGraphPin* LinkedPin : TargetPin->LinkedTo)
    {
        if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;
        
        TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
        ConnObj->SetStringField(TEXT("node_id"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
        ConnObj->SetStringField(TEXT("node_title"), LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
        ConnObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
        ConnObj->SetStringField(TEXT("pin_direction"), LinkedPin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
        ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
    }
    
    TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
    ResultJson->SetStringField(TEXT("status"), TEXT("success"));
    ResultJson->SetNumberField(TEXT("connection_count"), ConnectionsArray.Num());
    ResultJson->SetArrayField(TEXT("connections"), ConnectionsArray);
    
    return ResultJson;
}
```

### 3. Python側 (`blueprint_tools.py`)

#### get_node_details 追加

```python
@mcp.tool()
def get_node_details(
    ctx: Context,
    blueprint_name: str,
    node_id: str,
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    特定ノードの詳細情報を取得。
    
    Args:
        blueprint_name: Blueprintの名前
        node_id: 対象ノードのGUID
        path: Blueprintのパス
    
    Returns:
        ノードの詳細情報（全ピン、デフォルト値、接続先含む）
    """
    from unreal_mcp_server import get_unreal_connection
    
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        params = {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "path": path
        }
        
        return unreal.send_command("get_node_details", params)
    
    except Exception as e:
        return {"success": False, "message": str(e)}
```

#### get_pin_connections 追加

```python
@mcp.tool()
def get_pin_connections(
    ctx: Context,
    blueprint_name: str,
    node_id: str,
    pin_name: str,
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    特定ピンの接続先を取得。
    
    Args:
        blueprint_name: Blueprintの名前
        node_id: 対象ノードのGUID
        pin_name: 対象ピンの名前
        path: Blueprintのパス
    
    Returns:
        接続先ピンのリスト
    """
    from unreal_mcp_server import get_unreal_connection
    
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        params = {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
            "pin_name": pin_name,
            "path": path
        }
        
        return unreal.send_command("get_pin_connections", params)
    
    except Exception as e:
        return {"success": False, "message": str(e)}
```

## 必要なヘッダーインクルード

```cpp
#include "K2Node_DynamicCast.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_Literal.h"
#include "K2Node_MakeArray.h"
#include "K2Node_GetArrayItem.h"
```

## チェックリスト

### C++側
- [ ] `SpirrowBridgeBlueprintCommands.h` - HandleGetNodeDetails, HandleGetPinConnections 宣言追加
- [ ] `SpirrowBridgeBlueprintCommands.cpp` - 実装追加
- [ ] `SpirrowBridgeBlueprintCommands.cpp` - HandleCommand にルーティング追加
- [ ] `SpirrowBridge.cpp` - ExecuteCommand にルーティング追加
- [ ] 必要なヘッダーインクルード追加

### Python側
- [ ] `blueprint_tools.py` - get_node_details, get_pin_connections 追加

### ドキュメント
- [ ] `FEATURE_STATUS.md` 更新
- [ ] `AGENTS.md` 更新（ピン詳細情報の説明追加）

## テスト手順

1. 既存のBlueprintに対して `get_blueprint_graph` を実行
2. 出力に `graphs` 配列があることを確認
3. 各ノードに `subtype` と詳細なピン情報があることを確認
4. `get_node_details` で特定ノードの詳細が取得できることを確認
5. `get_pin_connections` で接続情報が取得できることを確認

## 補足事項

- 後方互換性を維持するため、既存の `nodes`, `connections` フィールドは残す
- 大きなBlueprintではパフォーマンスに注意（タイムアウトリスク）
- FunctionGraphsは初回から含めるが、MacroGraphs/EventGraphsの対応は後回し
