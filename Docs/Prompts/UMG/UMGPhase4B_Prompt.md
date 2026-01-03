# UMG Phase 4-B: 追加インタラクティブ Widget 実装

## 概要

Phase 4-A（Button/Slider/CheckBox/EventBinding）が完了しました。
Phase 4-B では追加のインタラクティブ Widget を実装します。

---

## 現在の状況

### 完了済み（25ツール）

| Phase | ツール数 | 状態 |
|-------|---------|------|
| Phase 1: Designer操作 | 11 | ✅ |
| Phase 2: 変数・関数 | 5 | ✅ |
| Phase 3: Animation | 4 | ✅ |
| Phase 3: Array Variables | 1 | ✅ |
| Phase 4-A: Interactive Widgets | 4 | ✅ |

---

## Phase 4-B 実装対象

### 優先度: 高

| ツール | 説明 | 用途 |
|--------|------|------|
| `add_combobox_to_widget` | ドロップダウン選択 | 設定画面、オプション選択 |
| `add_editabletext_to_widget` | テキスト入力フィールド | プレイヤー名入力、チャット |

### 優先度: 中

| ツール | 説明 | 用途 |
|--------|------|------|
| `add_spinbox_to_widget` | 数値入力（+/-ボタン付き） | 数量選択、設定値 |
| `add_scrollbox_to_widget` | スクロール可能コンテナ | 長いリスト、ログ表示 |

### 優先度: 低（将来検討）

| ツール | 説明 | 用途 |
|--------|------|------|
| `add_listview_to_widget` | リスト表示（動的生成） | インベントリ、ハイスコア |
| `add_tileview_to_widget` | タイル表示 | アイテムグリッド |

---

## 実装仕様

### 1. add_combobox_to_widget

**パラメータ**:
```python
def add_combobox_to_widget(
    widget_name: str,           # 対象 Widget Blueprint
    combobox_name: str,         # ComboBox 名
    options: List[str] = [],    # 選択肢リスト
    selected_index: int = 0,    # 初期選択インデックス
    font_size: int = 14,        # フォントサイズ
    size: List[float] = [200, 40],  # サイズ
    anchor: str = "Center",     # アンカー
    alignment: List[float] = [0.5, 0.5],  # アライメント
    path: str = "/Game/UI"      # パス
) -> Dict[str, Any]
```

**C++ 実装ポイント**:
- `UComboBoxString` を使用
- `AddOption()` で選択肢追加
- `SetSelectedIndex()` で初期選択

**インクルード**:
```cpp
#include "Components/ComboBoxString.h"
```

---

### 2. add_editabletext_to_widget

**パラメータ**:
```python
def add_editabletext_to_widget(
    widget_name: str,           # 対象 Widget Blueprint
    text_name: str,             # EditableText 名
    hint_text: str = "",        # プレースホルダーテキスト
    is_password: bool = False,  # パスワードモード
    is_multiline: bool = False, # 複数行対応
    font_size: int = 14,        # フォントサイズ
    text_color: List[float] = [1,1,1,1],  # テキスト色
    size: List[float] = [200, 40],  # サイズ
    anchor: str = "Center",     # アンカー
    alignment: List[float] = [0.5, 0.5],  # アライメント
    path: str = "/Game/UI"      # パス
) -> Dict[str, Any]
```

**C++ 実装ポイント**:
- `UEditableTextBox` を使用（単一行）
- `UMultiLineEditableTextBox` を使用（複数行）
- `SetHintText()` でプレースホルダー設定
- `SetIsPassword()` でパスワードモード

**インクルード**:
```cpp
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
```

---

### 3. add_spinbox_to_widget

**パラメータ**:
```python
def add_spinbox_to_widget(
    widget_name: str,           # 対象 Widget Blueprint
    spinbox_name: str,          # SpinBox 名
    value: float = 0.0,         # 初期値
    min_value: float = 0.0,     # 最小値
    max_value: float = 100.0,   # 最大値
    delta: float = 1.0,         # 増減単位
    size: List[float] = [150, 40],  # サイズ
    anchor: str = "Center",     # アンカー
    alignment: List[float] = [0.5, 0.5],  # アライメント
    path: str = "/Game/UI"      # パス
) -> Dict[str, Any]
```

**C++ 実装ポイント**:
- `USpinBox` を使用
- `SetMinValue()`, `SetMaxValue()` で範囲設定
- `SetDelta()` で増減単位設定

**インクルード**:
```cpp
#include "Components/SpinBox.h"
```

---

### 4. add_scrollbox_to_widget

**パラメータ**:
```python
def add_scrollbox_to_widget(
    widget_name: str,           # 対象 Widget Blueprint
    scrollbox_name: str,        # ScrollBox 名
    orientation: str = "Vertical",  # Vertical / Horizontal
    scroll_bar_visibility: str = "Visible",  # Visible / Hidden / Auto
    size: List[float] = [300, 200],  # サイズ
    anchor: str = "Center",     # アンカー
    alignment: List[float] = [0.5, 0.5],  # アライメント
    path: str = "/Game/UI"      # パス
) -> Dict[str, Any]
```

**C++ 実装ポイント**:
- `UScrollBox` を使用
- `SetOrientation()` でスクロール方向
- `SetScrollBarVisibility()` でスクロールバー表示

**インクルード**:
```cpp
#include "Components/ScrollBox.h"
```

---

## 実装手順

### Step 1: C++ 実装

**ファイル**: `SpirrowBridgeUMGCommands.cpp`

```cpp
// ヘッダーに追加
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ScrollBox.h"

// ハンドラ関数を追加
TSharedPtr<FJsonObject> HandleAddComboBoxToWidget(const TSharedPtr<FJsonObject>& Params);
TSharedPtr<FJsonObject> HandleAddEditableTextToWidget(const TSharedPtr<FJsonObject>& Params);
TSharedPtr<FJsonObject> HandleAddSpinBoxToWidget(const TSharedPtr<FJsonObject>& Params);
TSharedPtr<FJsonObject> HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params);
```

### Step 2: コマンドルーティング

**ファイル**: `SpirrowBridge.cpp` の UMG セクション

```cpp
// Phase 4-B: Additional Interactive Widgets
CommandType == TEXT("add_combobox_to_widget") ||
CommandType == TEXT("add_editabletext_to_widget") ||
CommandType == TEXT("add_spinbox_to_widget") ||
CommandType == TEXT("add_scrollbox_to_widget") ||
```

### Step 3: Python ツール

**ファイル**: `Python/tools/umg_tools.py`

4つのツール関数を追加。

---

## 参考: Phase 4-A 実装パターン

Phase 4-A で確立したパターンに従う:

1. **アンカーシステム**: 9ポジション対応（GetAnchorFromString 共通関数使用）
2. **Canvas Slot 設定**: 位置・サイズ・アライメント設定
3. **エラーハンドリング**: Widget/要素の存在チェック
4. **レスポンス形式**: `{"success": true, "widget_name": "...", ...}`

---

## テスト計画

### テスト用 Widget
既存の `WBP_Phase4ATest` または新規作成

### テストケース

```python
# 1. ComboBox テスト
add_combobox_to_widget(
    widget_name="WBP_Phase4BTest",
    combobox_name="DifficultyCombo",
    options=["Easy", "Normal", "Hard"],
    selected_index=1,
    path="/Game/TrapxTrap/UI"
)

# 2. EditableText テスト
add_editabletext_to_widget(
    widget_name="WBP_Phase4BTest",
    text_name="PlayerNameInput",
    hint_text="Enter your name...",
    path="/Game/TrapxTrap/UI"
)

# 3. SpinBox テスト
add_spinbox_to_widget(
    widget_name="WBP_Phase4BTest",
    spinbox_name="VolumeSpinBox",
    value=50,
    min_value=0,
    max_value=100,
    path="/Game/TrapxTrap/UI"
)

# 4. ScrollBox テスト
add_scrollbox_to_widget(
    widget_name="WBP_Phase4BTest",
    scrollbox_name="LogScrollBox",
    orientation="Vertical",
    path="/Game/TrapxTrap/UI"
)
```

---

## ファイル一覧

### 変更対象

| ファイル | 変更内容 |
|----------|----------|
| `SpirrowBridgeUMGCommands.h` | 4ハンドラ宣言追加 |
| `SpirrowBridgeUMGCommands.cpp` | 4ハンドラ実装、インクルード追加 |
| `SpirrowBridge.cpp` | Phase 4-B コマンドルーティング追加 |
| `Python/tools/umg_tools.py` | 4ツール追加 |

---

## 完了条件

- [ ] C++ ハンドラ実装（4つ）
- [ ] コマンドルーティング追加
- [ ] Python ツール実装（4つ）
- [ ] ビルド成功
- [ ] 全ツール動作確認

---

**作成日**: 2025-12-25
**対象フェーズ**: UMG Phase 4-B
**ステータス**: 実装待ち
