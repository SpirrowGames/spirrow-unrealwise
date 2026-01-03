# BugFix: HandleAddWidgetFunction Entry Node 重複エラー

## 問題概要

`add_widget_function` ツール実行時にエディタがクラッシュする。

## エラー内容

```
Assertion failed: EntryNodes.Num() == 1 
[File:D:\build\++UE5\Sync\Engine\Source\Editor\UnrealEd\Private\Kismet2\BlueprintEditorUtils.cpp] [Line: 5506]

UnrealEditor_SpirrowBridge!FSpirrowBridgeUMGCommands::HandleAddWidgetFunction() 
[SpirrowBridgeUMGCommands.cpp:2451]
```

## 原因分析

`HandleAddWidgetFunction` の実装で、`FBlueprintEditorUtils::AddFunctionGraph` を呼び出した後に、手動で `UK2Node_FunctionEntry` を作成している。

**問題のコード（現在の実装）**:
```cpp
// Add function to Blueprint
FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);

// ↓ 問題: AddFunctionGraph が既に Entry ノードを作成済みなのに、手動で追加している
UK2Node_FunctionEntry* EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
FuncGraph->AddNode(EntryNode, false, false);  // ← これで Entry ノードが2つになる
```

`AddFunctionGraph` は内部で自動的に `UK2Node_FunctionEntry` を作成するため、その後に手動で Entry ノードを作成すると、グラフに Entry ノードが2つ存在することになり、`EntryNodes.Num() == 1` のアサーションが失敗する。

## 修正方針

1. Entry ノードを手動作成せず、`AddFunctionGraph` が作成した既存ノードを検索して使用
2. Result ノードも同様に、既存チェックを行う
3. `HandleBindWidgetToVariable` も同じ問題があるため併せて修正

## 修正対象ファイル

- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGCommands.cpp`

## 修正内容

### 1. HandleAddWidgetFunction の修正

**修正前（行 2420-2470 付近）**:
```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetFunction(const TSharedPtr<FJsonObject>& Params)
{
    // ... パラメータ取得 ...

    // Create function graph
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
        WidgetBP,
        FName(*FunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass()
    );

    if (!FuncGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create function graph"));
    }

    // Add function to Blueprint
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);

    // ★問題: 手動で Entry ノードを作成（重複）
    UK2Node_FunctionEntry* EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
    FuncGraph->AddNode(EntryNode, false, false);
    EntryNode->NodePosX = 0;
    EntryNode->NodePosY = 0;
    EntryNode->AllocateDefaultPins();

    // ... 以下略 ...
}
```

**修正後**:
```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetFunction(const TSharedPtr<FJsonObject>& Params)
{
    FString WidgetName, FunctionName, Path = TEXT("/Game/UI");
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
        !Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }
    Params->TryGetStringField(TEXT("path"), Path);

    bool bIsPure = false;
    Params->TryGetBoolField(TEXT("is_pure"), bIsPure);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Blueprint not found"));
    }

    // Check if function already exists
    for (UEdGraph* Graph : WidgetBP->FunctionGraphs)
    {
        if (Graph->GetFName() == FName(*FunctionName))
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Function '%s' already exists"), *FunctionName));
        }
    }

    // Create function graph
    UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
        WidgetBP,
        FName(*FunctionName),
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass()
    );

    if (!FuncGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create function graph"));
    }

    // Add function to Blueprint - This automatically creates the Entry node
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);

    // ★修正: 既存の Entry ノードを検索（手動作成しない）
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : FuncGraph->Nodes)
    {
        EntryNode = Cast<UK2Node_FunctionEntry>(Node);
        if (EntryNode)
        {
            break;
        }
    }

    if (!EntryNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find function entry node"));
    }

    // Add input parameters to the existing Entry node
    const TArray<TSharedPtr<FJsonValue>>* InputsArray;
    if (Params->TryGetArrayField(TEXT("inputs"), InputsArray))
    {
        for (const TSharedPtr<FJsonValue>& InputValue : *InputsArray)
        {
            const TSharedPtr<FJsonObject>* InputObj;
            if (InputValue->TryGetObject(InputObj))
            {
                FString ParamName, ParamType;
                (*InputObj)->TryGetStringField(TEXT("name"), ParamName);
                (*InputObj)->TryGetStringField(TEXT("type"), ParamType);

                if (!ParamName.IsEmpty() && !ParamType.IsEmpty())
                {
                    FEdGraphPinType PinType;
                    if (SetupPinType(ParamType, PinType))
                    {
                        EntryNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
                    }
                }
            }
        }
    }

    // Add output parameters if provided - Create Result node only if needed
    const TArray<TSharedPtr<FJsonValue>>* OutputsArray;
    if (Params->TryGetArrayField(TEXT("outputs"), OutputsArray) && OutputsArray->Num() > 0)
    {
        // Check if Result node already exists
        UK2Node_FunctionResult* ResultNode = nullptr;
        for (UEdGraphNode* Node : FuncGraph->Nodes)
        {
            ResultNode = Cast<UK2Node_FunctionResult>(Node);
            if (ResultNode)
            {
                break;
            }
        }

        // Create Result node only if it doesn't exist
        if (!ResultNode)
        {
            ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
            FuncGraph->AddNode(ResultNode, false, false);
            ResultNode->NodePosX = 400;
            ResultNode->NodePosY = 0;
            ResultNode->AllocateDefaultPins();
        }

        for (const TSharedPtr<FJsonValue>& OutputValue : *OutputsArray)
        {
            const TSharedPtr<FJsonObject>* OutputObj;
            if (OutputValue->TryGetObject(OutputObj))
            {
                FString ParamName, ParamType;
                (*OutputObj)->TryGetStringField(TEXT("name"), ParamName);
                (*OutputObj)->TryGetStringField(TEXT("type"), ParamType);

                if (!ParamName.IsEmpty() && !ParamType.IsEmpty())
                {
                    FEdGraphPinType PinType;
                    if (SetupPinType(ParamType, PinType))
                    {
                        ResultNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Input);
                    }
                }
            }
        }
    }

    // Mark Blueprint as modified and compile
    FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("function_name"), FunctionName);
    ResultObj->SetStringField(TEXT("graph_id"), FuncGraph->GraphGuid.ToString());
    return ResultObj;
}
```

### 2. HandleBindWidgetToVariable の修正

同様の問題があるため、バインディング関数作成部分も修正が必要。

**修正ポイント（行 2550-2620 付近）**:

```cpp
// ★修正: 既存の Entry ノードを検索（手動作成しない）
UK2Node_FunctionEntry* EntryNode = nullptr;
for (UEdGraphNode* Node : FuncGraph->Nodes)
{
    EntryNode = Cast<UK2Node_FunctionEntry>(Node);
    if (EntryNode)
    {
        break;
    }
}

if (!EntryNode)
{
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find function entry node"));
}

// Entry ノードの設定やピン接続などは既存のものを使用
// 手動で NewObject<UK2Node_FunctionEntry> しない
```

## テスト手順

修正完了後、以下のテストを実行：

### Test 1: 入力パラメータ付き関数

```python
add_widget_function(
    widget_name="WBP_TT_TrapSelector",
    function_name="TestFunction1",
    inputs=[{"name": "TestParam", "type": "String"}],
    path="/Game/TrapxTrap/UI"
)
```

期待結果: 成功、クラッシュしない

### Test 2: 出力パラメータ付き関数

```python
add_widget_function(
    widget_name="WBP_TT_TrapSelector",
    function_name="TestFunction2",
    outputs=[{"name": "ReturnValue", "type": "Boolean"}],
    path="/Game/TrapxTrap/UI"
)
```

期待結果: 成功、クラッシュしない

### Test 3: 入力 + 出力パラメータ

```python
add_widget_function(
    widget_name="WBP_TT_TrapSelector",
    function_name="TestFunction3",
    inputs=[{"name": "InputA", "type": "Integer"}, {"name": "InputB", "type": "Float"}],
    outputs=[{"name": "Result", "type": "String"}],
    path="/Game/TrapxTrap/UI"
)
```

期待結果: 成功、クラッシュしない

### Test 4: バインディング関数

```python
bind_widget_to_variable(
    widget_name="WBP_TT_TrapSelector",
    element_name="TrapNameText",
    property_name="Text",
    variable_name="CurrentTrapName",
    path="/Game/TrapxTrap/UI"
)
```

期待結果: 成功、クラッシュしない

## 補足

- `FBlueprintEditorUtils::AddFunctionGraph` は内部で `UK2Node_FunctionEntry` を自動生成する
- Entry ノードは関数グラフに1つだけ存在する必要がある
- Result ノードは必要に応じて作成するが、重複チェックを行う
- 既存の Blueprint 変数追加実装 (`SpirrowBridgeBlueprintNodeCommands.cpp`) も参考にできる

## 関連ファイル

- 実装: `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGCommands.cpp`
- ヘッダ: `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeUMGCommands.h`
