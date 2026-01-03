# 機能名: add_widget_array_variable 実装

## 概要

Widget Blueprintに配列型変数を追加するツール。
TArray<T> 型の変数を作成し、UIでのリスト表示やデータ管理に使用。

## 仕様

### Python側パラメータ

| パラメータ | 型 | 必須 | デフォルト | 説明 |
|-----------|-----|------|------------|------|
| widget_name | str | ✅ | - | Widget Blueprint名 |
| variable_name | str | ✅ | - | 変数名（例: "TrapNames"） |
| element_type | str | ✅ | - | 配列要素の型（String, Integer, Float等） |
| is_exposed | bool | | False | エディタで公開するか |
| category | str | | None | 変数カテゴリ |
| path | str | | "/Game/UI" | Content Browser パス |

### サポートする要素型

| 型名 | UE内部型 | 用途例 |
|------|---------|--------|
| Boolean | bool | フラグリスト |
| Integer | int32 | ID一覧、カウント |
| Float | float | 数値リスト |
| String | FString | 名前リスト |
| Text | FText | ローカライズテキスト |
| Vector | FVector | 座標リスト |
| Vector2D | FVector2D | 2D座標リスト |
| LinearColor | FLinearColor | カラーリスト |
| Texture2D | UTexture2D* | アイコンリスト |
| Object | UObject* | 汎用オブジェクト |

### 戻り値

```json
{
  "success": true,
  "widget_name": "WBP_TT_TrapSelector",
  "variable_name": "TrapNames",
  "variable_type": "TArray<String>",
  "element_type": "String",
  "is_exposed": false
}
```

## 実装内容

### 1. Python側 (`Python/tools/umg_tools.py`)

`register_umg_tools` 関数内に追加:

```python
@mcp.tool()
def add_widget_array_variable(
    ctx: Context,
    widget_name: str,
    variable_name: str,
    element_type: str,
    is_exposed: bool = False,
    category: str = None,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add an array variable to a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        variable_name: Name for the new array variable (e.g., "TrapNames")
        element_type: Type of array elements - "Boolean", "Integer", "Float", "String",
                      "Text", "Vector", "Vector2D", "LinearColor", "Texture2D", "Object"
        is_exposed: Whether to expose the variable in the editor
        category: Category name for the variable in the editor
        path: Content browser path to the widget

    Returns:
        Dict containing success status and variable details

    Example:
        # String array for trap names
        add_widget_array_variable(
            widget_name="WBP_TT_TrapSelector",
            variable_name="TrapNames",
            element_type="String",
            path="/Game/TrapxTrap/UI"
        )

        # Texture2D array for trap icons
        add_widget_array_variable(
            widget_name="WBP_TT_TrapSelector",
            variable_name="TrapIcons",
            element_type="Texture2D",
            path="/Game/TrapxTrap/UI"
        )
    """
    from unreal_mcp_server import get_unreal_connection

    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.error("Failed to connect to Unreal Engine")
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        params = {
            "widget_name": widget_name,
            "variable_name": variable_name,
            "element_type": element_type,
            "is_exposed": is_exposed,
            "path": path
        }

        if category is not None:
            params["category"] = category

        logger.info(f"Adding widget array variable with params: {params}")
        response = unreal.send_command("add_widget_array_variable", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Add widget array variable response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error adding widget array variable: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### 2. Unreal側 (`SpirrowBridgeUMGCommands.h`)

privateセクションに追加:

```cpp
// Phase 3: Array Variable Operations
TSharedPtr<FJsonObject> HandleAddWidgetArrayVariable(const TSharedPtr<FJsonObject>& Params);
```

### 3. Unreal側 (`SpirrowBridgeUMGCommands.cpp`)

#### HandleCommand内ルーティング追加:

```cpp
else if (CommandType == TEXT("add_widget_array_variable"))
{
    return HandleAddWidgetArrayVariable(Params);
}
```

#### 関数実装:

```cpp
// Phase 3: Add Widget Array Variable
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetArrayVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters
    FString WidgetName = Params->GetStringField(TEXT("widget_name"));
    FString VariableName = Params->GetStringField(TEXT("variable_name"));
    FString ElementType = Params->GetStringField(TEXT("element_type"));
    bool bIsExposed = Params->HasField(TEXT("is_exposed")) ? Params->GetBoolField(TEXT("is_exposed")) : false;
    FString Category = Params->HasField(TEXT("category")) ? Params->GetStringField(TEXT("category")) : TEXT("");
    FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

    // Load Widget Blueprint
    FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
    UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
    }

    // Get Blueprint class
    UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(WidgetBP->GeneratedClass);
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get Blueprint generated class"));
    }

    // Check if variable already exists
    for (const FBPVariableDescription& Var : WidgetBP->NewVariables)
    {
        if (Var.VarName == *VariableName)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Variable '%s' already exists"), *VariableName));
        }
    }

    // Set up the element pin type
    FEdGraphPinType ElementPinType;
    if (!SetupPinType(ElementType, ElementPinType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported element type: %s"), *ElementType));
    }

    // Create array pin type
    FEdGraphPinType ArrayPinType;
    ArrayPinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;  // Will be overwritten
    ArrayPinType.ContainerType = EPinContainerType::Array;
    
    // Copy element type info to array type
    ArrayPinType.PinCategory = ElementPinType.PinCategory;
    ArrayPinType.PinSubCategory = ElementPinType.PinSubCategory;
    ArrayPinType.PinSubCategoryObject = ElementPinType.PinSubCategoryObject;
    ArrayPinType.PinSubCategoryMemberReference = ElementPinType.PinSubCategoryMemberReference;

    // Create the variable description
    FBPVariableDescription NewVar;
    NewVar.VarName = *VariableName;
    NewVar.VarGuid = FGuid::NewGuid();
    NewVar.VarType = ArrayPinType;
    NewVar.PropertyFlags = CPF_Edit | CPF_BlueprintVisible | CPF_DisableEditOnInstance;
    
    if (bIsExposed)
    {
        NewVar.PropertyFlags |= CPF_ExposeOnSpawn;
    }

    if (!Category.IsEmpty())
    {
        NewVar.Category = FText::FromString(Category);
    }

    // Add to Blueprint
    WidgetBP->NewVariables.Add(NewVar);

    // Compile
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Mark dirty
    WidgetBP->MarkPackageDirty();

    // Response
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("widget_name"), WidgetName);
    Response->SetStringField(TEXT("variable_name"), VariableName);
    Response->SetStringField(TEXT("variable_type"), FString::Printf(TEXT("TArray<%s>"), *ElementType));
    Response->SetStringField(TEXT("element_type"), ElementType);
    Response->SetBoolField(TEXT("is_exposed"), bIsExposed);

    return Response;
}
```

### 4. SpirrowBridge.cpp ルーティング追加

`ExecuteCommand` 内の UMG コマンド判定に追加:

```cpp
CommandType == TEXT("add_widget_array_variable") ||
```

## テスト

```python
# テスト1: String配列
add_widget_array_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="TrapNames",
    element_type="String",
    path="/Game/TrapxTrap/UI"
)

# テスト2: Integer配列
add_widget_array_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="TrapCounts",
    element_type="Integer",
    path="/Game/TrapxTrap/UI"
)

# テスト3: Texture2D配列（アイコン用）
add_widget_array_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="TrapIcons",
    element_type="Texture2D",
    is_exposed=True,
    path="/Game/TrapxTrap/UI"
)
```

## 補足事項

- `SetupPinType` ヘルパー関数は既存のPhase 2実装を再利用
- 配列のデフォルト値設定は別ツール `set_widget_array_default` で対応予定
- Blueprint コンパイルが必要なため、変数追加後に自動コンパイル実行

---

**作成日**: 2025-12-24
**フェーズ**: UMG Phase 3 - Array Variables
