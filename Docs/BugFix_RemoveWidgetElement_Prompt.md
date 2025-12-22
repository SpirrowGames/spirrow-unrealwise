# 不具合修正: remove_widget_element が WidgetTree から完全削除されない

## 問題

`remove_widget_element` ツールが成功を返すが、Widget が WidgetTree から完全に削除されない。

### 症状

```python
# 削除実行
remove_widget_element("WBP_TT_TrapSelector", "TestHBox", path="/Game/TrapxTrap/UI")
# => {"success": true, "removed_element": "TestHBox"}

# 確認
get_widget_elements("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")
# => TestHBox がまだリストに存在（parent: null, children に残る）
```

### 現象

- 親からは RemoveChild で外れている（parent が null になる）
- しかし WidgetTree->GetAllWidgets() で取得される要素リストには残っている
- つまり `WidgetTree->RemoveWidget()` が正しく動作していない可能性

## 原因調査

現在の実装（SpirrowBridgeUMGCommands.cpp）：

```cpp
// Remove from parent
UPanelWidget* Parent = Element->GetParent();
if (Parent)
{
    Parent->RemoveChild(Element);
}

// Remove from widget tree
WidgetTree->RemoveWidget(Element);
```

### 考えられる原因

1. **RemoveWidget の呼び出しタイミング**
   - RemoveChild 後に Widget がまだ参照されている可能性

2. **RemoveWidget の戻り値未確認**
   - 失敗している可能性があるが検知していない

3. **WidgetTree の内部状態との不整合**
   - Blueprint の Modify/MarkPackageDirty のタイミング問題

## 修正案

### 修正コード

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleRemoveWidgetElement(const TSharedPtr<FJsonObject>& Params)
{
    // ... パラメータ取得部分は同じ ...

    // Find the element to remove
    UWidget* Element = WidgetTree->FindWidget(FName(*ElementName));
    if (!Element)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
    }

    // Cannot remove root widget
    if (Element == WidgetTree->RootWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Cannot remove root widget"));
    }

    // Mark blueprint as modified BEFORE making changes
    WidgetBP->Modify();

    // Get parent for removal
    UPanelWidget* Parent = Element->GetParent();
    FString ParentName = Parent ? Parent->GetName() : TEXT("None");

    // Remove from parent first
    if (Parent)
    {
        Parent->RemoveChild(Element);
    }

    // Use ForceRemoveWidget for complete removal (UE 5.x)
    // This handles internal cleanup better than RemoveWidget
    bool bRemoved = WidgetTree->RemoveWidget(Element);
    
    if (!bRemoved)
    {
        // Alternative: Try direct removal from AllWidgets array
        // WidgetTree の内部実装によっては ForceRemoveWidget が必要な場合がある
        UE_LOG(LogTemp, Warning, TEXT("RemoveWidget returned false for '%s'"), *ElementName);
    }

    // Force garbage collection hint
    Element->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
    Element->MarkAsGarbage();

    // Mark package dirty and recompile
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Verify removal
    UWidget* VerifyWidget = WidgetTree->FindWidget(FName(*ElementName));
    if (VerifyWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to completely remove widget '%s' from WidgetTree"), *ElementName));
    }

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("removed_element"), ElementName);
    ResultObj->SetStringField(TEXT("former_parent"), ParentName);
    return ResultObj;
}
```

### 修正のポイント

1. **Modify() を変更前に呼ぶ**
   - Blueprint の変更トラッキングを正しく開始

2. **RemoveWidget の戻り値を確認**
   - false なら警告ログ出力

3. **ガベージコレクションヒント追加**
   - `Rename` + `MarkAsGarbage` で確実に破棄

4. **削除後の検証**
   - FindWidget で再確認し、残っていればエラー

## テスト手順

1. SpirrowBridgeUMGCommands.cpp を修正
2. ビルド
3. テスト実行：

```python
# HorizontalBox を追加
add_horizontal_box_to_widget(
    widget_name="WBP_TT_TrapSelector",
    box_name="TestDelete",
    path="/Game/TrapxTrap/UI"
)

# 要素確認（TestDelete が存在することを確認）
get_widget_elements("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")

# 削除
remove_widget_element("WBP_TT_TrapSelector", "TestDelete", path="/Game/TrapxTrap/UI")

# 要素確認（TestDelete が消えていることを確認）
get_widget_elements("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")
```

## 対象ファイル

```
spirrow-unrealwise/MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGCommands.cpp
```

## 修正日

2025-12-22
