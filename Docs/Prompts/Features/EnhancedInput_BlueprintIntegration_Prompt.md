# 機能名: Enhanced Input Blueprint統合機能 実装

## 概要
BlueprintのBeginPlayにAddMappingContextノードを自動追加し、Enhanced Inputの設定を自動化する機能を追加する。

## 仕様

### 1. add_mapping_context_to_blueprint

| パラメータ | 型 | 必須 | 説明 |
|-----------|-----|------|------|
| blueprint_name | str | ✓ | 対象Blueprint名 |
| context_name | str | ✓ | InputMappingContext名 (e.g., "IMC_Default") |
| priority | int | × | マッピングの優先度 (default: 0) |
| context_path | str | × | IMCアセットのパス (default: "/Game/Input") |
| path | str | × | Blueprintのパス (default: "/Game/Blueprints") |

**動作**:
1. BeginPlayイベントノードを検索（なければ作成）
2. GetControllerノードを追加
3. CastToPlayerControllerノードを追加
4. GetEnhancedInputLocalPlayerSubsystemノードを追加
5. AddMappingContextノードを追加
6. 各ノードを接続

**生成されるノード構成**:
```
[BeginPlay] → [GetController] → [CastTo PlayerController] → [GetEnhancedInputLocalPlayerSubsystem] → [AddMappingContext(IMC, Priority)]
```

**出力ピン**:
- success: bool
- nodes: 作成されたノードID一覧
- message: 結果メッセージ

### 2. set_default_mapping_context

| パラメータ | 型 | 必須 | 説明 |
|-----------|-----|------|------|
| blueprint_name | str | ✓ | 対象Blueprint名 (PlayerController or Character) |
| context_name | str | ✓ | InputMappingContext名 |
| priority | int | × | マッピングの優先度 (default: 0) |
| context_path | str | × | IMCアセットのパス (default: "/Game/Input") |
| path | str | × | Blueprintのパス (default: "/Game/Blueprints") |

**動作**:
APlayerControllerの`DefaultMappingContexts`プロパティに設定（UE5.4+の機能）
または、ACharacter系の場合は内部的にadd_mapping_context_to_blueprintを呼び出す

**注意**: DefaultMappingContextsはPlayerControllerでのみ有効。Characterの場合はBeginPlay方式を使用。

## 実装内容

### 1. Python側 (`project_tools.py`)

```python
@mcp.tool()
def add_mapping_context_to_blueprint(
    ctx: Context,
    blueprint_name: str,
    context_name: str,
    priority: int = 0,
    context_path: str = "/Game/Input",
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    Add AddMappingContext nodes to a Blueprint's BeginPlay.
    
    Creates the following node chain:
    BeginPlay → GetController → CastToPlayerController → 
    GetEnhancedInputLocalPlayerSubsystem → AddMappingContext
    
    Args:
        blueprint_name: Name of the target Blueprint
        context_name: Name of the InputMappingContext (e.g., "IMC_Default")
        priority: Mapping context priority (default: 0)
        context_path: Path where the IMC asset is located
        path: Content browser path where the blueprint is located
    
    Returns:
        Dict containing success status, created node IDs, and message
    
    Example:
        add_mapping_context_to_blueprint(
            blueprint_name="BP_PlayerCharacter",
            context_name="IMC_Default",
            priority=0,
            context_path="/Game/TrapxTrap/Input",
            path="/Game/TrapxTrap/Blueprints/Characters"
        )
    """
    from unreal_mcp_server import get_unreal_connection
    
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        params = {
            "blueprint_name": blueprint_name,
            "context_name": context_name,
            "priority": priority,
            "context_path": context_path,
            "path": path
        }
        
        response = unreal.send_command("add_mapping_context_to_blueprint", params)
        return response or {"success": False, "message": "No response from Unreal Engine"}
    
    except Exception as e:
        return {"success": False, "message": f"Error: {e}"}


@mcp.tool()
def set_default_mapping_context(
    ctx: Context,
    blueprint_name: str,
    context_name: str,
    priority: int = 0,
    context_path: str = "/Game/Input",
    path: str = "/Game/Blueprints"
) -> Dict[str, Any]:
    """
    Set the default Input Mapping Context for a PlayerController or Character Blueprint.
    
    For PlayerController: Sets DefaultMappingContexts property directly
    For Character/Pawn: Uses add_mapping_context_to_blueprint internally
    
    Args:
        blueprint_name: Name of the target Blueprint (PlayerController or Character)
        context_name: Name of the InputMappingContext (e.g., "IMC_Default")
        priority: Mapping context priority (default: 0)
        context_path: Path where the IMC asset is located
        path: Content browser path where the blueprint is located
    
    Returns:
        Dict containing success status and message
    
    Example:
        set_default_mapping_context(
            blueprint_name="BP_PlayerController",
            context_name="IMC_Default",
            context_path="/Game/TrapxTrap/Input",
            path="/Game/TrapxTrap/Blueprints/Controllers"
        )
    """
    from unreal_mcp_server import get_unreal_connection
    
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        params = {
            "blueprint_name": blueprint_name,
            "context_name": context_name,
            "priority": priority,
            "context_path": context_path,
            "path": path
        }
        
        response = unreal.send_command("set_default_mapping_context", params)
        return response or {"success": False, "message": "No response from Unreal Engine"}
    
    except Exception as e:
        return {"success": False, "message": f"Error: {e}"}
```

### 2. Unreal側 (`SpirrowBridgeProjectCommands.h`)

```cpp
// Private section に追加
private:
    TSharedPtr<FJsonObject> HandleAddMappingContextToBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetDefaultMappingContext(const TSharedPtr<FJsonObject>& Params);
```

### 3. Unreal側 (`SpirrowBridgeProjectCommands.cpp`)

HandleCommand に追加:
```cpp
else if (CommandType == TEXT("add_mapping_context_to_blueprint"))
{
    return HandleAddMappingContextToBlueprint(Params);
}
else if (CommandType == TEXT("set_default_mapping_context"))
{
    return HandleSetDefaultMappingContext(Params);
}
```

HandleAddMappingContextToBlueprint 実装:
```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeProjectCommands::HandleAddMappingContextToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString BlueprintName, ContextName, ContextPath, Path;
    int32 Priority = 0;
    
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name'"));
    if (!Params->TryGetStringField(TEXT("context_name"), ContextName))
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'context_name'"));
    
    Params->TryGetStringField(TEXT("context_path"), ContextPath);
    Params->TryGetStringField(TEXT("path"), Path);
    Params->TryGetNumberField(TEXT("priority"), Priority);
    
    if (ContextPath.IsEmpty()) ContextPath = TEXT("/Game/Input");
    if (Path.IsEmpty()) Path = TEXT("/Game/Blueprints");
    
    // Blueprintをロード
    FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *FullPath);
    if (!Blueprint)
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Blueprint not found: %s"), *FullPath));
    
    // IMCをロード
    FString FullContextPath = FString::Printf(TEXT("%s/%s.%s"), *ContextPath, *ContextName, *ContextName);
    UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr, *FullContextPath);
    if (!MappingContext)
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("IMC not found: %s"), *FullContextPath));
    
    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!EventGraph)
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Event graph not found"));
    
    // ノード配置用の開始位置
    int32 NodeX = 0;
    int32 NodeY = 0;
    
    TArray<FString> CreatedNodeIds;
    
    // 1. BeginPlayイベントを検索または作成
    UK2Node_Event* BeginPlayNode = nullptr;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName("ReceiveBeginPlay"))
        {
            BeginPlayNode = EventNode;
            NodeX = BeginPlayNode->NodePosX + 300;
            NodeY = BeginPlayNode->NodePosY;
            break;
        }
    }
    
    if (!BeginPlayNode)
    {
        // BeginPlayノード作成
        BeginPlayNode = FBlueprintEditorUtils::CreateDefaultEvent(
            Blueprint, FName("ReceiveBeginPlay"), true);
        if (!BeginPlayNode)
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create BeginPlay event"));
        NodeX = 300;
        NodeY = 0;
    }
    
    CreatedNodeIds.Add(BeginPlayNode->NodeGuid.ToString());
    
    // 2. GetController ノード
    UK2Node_CallFunction* GetControllerNode = NewObject<UK2Node_CallFunction>(EventGraph);
    GetControllerNode->SetFromFunction(APawn::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(APawn, GetController)));
    GetControllerNode->AllocateDefaultPins();
    GetControllerNode->NodePosX = NodeX;
    GetControllerNode->NodePosY = NodeY;
    EventGraph->AddNode(GetControllerNode, false, false);
    CreatedNodeIds.Add(GetControllerNode->NodeGuid.ToString());
    NodeX += 300;
    
    // 3. CastToPlayerController ノード
    UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(EventGraph);
    CastNode->TargetType = APlayerController::StaticClass();
    CastNode->AllocateDefaultPins();
    CastNode->NodePosX = NodeX;
    CastNode->NodePosY = NodeY;
    EventGraph->AddNode(CastNode, false, false);
    CreatedNodeIds.Add(CastNode->NodeGuid.ToString());
    NodeX += 300;
    
    // 4. GetEnhancedInputLocalPlayerSubsystem ノード
    // UEnhancedInputLocalPlayerSubsystem::Get(PlayerController)
    UK2Node_CallFunction* GetSubsystemNode = NewObject<UK2Node_CallFunction>(EventGraph);
    UFunction* GetSubsystemFunc = UEnhancedInputLocalPlayerSubsystem::StaticClass()->FindFunctionByName(TEXT("Get"));
    if (!GetSubsystemFunc)
    {
        // 静的メソッドを使用
        GetSubsystemNode->FunctionReference.SetExternalMember(
            FName("GetEnhancedInputLocalPlayerSubsystem"),
            UGameplayStatics::StaticClass());
    }
    else
    {
        GetSubsystemNode->SetFromFunction(GetSubsystemFunc);
    }
    GetSubsystemNode->AllocateDefaultPins();
    GetSubsystemNode->NodePosX = NodeX;
    GetSubsystemNode->NodePosY = NodeY;
    EventGraph->AddNode(GetSubsystemNode, false, false);
    CreatedNodeIds.Add(GetSubsystemNode->NodeGuid.ToString());
    NodeX += 300;
    
    // 5. AddMappingContext ノード
    UK2Node_CallFunction* AddContextNode = NewObject<UK2Node_CallFunction>(EventGraph);
    UFunction* AddContextFunc = UEnhancedInputLocalPlayerSubsystem::StaticClass()->FindFunctionByName(TEXT("AddMappingContext"));
    if (AddContextFunc)
    {
        AddContextNode->SetFromFunction(AddContextFunc);
    }
    else
    {
        AddContextNode->FunctionReference.SetExternalMember(
            FName("AddMappingContext"),
            UEnhancedInputLocalPlayerSubsystem::StaticClass());
    }
    AddContextNode->AllocateDefaultPins();
    AddContextNode->NodePosX = NodeX;
    AddContextNode->NodePosY = NodeY;
    EventGraph->AddNode(AddContextNode, false, false);
    CreatedNodeIds.Add(AddContextNode->NodeGuid.ToString());
    
    // ピン接続
    // BeginPlay -> GetController
    UEdGraphPin* BeginPlayExec = BeginPlayNode->FindPin(UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* GetControllerExec = GetControllerNode->FindPin(UEdGraphSchema_K2::PN_Execute);
    // ... 接続処理
    
    // IMCピンにデフォルト値設定
    UEdGraphPin* MappingContextPin = AddContextNode->FindPin(TEXT("MappingContext"));
    if (MappingContextPin)
    {
        MappingContextPin->DefaultObject = MappingContext;
    }
    
    // Priorityピン設定
    UEdGraphPin* PriorityPin = AddContextNode->FindPin(TEXT("Priority"));
    if (PriorityPin)
    {
        PriorityPin->DefaultValue = FString::FromInt(Priority);
    }
    
    // Blueprint コンパイル
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    
    // 結果
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    
    TArray<TSharedPtr<FJsonValue>> NodeIdsArray;
    for (const FString& NodeId : CreatedNodeIds)
    {
        NodeIdsArray.Add(MakeShared<FJsonValueString>(NodeId));
    }
    Result->SetArrayField(TEXT("nodes"), NodeIdsArray);
    Result->SetStringField(TEXT("message"), 
        FString::Printf(TEXT("Added %d nodes for AddMappingContext"), CreatedNodeIds.Num()));
    
    return Result;
}
```

### 4. SpirrowBridge.cpp の ExecuteCommand に追加

```cpp
else if (CommandType == TEXT("add_mapping_context_to_blueprint") ||
         CommandType == TEXT("set_default_mapping_context"))
{
    ResultJson = ProjectCommands->HandleCommand(CommandType, Params);
}
```

## テスト

### テスト手順
1. IA_TestとIMC_Testを作成
2. BP_TestCharacterを作成（Character継承）
3. add_mapping_context_to_blueprint を実行
4. Blueprint Editorで確認：BeginPlayにノードチェーンが追加されていること

### テストスクリプト
```python
# test_enhanced_input_integration.py
from unreal_mcp_server import get_unreal_connection

def test_add_mapping_context():
    unreal = get_unreal_connection()
    
    # 1. IMC作成
    unreal.send_command("create_input_mapping_context", {
        "context_name": "IMC_Test",
        "path": "/Game/Test"
    })
    
    # 2. IA作成
    unreal.send_command("create_input_action", {
        "action_name": "IA_TestJump",
        "value_type": "Digital",
        "path": "/Game/Test"
    })
    
    # 3. Blueprint作成
    unreal.send_command("create_blueprint", {
        "name": "BP_TestChar",
        "parent_class": "Character",
        "path": "/Game/Test"
    })
    
    # 4. AddMappingContext追加
    result = unreal.send_command("add_mapping_context_to_blueprint", {
        "blueprint_name": "BP_TestChar",
        "context_name": "IMC_Test",
        "priority": 0,
        "context_path": "/Game/Test",
        "path": "/Game/Test"
    })
    
    print(result)
```

## 補足事項

### 代替アプローチ: 既存ノードツールの組み合わせ

この機能は、既存のノードツールを順番に呼び出すPython側のみの実装も可能:
```python
# 1. BeginPlay検索/作成
# 2. add_blueprint_function_node(target="self", function_name="GetController")
# 3. add_blueprint_function_node(...)
# 4. connect_blueprint_nodes(...)
```

ただし、C++側で一括処理する方が:
- パフォーマンスが良い（単一リクエスト）
- エラーハンドリングが簡単
- アトミックな操作が保証される

### 今後の拡張
- `remove_mapping_context_from_blueprint`: 既存のノードチェーン削除
- `list_mapping_contexts_in_blueprint`: Blueprint内のIMC参照一覧
