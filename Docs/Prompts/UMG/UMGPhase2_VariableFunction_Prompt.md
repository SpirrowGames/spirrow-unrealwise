# UMG Phase 2: Widget 変数・関数操作 実装プロンプト

## 概要

Widget Blueprint に変数・関数を追加し、プロパティバインディングを構築する機能を実装する。

## 実装するツール

| # | ツール名 | 機能 | 優先度 |
|---|----------|------|--------|
| 1 | `add_widget_variable` | Widget BP に変数追加 | 高 |
| 2 | `set_widget_variable_default` | 変数のデフォルト値設定 | 中 |
| 3 | `bind_widget_to_variable` | 要素プロパティを変数にバインド | 高 |
| 4 | `add_widget_function` | カスタム関数作成 | 高 |
| 5 | `add_widget_event` | カスタムイベント作成 | 中 |

---

## 1. add_widget_variable

### Python インターフェース

```python
@mcp.tool()
async def add_widget_variable(
    ctx: Context,
    widget_name: str,
    variable_name: str,
    variable_type: str,
    default_value: str = None,
    is_exposed: bool = False,
    category: str = None,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Widget Blueprint に変数を追加する。

    Args:
        widget_name: Widget BP 名 (例: "WBP_TT_TrapSelector")
        variable_name: 変数名 (例: "CurrentTrapName")
        variable_type: 型 (Boolean, Integer, Float, String, Vector, Text, 
                          TimerHandle, Texture2D, Object など)
        default_value: デフォルト値 (オプション、型に応じた文字列)
        is_exposed: エディタに公開するか
        category: 変数のカテゴリ (オプション)
        path: Widget BP のパス

    Returns:
        Dict: {"success": True, "variable_name": "...", "variable_type": "..."}

    Example:
        add_widget_variable(
            widget_name="WBP_TT_TrapSelector",
            variable_name="bIsVisible",
            variable_type="Boolean",
            default_value="false",
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装

`SpirrowBridgeUMGCommands.cpp` に追加:

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetVariable(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    FString DefaultValue;
    bool bHasDefault = Params->TryGetStringField(TEXT("default_value"), DefaultValue);

    bool bIsExposed = false;
    Params->TryGetBoolField(TEXT("is_exposed"), bIsExposed);

    FString Category;
    Params->TryGetStringField(TEXT("category"), Category);

    // Widget Blueprint をロード
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    // Pin Type を設定
    FEdGraphPinType PinType;
    if (!SetupPinType(VariableType, PinType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
    }

    // 変数を追加
    FBlueprintEditorUtils::AddMemberVariable(WidgetBP, FName(*VariableName), PinType);

    // 変数のプロパティを設定
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : WidgetBP->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        // エディタ公開
        if (bIsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit | CPF_BlueprintVisible;
        }

        // カテゴリ設定
        if (!Category.IsEmpty())
        {
            NewVar->Category = FText::FromString(Category);
        }

        // デフォルト値設定
        if (bHasDefault)
        {
            NewVar->DefaultValue = DefaultValue;
        }
    }

    // Blueprint を更新
    FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // 成功レスポンス
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    return ResultObj;
}

// Pin Type 設定ヘルパー
bool FSpirrowBridgeUMGCommands::SetupPinType(const FString& TypeName, FEdGraphPinType& OutPinType)
{
    OutPinType.PinCategory = NAME_None;
    OutPinType.PinSubCategory = NAME_None;
    OutPinType.PinSubCategoryObject = nullptr;
    OutPinType.ContainerType = EPinContainerType::None;
    OutPinType.bIsReference = false;

    if (TypeName == TEXT("Boolean") || TypeName == TEXT("Bool"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (TypeName == TEXT("Integer") || TypeName == TEXT("Int"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (TypeName == TEXT("Float") || TypeName == TEXT("Double"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    }
    else if (TypeName == TEXT("String"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (TypeName == TEXT("Name"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
    }
    else if (TypeName == TEXT("Text"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
    }
    else if (TypeName == TEXT("Vector"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else if (TypeName == TEXT("Vector2D"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
    }
    else if (TypeName == TEXT("Rotator"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
    }
    else if (TypeName == TEXT("Transform"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
    }
    else if (TypeName == TEXT("LinearColor") || TypeName == TEXT("Color"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
    }
    else if (TypeName == TEXT("TimerHandle"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        OutPinType.PinSubCategoryObject = TBaseStructure<FTimerHandle>::Get();
    }
    else if (TypeName == TEXT("Texture2D"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = UTexture2D::StaticClass();
    }
    else if (TypeName == TEXT("Object"))
    {
        OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        OutPinType.PinSubCategoryObject = UObject::StaticClass();
    }
    else
    {
        // カスタム構造体/クラスの場合
        UClass* FoundClass = FindObject<UClass>(nullptr, *TypeName);
        if (FoundClass)
        {
            OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
            OutPinType.PinSubCategoryObject = FoundClass;
        }
        else
        {
            UScriptStruct* FoundStruct = FindObject<UScriptStruct>(nullptr, *TypeName);
            if (FoundStruct)
            {
                OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
                OutPinType.PinSubCategoryObject = FoundStruct;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}
```

---

## 2. set_widget_variable_default

### Python インターフェース

```python
@mcp.tool()
async def set_widget_variable_default(
    ctx: Context,
    widget_name: str,
    variable_name: str,
    default_value: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Widget Blueprint の変数にデフォルト値を設定する。

    Args:
        widget_name: Widget BP 名
        variable_name: 変数名
        default_value: デフォルト値（型に応じた文字列表現）
                       Boolean: "true" / "false"
                       Integer: "123"
                       Float: "1.5"
                       String: "Hello"
                       Vector: "(X=0,Y=0,Z=100)"
        path: Widget BP のパス

    Returns:
        Dict: {"success": True, "variable_name": "...", "default_value": "..."}
    """
```

### C++ 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleSetWidgetVariableDefault(const TSharedPtr<FJsonObject>& Params)
{
    FString WidgetName, VariableName, DefaultValue, Path = TEXT("/Game/UI");
    
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
        !Params->TryGetStringField(TEXT("variable_name"), VariableName) ||
        !Params->TryGetStringField(TEXT("default_value"), DefaultValue))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }
    Params->TryGetStringField(TEXT("path"), Path);

    // Widget Blueprint をロード
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found"), *WidgetName));
    }

    // 変数を検索
    FBPVariableDescription* Variable = nullptr;
    for (FBPVariableDescription& Var : WidgetBP->NewVariables)
    {
        if (Var.VarName == FName(*VariableName))
        {
            Variable = &Var;
            break;
        }
    }

    if (!Variable)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Variable '%s' not found in Widget Blueprint"), *VariableName));
    }

    // デフォルト値を設定
    Variable->DefaultValue = DefaultValue;

    // Blueprint を更新
    FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("default_value"), DefaultValue);
    return ResultObj;
}
```

---

## 3. bind_widget_to_variable

### Python インターフェース

```python
@mcp.tool()
async def bind_widget_to_variable(
    ctx: Context,
    widget_name: str,
    element_name: str,
    property_name: str,
    variable_name: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Widget 要素のプロパティを変数にバインドする。

    Args:
        widget_name: Widget BP 名
        element_name: 要素名 (例: "TrapNameText")
        property_name: プロパティ名 (例: "Text", "Visibility", "Percent")
        variable_name: バインド先の変数名
        path: Widget BP のパス

    Returns:
        Dict: {"success": True, "binding_function": "GetXxx"}

    Note:
        内部的にバインディング関数を自動生成する。
        例: Text プロパティに CurrentTrapName をバインド
            → GetCurrentTrapNameText() 関数が自動生成される
    """
```

### C++ 実装概要

バインディングは UMG の内部機構で、Getter 関数を作成して登録する必要がある。

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleBindWidgetToVariable(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString WidgetName, ElementName, PropertyName, VariableName, Path = TEXT("/Game/UI");
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
        !Params->TryGetStringField(TEXT("element_name"), ElementName) ||
        !Params->TryGetStringField(TEXT("property_name"), PropertyName) ||
        !Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
    }
    Params->TryGetStringField(TEXT("path"), Path);

    // Widget Blueprint をロード
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Blueprint not found"));
    }

    // 要素を検索
    UWidget* Element = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
    if (!Element)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget element not found"));
    }

    // バインディング関数名を生成
    FString FunctionName = FString::Printf(TEXT("Get%s%s"), *VariableName, *PropertyName);

    // 既存のバインディング関数を検索
    UEdGraph* FuncGraph = nullptr;
    for (UEdGraph* Graph : WidgetBP->FunctionGraphs)
    {
        if (Graph->GetFName() == FName(*FunctionName))
        {
            FuncGraph = Graph;
            break;
        }
    }

    // 関数グラフを作成（存在しない場合）
    if (!FuncGraph)
    {
        FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
            WidgetBP,
            FName(*FunctionName),
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass()
        );
        FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);

        // Entry ノードを作成
        UK2Node_FunctionEntry* EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
        FuncGraph->AddNode(EntryNode, false, false);
        EntryNode->NodePosX = 0;
        EntryNode->NodePosY = 0;
        EntryNode->FunctionReference.SetExternalMember(FName(*FunctionName), WidgetBP->GeneratedClass);
        EntryNode->AllocateDefaultPins();

        // 変数を取得するノードを追加
        UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
        GetVarNode->VariableReference.SetSelfMember(FName(*VariableName));
        FuncGraph->AddNode(GetVarNode, false, false);
        GetVarNode->NodePosX = 200;
        GetVarNode->NodePosY = 0;
        GetVarNode->AllocateDefaultPins();

        // Result ノードを作成して接続
        UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
        FuncGraph->AddNode(ResultNode, false, false);
        ResultNode->NodePosX = 400;
        ResultNode->NodePosY = 0;
        ResultNode->AllocateDefaultPins();

        // ノードを接続
        // (Entry → GetVar → Result の接続ロジック)
    }

    // プロパティバインディングを設定
    // Widget の PropertyBindings にエントリを追加
    FDelegateProperty* BindingProperty = FindFProperty<FDelegateProperty>(
        Element->GetClass(), 
        FName(*PropertyName)
    );
    
    if (BindingProperty)
    {
        // バインディングを設定
        // (PropertyBindings の操作 - UMG 内部 API を使用)
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("binding_function"), FunctionName);
    return ResultObj;
}
```

**注意**: プロパティバインディングの実装は複雑で、UMG の内部 API への深い理解が必要。
Phase 2 では基本実装に留め、複雑なケースは Phase 3 で対応予定。

---

## 4. add_widget_function

### Python インターフェース

```python
@mcp.tool()
async def add_widget_function(
    ctx: Context,
    widget_name: str,
    function_name: str,
    inputs: List[Dict] = None,
    outputs: List[Dict] = None,
    is_pure: bool = False,
    category: str = None,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Widget Blueprint にカスタム関数を作成する。

    Args:
        widget_name: Widget BP 名
        function_name: 関数名 (例: "UpdateTrapSelection")
        inputs: 入力パラメータ [{"name": "TrapName", "type": "String"}, ...]
        outputs: 出力パラメータ [{"name": "Success", "type": "Boolean"}]
        is_pure: Pure 関数か（実行ピンなし）
        category: 関数のカテゴリ
        path: Widget BP のパス

    Returns:
        Dict: {"success": True, "function_name": "...", "graph_id": "..."}
    """
```

### C++ 実装

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

    // Widget Blueprint をロード
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Blueprint not found"));
    }

    // 関数グラフを作成
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

    // Blueprint に関数を追加
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);

    // Entry ノードを作成
    UK2Node_FunctionEntry* EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
    FuncGraph->AddNode(EntryNode, false, false);
    EntryNode->NodePosX = 0;
    EntryNode->NodePosY = 0;
    EntryNode->AllocateDefaultPins();

    // 入力パラメータを追加
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

                FEdGraphPinType PinType;
                if (SetupPinType(ParamType, PinType))
                {
                    // 入力ピンを追加
                    EntryNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
                }
            }
        }
    }

    // 出力パラメータがある場合は Result ノードを作成
    const TArray<TSharedPtr<FJsonValue>>* OutputsArray;
    if (Params->TryGetArrayField(TEXT("outputs"), OutputsArray) && OutputsArray->Num() > 0)
    {
        UK2Node_FunctionResult* ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
        FuncGraph->AddNode(ResultNode, false, false);
        ResultNode->NodePosX = 400;
        ResultNode->NodePosY = 0;
        ResultNode->AllocateDefaultPins();

        for (const TSharedPtr<FJsonValue>& OutputValue : *OutputsArray)
        {
            const TSharedPtr<FJsonObject>* OutputObj;
            if (OutputValue->TryGetObject(OutputObj))
            {
                FString ParamName, ParamType;
                (*OutputObj)->TryGetStringField(TEXT("name"), ParamName);
                (*OutputObj)->TryGetStringField(TEXT("type"), ParamType);

                FEdGraphPinType PinType;
                if (SetupPinType(ParamType, PinType))
                {
                    ResultNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Input);
                }
            }
        }
    }

    // Pure 関数の設定
    if (bIsPure)
    {
        // ExtraFlags に Pure フラグを設定
        // EntryNode の設定で Pure 関数として構成
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("function_name"), FunctionName);
    ResultObj->SetStringField(TEXT("graph_id"), FuncGraph->GraphGuid.ToString());
    return ResultObj;
}
```

---

## 5. add_widget_event

### Python インターフェース

```python
@mcp.tool()
async def add_widget_event(
    ctx: Context,
    widget_name: str,
    event_name: str,
    inputs: List[Dict] = None,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Widget Blueprint にカスタムイベントを作成する。

    Args:
        widget_name: Widget BP 名
        event_name: イベント名 (例: "OnTrapSelected")
        inputs: 入力パラメータ [{"name": "TrapIndex", "type": "Integer"}]
        path: Widget BP のパス

    Returns:
        Dict: {"success": True, "event_name": "...", "node_id": "..."}
    """
```

---

## ファイル構成

### 変更が必要なファイル

1. **SpirrowBridgeUMGCommands.h**
   - 新しいハンドラ関数の宣言
   - `SetupPinType` ヘルパー関数の宣言

2. **SpirrowBridgeUMGCommands.cpp**
   - `HandleAddWidgetVariable`
   - `HandleSetWidgetVariableDefault`
   - `HandleBindWidgetToVariable`
   - `HandleAddWidgetFunction`
   - `HandleAddWidgetEvent`
   - `SetupPinType` ヘルパー

3. **SpirrowBridge.cpp**
   - `ExecuteCommand` に新しいコマンドのルーティングを追加

4. **Python/tools/umg_tools.py**
   - 5つの新しいツール関数を追加

---

## テスト手順

### 1. 変数追加テスト

```python
# Boolean 変数
add_widget_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="bIsVisible",
    variable_type="Boolean",
    default_value="false",
    path="/Game/TrapxTrap/UI"
)

# TimerHandle 変数
add_widget_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="HideTimer",
    variable_type="TimerHandle",
    path="/Game/TrapxTrap/UI"
)

# String 変数（エディタ公開）
add_widget_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="DefaultTrapName",
    variable_type="String",
    default_value="Explosion Trap",
    is_exposed=True,
    category="Display",
    path="/Game/TrapxTrap/UI"
)
```

### 2. 関数追加テスト

```python
add_widget_function(
    widget_name="WBP_TT_TrapSelector",
    function_name="UpdateTrapDisplay",
    inputs=[
        {"name": "TrapName", "type": "String"},
        {"name": "TrapCount", "type": "Integer"},
        {"name": "TrapIcon", "type": "Texture2D"}
    ],
    path="/Game/TrapxTrap/UI"
)
```

### 3. バインディングテスト

```python
# まず変数を追加
add_widget_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="CurrentTrapName",
    variable_type="Text",
    path="/Game/TrapxTrap/UI"
)

# Text にバインド
bind_widget_to_variable(
    widget_name="WBP_TT_TrapSelector",
    element_name="TrapNameText",
    property_name="Text",
    variable_name="CurrentTrapName",
    path="/Game/TrapxTrap/UI"
)
```

---

## 実装順序

1. **add_widget_variable** - 基本型のサポート
2. **add_widget_function** - 入力パラメータのみ
3. **add_widget_event** - シンプルなカスタムイベント
4. **set_widget_variable_default** - デフォルト値設定
5. **bind_widget_to_variable** - Text バインディングから開始

---

## 補足事項

### UE 5.5 での注意点

- `PC_Float` は `PC_Real` + `PC_Double` に変更されている
- `FindObject` の使用方法が変更されている箇所あり
- Widget Blueprint のバインディング API が一部変更

### 既知の制限

- 配列型変数は Phase 3 で対応予定
- マップ/セット型は未対応
- 複雑なバインディング（変換関数付き）は Phase 3

---

## 作成日

2025-12-22
