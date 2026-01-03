# UMG Phase 4-A: 修正と不要コード削除

## 概要

Phase 4-A の実装は完了していますが、以下の問題があります：

1. **SpirrowBridge.cpp のルーティング不足** - Phase 4-A コマンドがルーティングされていない
2. **Python 側の重複ツール定義** - 旧 API と新 API が混在

このドキュメントに従って修正を行ってください。

---

## 1. SpirrowBridge.cpp の修正（最重要）

### ファイル
`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/SpirrowBridge.cpp`

### 問題
UMG Commands のルーティングに Phase 4-A のコマンドが追加されていない。

### 修正箇所
`ExecuteCommand` 関数内の UMG Commands セクション（約 340-370 行目付近）を以下のように修正：

**現在のコード:**
```cpp
// UMG Commands
else if (CommandType == TEXT("create_umg_widget_blueprint") ||
         CommandType == TEXT("add_text_block_to_widget") ||
         CommandType == TEXT("add_button_to_widget") ||
         CommandType == TEXT("bind_widget_event") ||
         CommandType == TEXT("set_text_block_binding") ||
         CommandType == TEXT("add_widget_to_viewport") ||
         CommandType == TEXT("add_text_to_widget") ||
         CommandType == TEXT("add_image_to_widget") ||
         CommandType == TEXT("add_progressbar_to_widget") ||
         // Phase 1: Designer Operations
         CommandType == TEXT("get_widget_elements") ||
         CommandType == TEXT("set_widget_slot_property") ||
         CommandType == TEXT("set_widget_element_property") ||
         CommandType == TEXT("add_vertical_box_to_widget") ||
         CommandType == TEXT("add_horizontal_box_to_widget") ||
         CommandType == TEXT("reparent_widget_element") ||
         CommandType == TEXT("remove_widget_element") ||
         // Phase 2: Variable & Function Operations
         CommandType == TEXT("add_widget_variable") ||
         CommandType == TEXT("set_widget_variable_default") ||
         CommandType == TEXT("add_widget_function") ||
         CommandType == TEXT("add_widget_event") ||
         CommandType == TEXT("bind_widget_to_variable") ||
         // Phase 3: Animation Operations
         CommandType == TEXT("create_widget_animation") ||
         CommandType == TEXT("add_animation_track") ||
         CommandType == TEXT("add_animation_keyframe") ||
         CommandType == TEXT("get_widget_animations") ||
         // Phase 3: Array Variable Operations
         CommandType == TEXT("add_widget_array_variable"))
```

**修正後のコード:**
```cpp
// UMG Commands
else if (CommandType == TEXT("create_umg_widget_blueprint") ||
         CommandType == TEXT("add_text_block_to_widget") ||
         CommandType == TEXT("add_button_to_widget") ||
         CommandType == TEXT("bind_widget_event") ||
         CommandType == TEXT("set_text_block_binding") ||
         CommandType == TEXT("add_widget_to_viewport") ||
         CommandType == TEXT("add_text_to_widget") ||
         CommandType == TEXT("add_image_to_widget") ||
         CommandType == TEXT("add_progressbar_to_widget") ||
         // Phase 1: Designer Operations
         CommandType == TEXT("get_widget_elements") ||
         CommandType == TEXT("set_widget_slot_property") ||
         CommandType == TEXT("set_widget_element_property") ||
         CommandType == TEXT("add_vertical_box_to_widget") ||
         CommandType == TEXT("add_horizontal_box_to_widget") ||
         CommandType == TEXT("reparent_widget_element") ||
         CommandType == TEXT("remove_widget_element") ||
         // Phase 2: Variable & Function Operations
         CommandType == TEXT("add_widget_variable") ||
         CommandType == TEXT("set_widget_variable_default") ||
         CommandType == TEXT("add_widget_function") ||
         CommandType == TEXT("add_widget_event") ||
         CommandType == TEXT("bind_widget_to_variable") ||
         // Phase 3: Animation Operations
         CommandType == TEXT("create_widget_animation") ||
         CommandType == TEXT("add_animation_track") ||
         CommandType == TEXT("add_animation_keyframe") ||
         CommandType == TEXT("get_widget_animations") ||
         CommandType == TEXT("add_widget_array_variable") ||
         // Phase 4-A: Interactive Widgets
         CommandType == TEXT("add_button_to_widget_v2") ||
         CommandType == TEXT("bind_widget_component_event") ||
         CommandType == TEXT("add_slider_to_widget") ||
         CommandType == TEXT("add_checkbox_to_widget"))
```

---

## 2. Python umg_tools.py の重複コード削除

### ファイル
`Python/tools/umg_tools.py`

### 問題
`add_button_to_widget` が2回定義されている：
1. 旧 API（position ベース、コマンド名 `add_button_to_widget`）
2. 新 API（anchor/alignment ベース、コマンド名 `add_button_to_widget_v2`）

### 修正内容

**削除すべき旧 API 定義（約 120-180 行目）:**

以下の関数定義を **完全に削除** してください：

```python
    @mcp.tool()
    def add_button_to_widget(
        ctx: Context,
        widget_name: str,
        button_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0],
        font_size: int = 12,
        color: List[float] = [1.0, 1.0, 1.0, 1.0],
        background_color: List[float] = [0.1, 0.1, 0.1, 1.0]
    ) -> Dict[str, Any]:
        """
        Add a Button widget to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            button_name: Name to give the new Button
            text: Text to display on the button
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the button
            font_size: Font size for button text
            color: [R, G, B, A] text color values (0.0 to 1.0)
            background_color: [R, G, B, A] button background color values (0.0 to 1.0)
            
        Returns:
            Dict containing success status and button properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "button_name": button_name,
                "text": text,
                "position": position,
                "size": size,
                "font_size": font_size,
                "color": color,
                "background_color": background_color
            }
            
            logger.info(f"Adding Button to widget with params: {params}")
            response = unreal.send_command("add_button_to_widget", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add Button to widget response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding Button to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
```

**新 API は残す（ファイル末尾付近）:**

新しい `add_button_to_widget` 関数（`add_button_to_widget_v2` コマンドを使用するもの）はそのまま残してください。この関数は以下のシグネチャを持ちます：

```python
@mcp.tool()
def add_button_to_widget(
    ctx: Context,
    widget_name: str,
    button_name: str,
    text: str = "",
    font_size: int = 14,
    text_color: List[float] = [1.0, 1.0, 1.0, 1.0],
    normal_color: List[float] = [0.1, 0.1, 0.1, 1.0],
    hovered_color: List[float] = [0.2, 0.2, 0.2, 1.0],
    pressed_color: List[float] = [0.05, 0.05, 0.05, 1.0],
    size: List[float] = [200.0, 50.0],
    anchor: str = "Center",
    alignment: List[float] = [0.5, 0.5],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
```

---

## 3. 確認チェックリスト

修正完了後、以下を確認してください：

### C++ ビルド確認
- [ ] ビルドエラーがないこと
- [ ] 警告が増えていないこと

### Python 確認
- [ ] `add_button_to_widget` が1つだけ定義されていること
- [ ] 新 API（anchor/alignment/path 対応）が残っていること

### 機能テスト
修正後、以下のコマンドが正常に動作することを確認：

```python
# 1. Slider テスト
add_slider_to_widget(
    widget_name="WBP_TestMenu",
    slider_name="VolumeSlider",
    value=0.5,
    path="/Game/TrapxTrap/UI"
)

# 2. CheckBox テスト
add_checkbox_to_widget(
    widget_name="WBP_TestMenu",
    checkbox_name="MuteCheckbox",
    label_text="Mute Audio",
    path="/Game/TrapxTrap/UI"
)

# 3. Button テスト（新 API）
add_button_to_widget(
    widget_name="WBP_TestMenu",
    button_name="StartButton",
    text="Start Game",
    anchor="Center",
    path="/Game/TrapxTrap/UI"
)
```

---

## 4. 追加で削除可能な不要コード（オプション）

以下は旧 API で使われていた C++ 側の関数です。新 API に移行完了後、将来的に削除を検討できます：

### SpirrowBridgeUMGCommands.cpp

**旧 API 関数（削除可能）:**
- `HandleAddButtonToWidget` - 旧 Button 追加（blueprint_name ベース）
- `HandleBindWidgetEvent` - 旧イベントバインディング（/Game/Widgets/ ハードコード）
- `HandleSetTextBlockBinding` - 使用頻度低い

**ただし、今回は削除不要。** 既存の機能との互換性のために残しておくことを推奨。

---

## まとめ

### 必須修正（今回実施）:
1. ✅ SpirrowBridge.cpp に Phase 4-A コマンドルーティング追加
2. ✅ umg_tools.py の旧 `add_button_to_widget` 定義を削除

### オプション（将来検討）:
- 旧 API の C++ 実装削除

---

**作成日**: 2025-12-25
**対象フェーズ**: UMG Phase 4-A 修正
**ステータス**: 修正待ち
