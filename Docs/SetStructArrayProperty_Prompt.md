# 機能名: set_struct_array_property 実装

## 概要

Blueprint の構造体配列プロパティ（例: `TArray<FTrapInventoryEntry>`）を MCP 経由で設定するツールを追加する。

## 背景

- TrapxTrapCpp の BP_PlayerCharacter には `TrapInventory` プロパティがある
- 型: `TArray<FTrapInventoryEntry>`
- 既存の `set_blueprint_class_array` は `TSubclassOf` 配列のみ対応
- 構造体配列は UE の Reflection API で個別フィールドアクセスが必要

## 仕様

### ツールインターフェース

```python
set_struct_array_property(
    blueprint_name: str,        # "BP_PlayerCharacter"
    property_name: str,         # "TrapInventory"
    values: List[Dict],         # JSON 形式の構造体配列
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]
```

### 入力例 (TrapInventory)

```python
set_struct_array_property(
    blueprint_name="BP_PlayerCharacter",
    property_name="TrapInventory",
    values=[
        {
            "TrapClass": "/Game/TrapxTrap/Blueprints/Traps/BP_ExplosionTrap.BP_ExplosionTrap_C",
            "InitialCount": 3,
            "MaxCount": 5,
            "CurrentCount": 0
        },
        {
            "TrapClass": "/Game/TrapxTrap/Blueprints/Traps/BP_SpikeTrap.BP_SpikeTrap_C",
            "InitialCount": 2,
            "MaxCount": 3,
            "CurrentCount": 0
        }
    ],
    path="/Game/TrapxTrap/Blueprints/Characters"
)
```

### 構造体フィールド型サポート

| フィールド型 | JSON 入力形式 | 例 |
|-------------|---------------|-----|
| `TSubclassOf<T>` | String (クラスパス) | `/Game/.../BP_Trap.BP_Trap_C` |
| `int32` | Number | `3` |
| `float` | Number | `1.5` |
| `bool` | Boolean | `true` |
| `FString` | String | `"Name"` |
| `FName` | String | `"TagName"` |

## 実装内容

### 1. Python 側 (`Python/tools/blueprint_tools.py`)

`register_blueprint_tools` 関数内に追加:

```python
@mcp.tool()
def set_struct_array_property(
    ctx: Context,
    blueprint_name: str,
    property_name: str,
    values: List[Dict[str, Any]],
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    Set a struct array property on a Blueprint.

    This tool allows you to configure Blueprint properties that are arrays of structs,
    such as TrapInventory (list of FTrapInventoryEntry) or custom struct arrays.

    Args:
        blueprint_name: Name of the target Blueprint (e.g., "BP_PlayerCharacter")
        property_name: Name of the array property (e.g., "TrapInventory")
        values: List of dictionaries, where each dictionary represents a struct.
                Keys are field names, values are the field values.
                - TSubclassOf fields: Use full class path ending with "_C"
                - Numeric fields: Use numbers
                - Boolean fields: Use true/false
                - String fields: Use strings
        path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

    Returns:
        Dict containing success status, property name, and number of elements set

    Example:
        # Set trap inventory for player character
        set_struct_array_property(
            blueprint_name="BP_PlayerCharacter",
            property_name="TrapInventory",
            values=[
                {
                    "TrapClass": "/Game/TrapxTrap/Blueprints/Traps/BP_ExplosionTrap.BP_ExplosionTrap_C",
                    "InitialCount": 3,
                    "MaxCount": 5,
                    "CurrentCount": 0
                }
            ],
            path="/Game/TrapxTrap/Blueprints/Characters"
        )
    """
    from unreal_mcp_server import get_unreal_connection
    import json

    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.error("Failed to connect to Unreal Engine")
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        params = {
            "blueprint_name": blueprint_name,
            "property_name": property_name,
            "values": values,  # JSON serializable list of dicts
            "path": path
        }

        logger.info(f"Setting struct array property '{property_name}' on '{blueprint_name}' with {len(values)} elements")
        response = unreal.send_command("set_struct_array_property", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Set struct array property response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error setting struct array property: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### 2. C++ 側 - ヘッダー (`SpirrowBridgeBlueprintCommands.h`)

`HandleSetBlueprintClassArray` の下に追加:

```cpp
TSharedPtr<FJsonObject> HandleSetStructArrayProperty(const TSharedPtr<FJsonObject>& Params);
```

### 3. C++ 側 - ルーティング (`SpirrowBridgeBlueprintCommands.cpp`)

`HandleCommand` 関数に追加:

```cpp
else if (CommandType == TEXT("set_struct_array_property"))
{
    return HandleSetStructArrayProperty(Params);
}
```

### 4. C++ 側 - 実装 (`SpirrowBridgeBlueprintCommands.cpp`)

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintCommands::HandleSetStructArrayProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString PropertyName = Params->GetStringField(TEXT("property_name"));
    
    const TArray<TSharedPtr<FJsonValue>>* ValuesArray = nullptr;
    if (!Params->TryGetArrayField(TEXT("values"), ValuesArray))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: values"));
    }

    FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/Blueprints");

    // Load Blueprint
    FString BlueprintPath = Path + TEXT("/") + BlueprintName + TEXT(".") + BlueprintName;
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    // Get CDO
    UClass* BPClass = Blueprint->GeneratedClass;
    if (!BPClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Blueprint has no generated class"));
    }

    UObject* CDO = BPClass->GetDefaultObject();
    if (!CDO)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get CDO"));
    }

    // Find array property
    FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(BPClass, *PropertyName);
    if (!ArrayProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Property not found: %s"), *PropertyName));
    }

    // Verify inner property is a struct
    FStructProperty* StructProp = CastField<FStructProperty>(ArrayProp->Inner);
    if (!StructProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Property %s is not a struct array"), *PropertyName));
    }

    UScriptStruct* StructType = StructProp->Struct;
    UE_LOG(LogTemp, Log, TEXT("Setting struct array property %s of type %s"), *PropertyName, *StructType->GetName());

    // Get array helper
    void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(CDO);
    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

    // Clear existing array and resize
    ArrayHelper.EmptyValues();
    ArrayHelper.Resize(ValuesArray->Num());

    int32 SetCount = 0;
    TArray<FString> Errors;

    // Set each element
    for (int32 i = 0; i < ValuesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& ElementValue = (*ValuesArray)[i];
        if (ElementValue->Type != EJson::Object)
        {
            Errors.Add(FString::Printf(TEXT("Element %d is not an object"), i));
            continue;
        }

        const TSharedPtr<FJsonObject>& ElementObj = ElementValue->AsObject();
        void* ElementPtr = ArrayHelper.GetRawPtr(i);

        // Set each field of the struct
        for (TFieldIterator<FProperty> PropIt(StructType); PropIt; ++PropIt)
        {
            FProperty* FieldProp = *PropIt;
            FString FieldName = FieldProp->GetName();

            if (!ElementObj->HasField(FieldName))
            {
                continue; // Skip fields not provided in JSON
            }

            TSharedPtr<FJsonValue> FieldValue = ElementObj->TryGetField(FieldName);
            if (!FieldValue.IsValid())
            {
                continue;
            }

            void* FieldPtr = FieldProp->ContainerPtrToValuePtr<void>(ElementPtr);

            // Handle different field types
            if (FClassProperty* ClassFieldProp = CastField<FClassProperty>(FieldProp))
            {
                // TSubclassOf field
                FString ClassPath = FieldValue->AsString();
                UClass* LoadedClass = LoadObject<UClass>(nullptr, *ClassPath);
                if (LoadedClass)
                {
                    ClassFieldProp->SetObjectPropertyValue(FieldPtr, LoadedClass);
                    UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, *LoadedClass->GetName());
                }
                else
                {
                    Errors.Add(FString::Printf(TEXT("Element %d: Failed to load class for %s: %s"), i, *FieldName, *ClassPath));
                }
            }
            else if (FIntProperty* IntFieldProp = CastField<FIntProperty>(FieldProp))
            {
                int32 IntValue = static_cast<int32>(FieldValue->AsNumber());
                IntFieldProp->SetPropertyValue(FieldPtr, IntValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %d"), i, *FieldName, IntValue);
            }
            else if (FFloatProperty* FloatFieldProp = CastField<FFloatProperty>(FieldProp))
            {
                float FloatValue = static_cast<float>(FieldValue->AsNumber());
                FloatFieldProp->SetPropertyValue(FieldPtr, FloatValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %f"), i, *FieldName, FloatValue);
            }
            else if (FDoubleProperty* DoubleFieldProp = CastField<FDoubleProperty>(FieldProp))
            {
                double DoubleValue = FieldValue->AsNumber();
                DoubleFieldProp->SetPropertyValue(FieldPtr, DoubleValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %f"), i, *FieldName, DoubleValue);
            }
            else if (FBoolProperty* BoolFieldProp = CastField<FBoolProperty>(FieldProp))
            {
                bool BoolValue = FieldValue->AsBool();
                BoolFieldProp->SetPropertyValue(FieldPtr, BoolValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, BoolValue ? TEXT("true") : TEXT("false"));
            }
            else if (FStrProperty* StrFieldProp = CastField<FStrProperty>(FieldProp))
            {
                FString StrValue = FieldValue->AsString();
                StrFieldProp->SetPropertyValue(FieldPtr, StrValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, *StrValue);
            }
            else if (FNameProperty* NameFieldProp = CastField<FNameProperty>(FieldProp))
            {
                FName NameValue = FName(*FieldValue->AsString());
                NameFieldProp->SetPropertyValue(FieldPtr, NameValue);
                UE_LOG(LogTemp, Log, TEXT("  [%d] Set %s = %s"), i, *FieldName, *NameValue.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("  [%d] Unsupported field type for %s: %s"), 
                    i, *FieldName, *FieldProp->GetClass()->GetName());
            }
        }

        SetCount++;
    }

    // Mark modified and compile
    Blueprint->Modify();
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Create response
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());
    ResultJson->SetBoolField(TEXT("success"), Errors.Num() == 0);
    ResultJson->SetStringField(TEXT("property_name"), PropertyName);
    ResultJson->SetStringField(TEXT("struct_type"), StructType->GetName());
    ResultJson->SetNumberField(TEXT("count"), SetCount);

    if (Errors.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrorsArray;
        for (const FString& Error : Errors)
        {
            ErrorsArray.Add(MakeShareable(new FJsonValueString(Error)));
        }
        ResultJson->SetArrayField(TEXT("errors"), ErrorsArray);
    }

    return ResultJson;
}
```

## テスト

### 1. FTrapInventoryEntry の構造体定義確認

```cpp
// TrapxTrapCpp の TrapBase.h にあるはず
USTRUCT(BlueprintType)
struct FTrapInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<ATrapBase> TrapClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 InitialCount = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 MaxCount = 3;

    UPROPERTY(BlueprintReadWrite)
    int32 CurrentCount = 0;
};
```

### 2. MCP テスト

```python
# Claude.ai または Python から実行
set_struct_array_property(
    blueprint_name="BP_PlayerCharacter",
    property_name="TrapInventory",
    values=[
        {
            "TrapClass": "/Game/TrapxTrap/Blueprints/Traps/BP_ExplosionTrap.BP_ExplosionTrap_C",
            "InitialCount": 3,
            "MaxCount": 5,
            "CurrentCount": 0
        }
    ],
    path="/Game/TrapxTrap/Blueprints/Characters"
)
```

### 3. 確認項目

- [ ] BP_PlayerCharacter の TrapInventory に要素が追加されている
- [ ] TrapClass が正しいクラスを参照している
- [ ] InitialCount, MaxCount, CurrentCount が正しい値

## 補足事項

### 既存実装との比較

| ツール | 対応型 | 実装パターン |
|--------|--------|--------------|
| `set_blueprint_class_array` | `TArray<TSubclassOf<T>>` | 単純なクラス配列 |
| `set_struct_array_property` (新規) | `TArray<FMyStruct>` | 構造体配列、複数フィールド |

### 拡張可能性

将来的に追加サポートが必要なフィールド型:
- `TSoftObjectPtr<T>` - ソフト参照
- `FVector`, `FRotator` - ベクトル/回転
- ネストした構造体

### 注意点

1. **クラスパスの `_C` サフィックス**: Blueprint クラスは末尾に `_C` が必要
2. **フィールド名の大文字小文字**: UE の Reflection は正確なフィールド名が必要
3. **Blueprint コンパイル**: 設定後に自動コンパイルされる
