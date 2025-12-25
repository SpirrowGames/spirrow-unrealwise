# 機能名: Math/Comparison ノード実装修正

## 概要

`add_math_node` と `add_comparison_node` が "Failed to find math function" エラーで動作しない問題を修正する。

## 問題の原因

現在の実装では `UKismetMathLibrary` の関数を `UK2Node_CallFunction` で呼び出そうとしているが、
Blueprint の数学演算子ノード（+, -, *, /, >, <, ==等）は `UK2Node_CommutativeAssociativeBinaryOperator` という
専用のノードクラスで実装されている。

### 現在の実装（動作しない）

```cpp
// SpirrowBridgeBlueprintNodeCommands.cpp
// 関数名 "Add_FloatFloat" を探そうとするが見つからない
FName FunctionName = FName(*FString::Printf(TEXT("%s_%s%s"), *Operation, *ValueType, *ValueType));
UFunction* Function = UKismetMathLibrary::StaticClass()->FindFunctionByName(FunctionName);
```

### 正しいアプローチ

```cpp
// UK2Node_CommutativeAssociativeBinaryOperator を使用する
#include "K2Node_CommutativeAssociativeBinaryOperator.h"

UK2Node_CommutativeAssociativeBinaryOperator* MathNode = 
    NewObject<UK2Node_CommutativeAssociativeBinaryOperator>(EventGraph);
MathNode->SetFromFunction(Function);  // 演算子に対応する関数を設定
```

## 実装内容

### 1. C++側 (`SpirrowBridgeBlueprintNodeCommands.cpp`)

#### HandleAddMathNode の修正

```cpp
#include "K2Node_CommutativeAssociativeBinaryOperator.h"

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
    FName FunctionName;
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
    
    // 関数を取得
    UFunction* Function = UKismetMathLibrary::StaticClass()->FindFunctionByName(FunctionName);
    if (!Function)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to find function: %s"), *FunctionName.ToString()));
    }
    
    // UK2Node_CommutativeAssociativeBinaryOperator を作成
    UK2Node_CommutativeAssociativeBinaryOperator* MathNode = 
        NewObject<UK2Node_CommutativeAssociativeBinaryOperator>(EventGraph);
    MathNode->SetFromFunction(Function);
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
    // パラメータ取得（同様のパターン）
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString Operation = Params->GetStringField(TEXT("operation"));
    FString ValueType = Params->HasField(TEXT("value_type")) ? 
        Params->GetStringField(TEXT("value_type")) : TEXT("Float");
    FString Path = Params->HasField(TEXT("path")) ? 
        Params->GetStringField(TEXT("path")) : TEXT("/Game/Blueprints");
    
    // 位置取得（省略）
    
    // Blueprint & EventGraph取得（省略）
    
    // 比較演算子に対応する関数名を決定
    FName FunctionName;
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
    
    // 以下、MathNodeと同様の処理
    // ...
}
```

### 2. 必要なヘッダー追加

```cpp
// SpirrowBridgeBlueprintNodeCommands.cpp の先頭
#include "K2Node_CommutativeAssociativeBinaryOperator.h"
```

### 3. Build.cs の確認

`BlueprintGraph` モジュールが依存関係に含まれていることを確認：

```csharp
// SpirrowBridge.Build.cs
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", 
    "CoreUObject", 
    "Engine",
    "BlueprintGraph",  // 必須
    "UnrealEd",
    "Kismet",
    // ...
});
```

## 代替アプローチ（より簡単）

`UK2Node_CallFunction` を使って純粋関数として呼び出す方法もある：

```cpp
// 通常の関数ノードとして作成
UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(EventGraph);
FunctionNode->FunctionReference.SetExternalMember(FunctionName, UKismetMathLibrary::StaticClass());
FunctionNode->NodePosX = PosX;
FunctionNode->NodePosY = PosY;
FunctionNode->AllocateDefaultPins();
EventGraph->AddNode(FunctionNode, false, false);
```

この方法では演算子ノード（コンパクト表示）にはならないが、機能的には同等。

## テスト

```python
# Math Node テスト
add_math_node(
    blueprint_name="BP_MathTest",
    operation="Add",
    value_type="Float",
    node_position=[200, 0]
)

add_math_node(
    blueprint_name="BP_MathTest",
    operation="Multiply",
    value_type="Int",
    node_position=[200, 150]
)

# Comparison Node テスト
add_comparison_node(
    blueprint_name="BP_MathTest",
    operation="Greater",
    value_type="Float",
    node_position=[400, 0]
)

add_comparison_node(
    blueprint_name="BP_MathTest",
    operation="Equal",
    value_type="Int",
    node_position=[400, 150]
)
```

## 補足事項

### 関数名の確認方法

UE エディタでBlueprintを開き、該当ノードを追加してからグラフを `get_blueprint_graph` で取得すると、
実際の関数名を確認できる。

### 注意点

1. `GET_FUNCTION_NAME_CHECKED` マクロを使うと、存在しない関数名の場合にコンパイルエラーになるため安全
2. `UK2Node_CommutativeAssociativeBinaryOperator` は可換結合演算子（+, *）に最適化されている
3. 比較演算子は `UK2Node_CallFunction` でも問題なく動作する

## 優先度

中（動作するワークアラウンドが存在するため）

## 関連ファイル

- `Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeBlueprintNodeCommands.cpp`
- `Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeBlueprintNodeCommands.h`
- `Python/tools/node_tools.py`
