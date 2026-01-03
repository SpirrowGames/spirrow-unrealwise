# UMGCommands 分割リファクタリング実装プロンプト

## 概要

`SpirrowBridgeUMGCommands.cpp`（166 KB）を4つのファイルに分割し、SpirrowBridge.cpp で直接ルーティングする構造（オプションB）に変更する。

## 目的

- ファイルサイズを 40-60 KB/ファイルに抑える
- コード構造をフラット化し、全ハンドラを同列に扱う
- 将来の拡張性を確保

---

## 分割構成

### 新規作成ファイル（8ファイル）

| ファイル | 担当 | 推定サイズ |
|----------|------|-----------|
| `SpirrowBridgeUMGWidgetCommands.h/cpp` | Widget追加（12関数） | ~50 KB |
| `SpirrowBridgeUMGLayoutCommands.h/cpp` | レイアウト操作（7関数） | ~30 KB |
| `SpirrowBridgeUMGAnimationCommands.h/cpp` | アニメーション（4関数） | ~25 KB |
| `SpirrowBridgeUMGVariableCommands.h/cpp` | 変数・関数・バインディング（9関数） | ~40 KB |

### 削除ファイル（2ファイル）

- `SpirrowBridgeUMGCommands.h`
- `SpirrowBridgeUMGCommands.cpp`

### 変更ファイル（2ファイル）

- `SpirrowBridge.h` - 新しいハンドラのインクルードとメンバ変数
- `SpirrowBridge.cpp` - UMGコマンドのルーティングを4分割

---

## 関数配置詳細

### 1. SpirrowBridgeUMGWidgetCommands（Widget追加系）

**MCPコマンド → 関数**:
```
create_umg_widget_blueprint    → HandleCreateUMGWidgetBlueprint
add_text_to_widget             → HandleAddTextToWidget
add_text_block_to_widget       → HandleAddTextBlockToWidget (Legacy)
add_image_to_widget            → HandleAddImageToWidget
add_progressbar_to_widget      → HandleAddProgressBarToWidget
add_button_to_widget           → HandleAddButtonToWidgetV2
add_slider_to_widget           → HandleAddSliderToWidget
add_checkbox_to_widget         → HandleAddCheckBoxToWidget
add_combobox_to_widget         → HandleAddComboBoxToWidget
add_editabletext_to_widget     → HandleAddEditableTextToWidget
add_spinbox_to_widget          → HandleAddSpinBoxToWidget
add_scrollbox_to_widget        → HandleAddScrollBoxToWidget
add_widget_to_viewport         → HandleAddWidgetToViewport (Legacy)
```

**旧APIも含める**:
- `HandleAddButtonToWidget` (内部用、V2が外部API)

### 2. SpirrowBridgeUMGLayoutCommands（レイアウト系）

**MCPコマンド → 関数**:
```
add_vertical_box_to_widget     → HandleAddVerticalBoxToWidget
add_horizontal_box_to_widget   → HandleAddHorizontalBoxToWidget
get_widget_elements            → HandleGetWidgetElements
set_widget_slot_property       → HandleSetWidgetSlotProperty
set_widget_element_property    → HandleSetWidgetElementProperty
reparent_widget_element        → HandleReparentWidgetElement
remove_widget_element          → HandleRemoveWidgetElement
```

### 3. SpirrowBridgeUMGAnimationCommands（アニメーション系）

**MCPコマンド → 関数**:
```
create_widget_animation        → HandleCreateWidgetAnimation
add_animation_track            → HandleAddAnimationTrack
add_animation_keyframe         → HandleAddAnimationKeyframe
get_widget_animations          → HandleGetWidgetAnimations
```

### 4. SpirrowBridgeUMGVariableCommands（変数・関数・バインディング系）

**MCPコマンド → 関数**:
```
add_widget_variable            → HandleAddWidgetVariable
add_widget_array_variable      → HandleAddWidgetArrayVariable
set_widget_variable_default    → HandleSetWidgetVariableDefault
add_widget_function            → HandleAddWidgetFunction
add_widget_event               → HandleAddWidgetEvent
bind_widget_to_variable        → HandleBindWidgetToVariable
bind_widget_event              → HandleBindWidgetEvent (Legacy)
set_text_block_binding         → HandleSetTextBlockBinding (Legacy)
bind_widget_component_event    → HandleBindWidgetComponentEvent
```

**ヘルパー関数**:
- `SetupPinType` - ピン型設定（この関数のみ使用するため、このファイルに配置）

---

## 実装内容

### 1. SpirrowBridgeUMGWidgetCommands.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handles UMG Widget creation commands
 * Responsible for adding widget elements (Text, Image, Button, etc.)
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGWidgetCommands
{
public:
    FSpirrowBridgeUMGWidgetCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Core
    TSharedPtr<FJsonObject> HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params);

    // Basic Widgets
    TSharedPtr<FJsonObject> HandleAddTextToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddTextBlockToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params);

    // Interactive Widgets
    TSharedPtr<FJsonObject> HandleAddButtonToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddButtonToWidgetV2(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddComboBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddEditableTextToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddSpinBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params);
};
```

### 2. SpirrowBridgeUMGLayoutCommands.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handles UMG Layout and Designer operations
 * Responsible for layout containers and element manipulation
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGLayoutCommands
{
public:
    FSpirrowBridgeUMGLayoutCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Layout Containers
    TSharedPtr<FJsonObject> HandleAddVerticalBoxToWidget(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddHorizontalBoxToWidget(const TSharedPtr<FJsonObject>& Params);

    // Element Operations
    TSharedPtr<FJsonObject> HandleGetWidgetElements(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetWidgetSlotProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetWidgetElementProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleReparentWidgetElement(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRemoveWidgetElement(const TSharedPtr<FJsonObject>& Params);
};
```

### 3. SpirrowBridgeUMGAnimationCommands.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handles UMG Widget Animation operations
 * Responsible for creating and configuring widget animations
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGAnimationCommands
{
public:
    FSpirrowBridgeUMGAnimationCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateWidgetAnimation(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddAnimationKeyframe(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params);
};
```

### 4. SpirrowBridgeUMGVariableCommands.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Json.h"
#include "EdGraphSchema_K2.h"

/**
 * Handles UMG Widget Variable, Function, and Binding operations
 * Responsible for Blueprint-side widget logic
 */
class SPIRROWBRIDGE_API FSpirrowBridgeUMGVariableCommands
{
public:
    FSpirrowBridgeUMGVariableCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Variables
    TSharedPtr<FJsonObject> HandleAddWidgetVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetArrayVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetWidgetVariableDefault(const TSharedPtr<FJsonObject>& Params);

    // Functions & Events
    TSharedPtr<FJsonObject> HandleAddWidgetFunction(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddWidgetEvent(const TSharedPtr<FJsonObject>& Params);

    // Bindings
    TSharedPtr<FJsonObject> HandleBindWidgetToVariable(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleBindWidgetEvent(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params);

    // Helper
    bool SetupPinType(const FString& TypeName, FEdGraphPinType& OutPinType);
};
```

### 5. SpirrowBridge.h の変更

```cpp
// 削除
#include "Commands/SpirrowBridgeUMGCommands.h"

// 追加
#include "Commands/SpirrowBridgeUMGWidgetCommands.h"
#include "Commands/SpirrowBridgeUMGLayoutCommands.h"
#include "Commands/SpirrowBridgeUMGAnimationCommands.h"
#include "Commands/SpirrowBridgeUMGVariableCommands.h"

// メンバ変数変更
// 削除
TSharedPtr<FSpirrowBridgeUMGCommands> UMGCommands;

// 追加
TSharedPtr<FSpirrowBridgeUMGWidgetCommands> UMGWidgetCommands;
TSharedPtr<FSpirrowBridgeUMGLayoutCommands> UMGLayoutCommands;
TSharedPtr<FSpirrowBridgeUMGAnimationCommands> UMGAnimationCommands;
TSharedPtr<FSpirrowBridgeUMGVariableCommands> UMGVariableCommands;
```

### 6. SpirrowBridge.cpp の変更

**コンストラクタ**:
```cpp
// 削除
UMGCommands = MakeShared<FSpirrowBridgeUMGCommands>();

// 追加
UMGWidgetCommands = MakeShared<FSpirrowBridgeUMGWidgetCommands>();
UMGLayoutCommands = MakeShared<FSpirrowBridgeUMGLayoutCommands>();
UMGAnimationCommands = MakeShared<FSpirrowBridgeUMGAnimationCommands>();
UMGVariableCommands = MakeShared<FSpirrowBridgeUMGVariableCommands>();
```

**ExecuteCommand() ルーティング**:
```cpp
// 削除: 既存のUMGコマンド判定ブロック全体

// 追加: 4つの分割ブロック

// UMG Widget Commands
else if (CommandType == TEXT("create_umg_widget_blueprint") ||
         CommandType == TEXT("add_text_to_widget") ||
         CommandType == TEXT("add_text_block_to_widget") ||
         CommandType == TEXT("add_image_to_widget") ||
         CommandType == TEXT("add_progressbar_to_widget") ||
         CommandType == TEXT("add_button_to_widget") ||
         CommandType == TEXT("add_button_to_widget_v2") ||
         CommandType == TEXT("add_slider_to_widget") ||
         CommandType == TEXT("add_checkbox_to_widget") ||
         CommandType == TEXT("add_combobox_to_widget") ||
         CommandType == TEXT("add_editabletext_to_widget") ||
         CommandType == TEXT("add_spinbox_to_widget") ||
         CommandType == TEXT("add_scrollbox_to_widget") ||
         CommandType == TEXT("add_widget_to_viewport"))
{
    ResultJson = UMGWidgetCommands->HandleCommand(CommandType, Params);
}
// UMG Layout Commands
else if (CommandType == TEXT("add_vertical_box_to_widget") ||
         CommandType == TEXT("add_horizontal_box_to_widget") ||
         CommandType == TEXT("get_widget_elements") ||
         CommandType == TEXT("set_widget_slot_property") ||
         CommandType == TEXT("set_widget_element_property") ||
         CommandType == TEXT("reparent_widget_element") ||
         CommandType == TEXT("remove_widget_element"))
{
    ResultJson = UMGLayoutCommands->HandleCommand(CommandType, Params);
}
// UMG Animation Commands
else if (CommandType == TEXT("create_widget_animation") ||
         CommandType == TEXT("add_animation_track") ||
         CommandType == TEXT("add_animation_keyframe") ||
         CommandType == TEXT("get_widget_animations"))
{
    ResultJson = UMGAnimationCommands->HandleCommand(CommandType, Params);
}
// UMG Variable Commands
else if (CommandType == TEXT("add_widget_variable") ||
         CommandType == TEXT("add_widget_array_variable") ||
         CommandType == TEXT("set_widget_variable_default") ||
         CommandType == TEXT("add_widget_function") ||
         CommandType == TEXT("add_widget_event") ||
         CommandType == TEXT("bind_widget_to_variable") ||
         CommandType == TEXT("bind_widget_event") ||
         CommandType == TEXT("set_text_block_binding") ||
         CommandType == TEXT("bind_widget_component_event"))
{
    ResultJson = UMGVariableCommands->HandleCommand(CommandType, Params);
}
```

---

## 各 .cpp ファイルの実装手順

### 共通インクルード

各 .cpp ファイルの先頭に必要なインクルードを追加。元の `SpirrowBridgeUMGCommands.cpp` から該当関数が使用しているインクルードを抽出する。

**SpirrowBridgeUMGWidgetCommands.cpp の例**:
```cpp
#include "Commands/SpirrowBridgeUMGWidgetCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ScrollBox.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
// ... 必要に応じて追加
```

### HandleCommand() の実装パターン

各ファイルで同様のパターン:

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleCommand(
    const FString& CommandType, 
    const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_umg_widget_blueprint"))
    {
        return HandleCreateUMGWidgetBlueprint(Params);
    }
    else if (CommandType == TEXT("add_text_to_widget"))
    {
        return HandleAddTextToWidget(Params);
    }
    // ... 他のコマンド

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Unknown UMG Widget command: %s"), *CommandType));
}
```

### 関数本体の移動

元の `SpirrowBridgeUMGCommands.cpp` から該当関数をコピー。クラス名のプレフィックスを変更:

```cpp
// 変更前
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddTextToWidget(...)

// 変更後
TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddTextToWidget(...)
```

---

## SpirrowBridge.Build.cs の確認

新しいファイルが自動的にビルドに含まれることを確認。通常、同一フォルダ内の .cpp は自動検出されるが、念のため確認。

---

## テスト手順

### 1. ビルド確認
```
1. UE エディタを閉じる
2. Visual Studio で Rebuild Solution
3. エラーがないことを確認
```

### 2. 全UMGコマンドの動作確認

各カテゴリから最低1つずつテスト:

**Widget系**:
```python
create_umg_widget_blueprint(widget_name="WBP_Test", path="/Game/Test")
add_text_to_widget(widget_name="WBP_Test", text_name="TestText", text="Hello")
```

**Layout系**:
```python
add_vertical_box_to_widget(widget_name="WBP_Test", box_name="VBox")
get_widget_elements(widget_name="WBP_Test")
```

**Animation系**:
```python
create_widget_animation(widget_name="WBP_Test", animation_name="FadeIn")
get_widget_animations(widget_name="WBP_Test")
```

**Variable系**:
```python
add_widget_variable(widget_name="WBP_Test", variable_name="TestVar", variable_type="Float")
```

### 3. 既存機能への影響確認

他のコマンドカテゴリ（Blueprint, Editor, GAS等）が正常に動作することを確認。

---

## ドキュメント更新

リファクタリング完了後、以下を更新:

1. **Docs/IMPLEMENTATION_SUMMARY.md**
   - ファイル構成の更新
   - 関数配置の更新

2. **FEATURE_STATUS.md**
   - 更新履歴に記載

3. **AGENTS.md**（必要に応じて）
   - アーキテクチャ概要の更新

---

## チェックリスト

### 実装前
- [ ] 元の SpirrowBridgeUMGCommands.cpp をバックアップ
- [ ] 全UMGコマンド一覧を確認（29コマンド）

### 実装中
- [ ] SpirrowBridgeUMGWidgetCommands.h/cpp 作成
- [ ] SpirrowBridgeUMGLayoutCommands.h/cpp 作成
- [ ] SpirrowBridgeUMGAnimationCommands.h/cpp 作成
- [ ] SpirrowBridgeUMGVariableCommands.h/cpp 作成
- [ ] SpirrowBridge.h 更新
- [ ] SpirrowBridge.cpp 更新
- [ ] SpirrowBridgeUMGCommands.h/cpp 削除

### 実装後
- [ ] ビルド成功
- [ ] Widget系コマンド動作確認
- [ ] Layout系コマンド動作確認
- [ ] Animation系コマンド動作確認
- [ ] Variable系コマンド動作確認
- [ ] 他カテゴリのコマンド動作確認
- [ ] ドキュメント更新

---

## 補足事項

### インクルード依存関係

`SpirrowBridgeUMGVariableCommands` は `FEdGraphPinType` を使用するため、`EdGraphSchema_K2.h` のインクルードが必要。他のファイルでは不要な場合、インクルードを最小限に保つ。

### 共通ヘルパーの扱い

`SetupPinType` は現在 `SpirrowBridgeUMGVariableCommands` のみで使用。将来的に他でも使う場合は `SpirrowBridgeCommonUtils` への移動を検討。

### Legacy API の扱い

`add_text_block_to_widget`, `bind_widget_event` 等の旧APIは後方互換性のため残す。将来的に非推奨警告を追加する可能性あり。
