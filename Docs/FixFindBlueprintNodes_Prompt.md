# 機能名: find_blueprint_nodes 修正

**ステータス: ✅ 実装完了（2026-01-01）**

## 概要

`find_blueprint_nodes`コマンドの改善。現在は`node_type`が必須パラメータだが、オプショナルにして全ノード取得や柔軟なフィルタリングを可能にする。

## 現状の問題

1. `node_type`パラメータが必須（Python側ではオプショナル定義）
2. `Event`タイプのみ対応、他タイプ未実装
3. ノード情報が不十分（GUIDのみ返却）

## 修正仕様

### パラメータ

| パラメータ | 型 | 必須 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| blueprint_name | string | ○ | - | Blueprint名 |
| path | string | × | /Game/Blueprints | Blueprintのパス |
| node_type | string | × | null | フィルタ: Event, Function, Variable, Branch, Sequence, Macro, All |
| event_type | string | × | null | Eventノードの場合のイベント名フィルタ |
| function_name | string | × | null | Functionノードの場合の関数名フィルタ |
| variable_name | string | × | null | Variableノードの場合の変数名フィルタ |

### 返却データ

```json
{
  "success": true,
  "nodes": [
    {
      "node_id": "GUID文字列",
      "node_type": "Event|Function|Variable|Branch|Sequence|Macro|Other",
      "node_class": "UK2Node_Event",
      "name": "ReceiveBeginPlay",
      "position": { "x": 0, "y": 0 },
      "pins": [
        {
          "name": "then",
          "direction": "Output",
          "type": "exec",
          "connected": true
        }
      ]
    }
  ],
  "count": 1
}
```

## 実装内容

### C++側 (`SpirrowBridgeBlueprintNodeCommands.cpp`)

`HandleFindBlueprintNodes`関数を以下のように修正：

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // 必須パラメータ
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // オプショナルパラメータ
    FString NodeType;
    Params->TryGetStringField(TEXT("node_type"), NodeType);
    
    FString EventType;
    Params->TryGetStringField(TEXT("event_type"), EventType);
    
    FString FunctionName;
    Params->TryGetStringField(TEXT("function_name"), FunctionName);
    
    FString VariableName;
    Params->TryGetStringField(TEXT("variable_name"), VariableName);

    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        Path = TEXT("/Game/Blueprints");
    }

    // Blueprintを検索
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprint(BlueprintName, Path);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // EventGraphを取得
    UEdGraph* EventGraph = FSpirrowBridgeCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // ノード情報を収集
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        if (!Node) continue;
        
        // ノードタイプを判定
        FString DetectedType = TEXT("Other");
        FString NodeName;
        
        if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
        {
            DetectedType = TEXT("Event");
            NodeName = EventNode->EventReference.GetMemberName().ToString();
            
            // EventTypeフィルタ
            if (!EventType.IsEmpty() && NodeName != EventType)
            {
                continue;
            }
        }
        else if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
        {
            DetectedType = TEXT("Function");
            NodeName = FuncNode->FunctionReference.GetMemberName().ToString();
            
            // FunctionNameフィルタ
            if (!FunctionName.IsEmpty() && NodeName != FunctionName)
            {
                continue;
            }
        }
        else if (UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node))
        {
            DetectedType = TEXT("VariableGet");
            NodeName = VarGetNode->VariableReference.GetMemberName().ToString();
            
            if (!VariableName.IsEmpty() && NodeName != VariableName)
            {
                continue;
            }
        }
        else if (UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node))
        {
            DetectedType = TEXT("VariableSet");
            NodeName = VarSetNode->VariableReference.GetMemberName().ToString();
            
            if (!VariableName.IsEmpty() && NodeName != VariableName)
            {
                continue;
            }
        }
        else if (UK2Node_IfThenElse* BranchNode = Cast<UK2Node_IfThenElse>(Node))
        {
            DetectedType = TEXT("Branch");
            NodeName = TEXT("Branch");
        }
        else if (UK2Node_ExecutionSequence* SeqNode = Cast<UK2Node_ExecutionSequence>(Node))
        {
            DetectedType = TEXT("Sequence");
            NodeName = TEXT("Sequence");
        }
        else if (UK2Node_MacroInstance* MacroNode = Cast<UK2Node_MacroInstance>(Node))
        {
            DetectedType = TEXT("Macro");
            if (MacroNode->GetMacroGraph())
            {
                NodeName = MacroNode->GetMacroGraph()->GetName();
            }
        }
        else if (UK2Node_InputAction* InputNode = Cast<UK2Node_InputAction>(Node))
        {
            DetectedType = TEXT("InputAction");
            NodeName = InputNode->InputActionName.ToString();
        }
        else if (UK2Node_Self* SelfNode = Cast<UK2Node_Self>(Node))
        {
            DetectedType = TEXT("Self");
            NodeName = TEXT("Self");
        }
        
        // NodeTypeフィルタ（空の場合は全て通過）
        if (!NodeType.IsEmpty() && NodeType != TEXT("All"))
        {
            // Variable は VariableGet/VariableSet 両方にマッチ
            if (NodeType == TEXT("Variable"))
            {
                if (DetectedType != TEXT("VariableGet") && DetectedType != TEXT("VariableSet"))
                {
                    continue;
                }
            }
            else if (DetectedType != NodeType)
            {
                continue;
            }
        }
        
        // ノード情報をJSON化
        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
        NodeObj->SetStringField(TEXT("node_type"), DetectedType);
        NodeObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
        NodeObj->SetStringField(TEXT("name"), NodeName);
        
        // 位置情報
        TSharedPtr<FJsonObject> PosObj = MakeShared<FJsonObject>();
        PosObj->SetNumberField(TEXT("x"), Node->NodePosX);
        PosObj->SetNumberField(TEXT("y"), Node->NodePosY);
        NodeObj->SetObjectField(TEXT("position"), PosObj);
        
        // ピン情報
        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin) continue;
            
            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
            PinObj->SetBoolField(TEXT("connected"), Pin->LinkedTo.Num() > 0);
            
            PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
        }
        NodeObj->SetArrayField(TEXT("pins"), PinsArray);
        
        NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
    ResultObj->SetNumberField(TEXT("count"), NodesArray.Num());
    
    return ResultObj;
}
```

### 必要なinclude追加

`SpirrowBridgeBlueprintNodeCommands.cpp`の先頭に以下が既にあることを確認：

```cpp
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_MacroInstance.h"
```

### Python側（変更不要）

`node_tools.py`の`find_blueprint_nodes`は既にオプショナルパラメータとして定義済み。変更不要。

## テスト手順

1. UEエディタを閉じる
2. Visual Studioでリビルド
3. UEエディタを起動
4. テスト用Blueprint作成（BeginPlay + PrintString + 変数）
5. MCPツールでテスト：

```python
# 全ノード取得
find_blueprint_nodes(blueprint_name="BP_Test", path="/Game/Blueprints")

# Eventのみ
find_blueprint_nodes(blueprint_name="BP_Test", node_type="Event")

# 特定のイベント
find_blueprint_nodes(blueprint_name="BP_Test", node_type="Event", event_type="ReceiveBeginPlay")

# 関数ノード
find_blueprint_nodes(blueprint_name="BP_Test", node_type="Function")

# 変数ノード
find_blueprint_nodes(blueprint_name="BP_Test", node_type="Variable")
```

## 補足事項

- 既存の呼び出しとの後方互換性を維持（node_typeがEventで、event_typeが指定されている場合の動作は変わらない）
- ピン情報を含めることで、ノード接続の確認や自動接続の判断材料になる
- `node_type`フィルタに`All`を指定した場合と、未指定の場合は同じ動作（全ノード返却）
