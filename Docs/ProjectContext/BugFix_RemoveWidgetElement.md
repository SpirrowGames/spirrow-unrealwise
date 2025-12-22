# バグ修正完了: remove_widget_element の完全削除問題

## 修正日
2025-12-22

## 問題概要

`remove_widget_element` ツールが成功を返すが、Widget が WidgetTree から完全に削除されない不具合を修正。

### 症状
- 親からは RemoveChild で外れる（parent が null になる）
- しかし `WidgetTree->GetAllWidgets()` で取得される要素リストには残る
- 結果として `get_widget_elements` で削除したはずの要素が表示される

## 原因

以下の問題があった:

1. **Modify() の呼び出しタイミングが遅い**
   - 変更後に Modify() を呼んでいたため、変更トラッキングが正しく動作していない可能性

2. **RemoveWidget の戻り値未確認**
   - 失敗していても検知できていない

3. **ガベージコレクション未指示**
   - Widget オブジェクトの破棄が確実でない

4. **削除後の検証なし**
   - 実際に削除されたか確認していない

## 修正内容

### 変更前のコード

```cpp
// Remove from parent
UPanelWidget* Parent = Element->GetParent();
if (Parent)
{
    Parent->RemoveChild(Element);
}

// Remove from widget tree
WidgetTree->RemoveWidget(Element);

// Mark as modified and compile
WidgetBP->Modify();
WidgetBP->MarkPackageDirty();
FKismetEditorUtilities::CompileBlueprint(WidgetBP);
```

### 変更後のコード

```cpp
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

// Remove from widget tree and check result
bool bRemoved = WidgetTree->RemoveWidget(Element);

if (!bRemoved)
{
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
```

## 修正のポイント

### 1. Modify() を変更前に呼ぶ
Blueprint の変更トラッキングを正しく開始することで、Unreal Editor が変更を正しく認識する。

### 2. RemoveWidget の戻り値を確認
失敗時に警告ログを出力し、デバッグを容易にする。

### 3. ガベージコレクションヒント追加
- `Rename(nullptr, GetTransientPackage(), ...)` で Widget を一時パッケージに移動
- `MarkAsGarbage()` で確実に破棄予約
- これにより次回の GC 実行時に確実に削除される

### 4. 削除後の検証
`FindWidget` で再確認し、残っていればエラーを返すことで確実性を保証。

### 5. レスポンスに former_parent 追加
削除前の親情報を返すことでデバッグを容易にする。

## テスト結果

修正後のテスト:

```python
# 1. HorizontalBox を追加
add_horizontal_box_to_widget(
    widget_name="WBP_TT_TrapSelector",
    box_name="TestDelete",
    path="/Game/TrapxTrap/UI"
)

# 2. 要素確認（TestDelete が存在することを確認）
result = get_widget_elements("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")
# => TestDelete が要素リストに存在

# 3. 削除
remove_widget_element("WBP_TT_TrapSelector", "TestDelete", path="/Game/TrapxTrap/UI")
# => {"success": true, "removed_element": "TestDelete", "former_parent": "RootCanvas"}

# 4. 要素確認（TestDelete が消えていることを確認）
result = get_widget_elements("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")
# => TestDelete が要素リストから完全に削除されている ✅
```

## 影響範囲

### 変更ファイル
```
MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/
  SpirrowBridgeUMGCommands.cpp  (+22行, -6行)
```

### API変更
レスポンスに `former_parent` フィールドを追加（後方互換性あり）

```json
{
  "success": true,
  "widget_name": "WBP_TT_TrapSelector",
  "removed_element": "TestDelete",
  "former_parent": "RootCanvas"  // 新規追加
}
```

## 学んだこと

### UE Blueprint 編集における注意点

1. **変更前に Modify() を呼ぶのが重要**
   - Unreal の変更トラッキングシステムを正しく動作させるため

2. **Widget の完全削除には複数ステップが必要**
   - 親から RemoveChild
   - WidgetTree から RemoveWidget
   - Rename で一時パッケージに移動
   - MarkAsGarbage で破棄予約

3. **検証は必須**
   - UE の API は失敗を返さないことがあるため、削除後の検証が重要

## 関連ドキュメント

- [BugFix_RemoveWidgetElement_Prompt.md](../BugFix_RemoveWidgetElement_Prompt.md) - 詳細な問題分析と修正案
- [UMGPhase1_DesignerOperations_Prompt.md](../UMGPhase1_DesignerOperations_Prompt.md) - 元の実装仕様
