# 機能名: Math/Comparison ノード実装修正

## 概要

`add_math_node` と `add_comparison_node` が "Failed to find math function" エラーで動作しない問題を修正する。

## 問題の原因

現在の実装では `UKismetMathLibrary` の関数を `UK2Node_CallFunction` で呼び出そうとしているが、
関数名の生成方法が間違っている。

### 現在の実装（動作しない）

```cpp
// SpirrowBridgeBlueprintNodeCommands.cpp
// 関数名 "Add_FloatFloat" を文字列連結で生成しようとするが失敗
FName FunctionName = FName(*FString::Printf(TEXT("%s_%s%s"), *Operation, *ValueType, *ValueType));
UFunction* Function = UKismetMathLibrary::StaticClass()->FindFunctionByName(FunctionName);
```

**問題点**: 関数名の生成が不正確で、実際のUKismetMathLibraryの関数名と一致しない。

## 推奨実装方針

**UK2Node_CallFunction を使用する方式を採用する。**

理由：
1. Math/Comparison両方で同じパターンが使える
2. 実装がシンプルで保守しやすい
3. 機能的に問題ない（見た目がコンパクトでないだけ）
4. `GET_FUNCTION_NAME_CHECKED` マクロでコンパイル時に関数名を検証できる

## 実装内容

### 1. C++側 (`SpirrowBridgeBlueprintNodeCommands.cpp`)

#### 必要なヘッダー追加

```cpp
#include "Kismet/KismetMathLibrary.h"
#include "K2Node_CallFunction.h"
```

#### HandleAddMathNode の修正

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddMathNode(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString Operation = Params->GetStringField(TEXT("operation"));
    FString ValueType = Params->HasField(TEXT("value_type")) ? 
        Params->GetStringField(TEXT("value_type")) : TEXT("Float");
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
    
    // 演算子に対応する関数名を決定
    FName FunctionName = NAME_None;
    if (ValueType == TEXT("Float"))
    {
        if (Operation == TEXT("Add")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_FloatFloat);
        else if (Operation == TEXT("Subtract")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_FloatFloat);
        else if (Operation == TEXT("Multiply")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_FloatFloat);
        else if (Operation == TEXT("Divide")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_FloatFloat);
    }
    else if (ValueType == TEXT("Int"))
    {
        if (Operation == TEXT("Add")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt);
        else if (Operation == TEXT("Subtract")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt);
        else if (Operation == TEXT("Multiply")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_IntInt);
        else if (Operation == TEXT("Divide")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_IntInt);
    }
    
    if (FunctionName == NAME_None)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported operation/type: %s/%s"), *Operation, *ValueType));
    }
    
    // UK2Node_CallFunction を作成
    UK2Node_CallFunction* MathNode = NewObject<UK2Node_CallFunction>(EventGraph);
    MathNode->FunctionReference.SetExternalMember(FunctionName, UKismetMathLibrary::StaticClass());
    MathNode->NodePosX = PosX;
    MathNode->NodePosY = PosY;
    MathNode->AllocateDefaultPins();
    EventGraph->AddNode(MathNode, false, false);
    
    // Blueprintをマーク
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // 結果を返す
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetStringField(TEXT("node_id"), MathNode->NodeGuid.ToString());
    ResultJson->SetStringField(TEXT("operation"), Operation);
    ResultJson->SetStringField(TEXT("value_type"), ValueType);
    
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(ResultJson);
}
```

#### HandleAddComparisonNode の修正

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCommands::HandleAddComparisonNode(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString Operation = Params->GetStringField(TEXT("operation"));
    FString ValueType = Params->HasField(TEXT("value_type")) ? 
        Params->GetStringField(TEXT("value_type")) : TEXT("Float");
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
    
    // 比較演算子に対応する関数名を決定
    FName FunctionName = NAME_None;
    if (ValueType == TEXT("Float"))
    {
        if (Operation == TEXT("Greater")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_FloatFloat);
        else if (Operation == TEXT("GreaterEqual")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_FloatFloat);
        else if (Operation == TEXT("Less")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_FloatFloat);
        else if (Operation == TEXT("LessEqual")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_FloatFloat);
        else if (Operation == TEXT("Equal")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_FloatFloat);
        else if (Operation == TEXT("NotEqual")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_FloatFloat);
    }
    else if (ValueType == TEXT("Int"))
    {
        if (Operation == TEXT("Greater")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt);
        else if (Operation == TEXT("GreaterEqual")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_IntInt);
        else if (Operation == TEXT("Less")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt);
        else if (Operation == TEXT("LessEqual")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_IntInt);
        else if (Operation == TEXT("Equal")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt);
        else if (Operation == TEXT("NotEqual")) FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_IntInt);
    }
    
    if (FunctionName == NAME_None)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported comparison/type: %s/%s"), *Operation, *ValueType));
    }
    
    // UK2Node_CallFunction を作成
    UK2Node_CallFunction* CompareNode = NewObject<UK2Node_CallFunction>(EventGraph);
    CompareNode->FunctionReference.SetExternalMember(FunctionName, UKismetMathLibrary::StaticClass());
    CompareNode->NodePosX = PosX;
    CompareNode->NodePosY = PosY;
    CompareNode->AllocateDefaultPins();
    EventGraph->AddNode(CompareNode, false, false);
    
    // Blueprintをマーク
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    
    // 結果を返す
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetStringField(TEXT("node_id"), CompareNode->NodeGuid.ToString());
    ResultJson->SetStringField(TEXT("operation"), Operation);
    ResultJson->SetStringField(TEXT("value_type"), ValueType);
    
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(ResultJson);
}
```

### 2. Build.cs の確認

`Kismet` モジュールが依存関係に含まれていることを確認：

```csharp
// SpirrowBridge.Build.cs
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", 
    "CoreUObject", 
    "Engine",
    "BlueprintGraph",
    "UnrealEd",
    "Kismet",  // UKismetMathLibrary用
    // ...
});
```

### 3. 関数名の検証

実装前に、UEソースコードで実際の関数名を確認すること。
以下のパスで確認可能：

```
Engine/Source/Runtime/Engine/Classes/Kismet/KismetMathLibrary.h
```

または、UEエディタで該当ノードを追加し `get_blueprint_graph` で取得して確認：

```python
# テスト用Blueprintを作成し、手動でノードを追加してから
get_blueprint_graph(blueprint_name="BP_Test", path="/Game/Blueprints")
```

## テスト手順

### 1. プラグインをリビルド

```bash
# UEエディタを閉じてからビルド
```

### 2. Math Node テスト

```python
# Float加算
add_math_node(
    blueprint_name="BP_MathTest",
    operation="Add",
    value_type="Float",
    node_position=[200, 0]
)

# Int乗算
add_math_node(
    blueprint_name="BP_MathTest",
    operation="Multiply",
    value_type="Int",
    node_position=[200, 150]
)
```

### 3. Comparison Node テスト

```python
# Float比較（大なり）
add_comparison_node(
    blueprint_name="BP_MathTest",
    operation="Greater",
    value_type="Float",
    node_position=[400, 0]
)

# Int比較（等しい）
add_comparison_node(
    blueprint_name="BP_MathTest",
    operation="Equal",
    value_type="Int",
    node_position=[400, 150]
)
```

### 4. 接続テスト

```python
# 変数とMathノードを接続
add_blueprint_variable(blueprint_name="BP_MathTest", variable_name="ValueA", variable_type="Float")
add_blueprint_variable(blueprint_name="BP_MathTest", variable_name="ValueB", variable_type="Float")

var_a = add_variable_get_node(blueprint_name="BP_MathTest", variable_name="ValueA")
var_b = add_variable_get_node(blueprint_name="BP_MathTest", variable_name="ValueB")
math = add_math_node(blueprint_name="BP_MathTest", operation="Add", value_type="Float")

# 接続（ピン名は実際のノードを確認して調整）
connect_blueprint_nodes(
    blueprint_name="BP_MathTest",
    source_node_id=var_a["node_id"],
    source_pin="ValueA",
    target_node_id=math["node_id"],
    target_pin="A"
)
```

## 補足：コンパクト表示が必要な場合

将来的にコンパクト表示（演算子記号のみの表示）が必要な場合は、
`UK2Node_CommutativeAssociativeBinaryOperator` を使用する：

```cpp
#include "K2Node_CommutativeAssociativeBinaryOperator.h"

// 可換結合演算子（+, *）のみ対応
UK2Node_CommutativeAssociativeBinaryOperator* MathNode = 
    NewObject<UK2Node_CommutativeAssociativeBinaryOperator>(EventGraph);

UFunction* Function = UKismetMathLibrary::StaticClass()->FindFunctionByName(FunctionName);
MathNode->SetFromFunction(Function);
MathNode->NodePosX = PosX;
MathNode->NodePosY = PosY;
MathNode->AllocateDefaultPins();
EventGraph->AddNode(MathNode, false, false);
```

**注意**: この方式は `+` と `*` のみに対応。`-` と `/` は可換ではないため使用不可。

## 優先度

中

## 関連ファイル

- `Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeBlueprintNodeCommands.cpp` - 修正対象
- `Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeBlueprintNodeCommands.h`
- `Python/tools/node_tools.py` - 変更不要（API定義は既存のまま）
