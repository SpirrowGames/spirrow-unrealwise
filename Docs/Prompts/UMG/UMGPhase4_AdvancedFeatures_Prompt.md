# UMG Phase 4: 高度な機能（イベントバインド・アニメーション）

## 概要

Phase 3（Graph ノード操作）完了後、Widget BP の高度な機能を MCP 経由で操作可能にする。

- イベントバインド（Button OnClicked 等）
- Widget アニメーション
- Timer 操作

## 前提

- Phase 1: Designer 操作完全化 ✅
- Phase 2: 変数・関数操作 ✅
- Phase 3: Graph ノード操作 ✅
- 本 Phase: 高度な機能

---

## 実装するツール

### 4.1 イベントバインド

| # | ツール名 | 機能 | 優先度 |
|---|----------|------|--------|
| 1 | `bind_widget_element_event` | Widget 要素のイベントをバインド | 高 |
| 2 | `get_widget_element_events` | Widget 要素の利用可能なイベント一覧 | 中 |

### 4.2 アニメーション

| # | ツール名 | 機能 | 優先度 |
|---|----------|------|--------|
| 3 | `create_widget_animation` | Widget アニメーション作成 | 高 |
| 4 | `add_animation_track` | アニメーショントラック追加 | 高 |
| 5 | `add_animation_keyframe` | キーフレーム追加 | 高 |
| 6 | `add_play_animation_node` | PlayAnimation ノード追加 | 中 |

### 4.3 Timer 操作

| # | ツール名 | 機能 | 優先度 |
|---|----------|------|--------|
| 7 | `add_set_timer_node` | SetTimerByFunctionName ノード追加 | 高 |
| 8 | `add_clear_timer_node` | ClearTimer ノード追加 | 中 |

---

## 1. bind_widget_element_event

### Python 側

```python
@mcp.tool()
def bind_widget_element_event(
    ctx: Context,
    widget_name: str,
    element_name: str,            # Button 等の Widget 要素名
    event_name: str,              # "OnClicked", "OnPressed", "OnReleased", etc.
    function_name: str,           # バインド先の関数名（自動作成 or 既存）
    create_function: bool = True, # 関数が無ければ作成するか
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Bind a widget element's event to a function.

    Args:
        widget_name: Name of the Widget Blueprint
        element_name: Name of the widget element (e.g., Button)
        event_name: Name of the event to bind
        function_name: Name of the function to bind to
        create_function: Whether to create the function if it doesn't exist
        path: Content browser path to the widget

    Returns:
        Dict containing success status and binding info

    Supported events by widget type:
        Button:
            - OnClicked
            - OnPressed
            - OnReleased
            - OnHovered
            - OnUnhovered
        
        CheckBox:
            - OnCheckStateChanged
        
        Slider:
            - OnValueChanged
        
        EditableText:
            - OnTextChanged
            - OnTextCommitted

    Example:
        bind_widget_element_event(
            widget_name="WBP_MainMenu",
            element_name="StartButton",
            event_name="OnClicked",
            function_name="OnStartButtonClicked",
            path="/Game/UI"
        )
    """
```

### C++ 実装ポイント

```cpp
// 1. Widget 要素を取得
UWidget* Element = WidgetTree->FindWidget(FName(*ElementName));

// 2. Button の場合
if (UButton* Button = Cast<UButton>(Element))
{
    // OnClicked イベントをバインド
    // FScriptDelegate を作成して Button->OnClicked に追加
    
    // または、Blueprint グラフにイベントノードを追加
    // UK2Node_ComponentBoundEvent を使用
}
```

---

## 2. get_widget_element_events

### Python 側

```python
@mcp.tool()
def get_widget_element_events(
    ctx: Context,
    widget_name: str,
    element_name: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Get available events for a widget element.

    Returns:
        Dict containing list of bindable events

    Example response:
        {
            "success": true,
            "element_name": "StartButton",
            "element_type": "Button",
            "events": [
                {"name": "OnClicked", "signature": "void()"},
                {"name": "OnPressed", "signature": "void()"},
                {"name": "OnReleased", "signature": "void()"},
                {"name": "OnHovered", "signature": "void()"},
                {"name": "OnUnhovered", "signature": "void()"}
            ]
        }
    """
```

---

## 3. create_widget_animation

### Python 側

```python
@mcp.tool()
def create_widget_animation(
    ctx: Context,
    widget_name: str,
    animation_name: str,
    length: float = 1.0,          # アニメーション長さ（秒）
    loop: bool = False,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Create a Widget Animation in a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        animation_name: Name for the new animation
        length: Animation length in seconds
        loop: Whether the animation should loop
        path: Content browser path to the widget

    Returns:
        Dict containing success status and animation info

    Example:
        create_widget_animation(
            widget_name="WBP_TT_TrapSelector",
            animation_name="FadeIn",
            length=0.3,
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装ポイント

```cpp
// UWidgetBlueprint から UWidgetAnimation を作成
UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(WidgetBP, FName(*AnimationName));
Animation->MovieScene = NewObject<UMovieScene>(Animation);
Animation->MovieScene->SetPlaybackRange(FFrameNumber(0), FFrameNumber(Length * FrameRate));

// WidgetBP に追加
WidgetBP->Animations.Add(Animation);
```

---

## 4. add_animation_track

### Python 側

```python
@mcp.tool()
def add_animation_track(
    ctx: Context,
    widget_name: str,
    animation_name: str,
    element_name: str,            # アニメーション対象の Widget 要素
    property_name: str,           # アニメーションするプロパティ
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add an animation track for a widget element's property.

    Args:
        widget_name: Name of the Widget Blueprint
        animation_name: Name of the animation
        element_name: Name of the widget element to animate
        property_name: Name of the property to animate
        path: Content browser path to the widget

    Returns:
        Dict containing track_id and success status

    Supported properties:
        - RenderOpacity (float 0.0-1.0)
        - RenderTransform.Translation (Vector2D)
        - RenderTransform.Scale (Vector2D)
        - RenderTransform.Angle (float)
        - ColorAndOpacity (LinearColor)
        - Visibility (enum - not smooth, use RenderOpacity instead)

    Example:
        # Fade animation track
        add_animation_track(
            widget_name="WBP_TT_TrapSelector",
            animation_name="FadeIn",
            element_name="MainContainer",
            property_name="RenderOpacity",
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装ポイント

```cpp
// MovieScene にトラックを追加
// UMovieSceneFloatTrack (RenderOpacity等)
// UMovieScene2DTransformTrack (Transform)
```

---

## 5. add_animation_keyframe

### Python 側

```python
@mcp.tool()
def add_animation_keyframe(
    ctx: Context,
    widget_name: str,
    animation_name: str,
    track_id: str,                # add_animation_track で返された ID
    time: float,                  # キーフレーム時間（秒）
    value: Any,                   # プロパティ値
    interpolation: str = "Linear", # "Linear", "Cubic", "Constant"
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a keyframe to an animation track.

    Args:
        widget_name: Name of the Widget Blueprint
        animation_name: Name of the animation
        track_id: ID of the track
        time: Time in seconds for the keyframe
        value: Value at this keyframe
        interpolation: Interpolation type
        path: Content browser path to the widget

    Returns:
        Dict containing success status

    Example:
        # Fade from 0 to 1 over 0.3 seconds
        add_animation_keyframe(
            widget_name="WBP_TT_TrapSelector",
            animation_name="FadeIn",
            track_id="track_0",
            time=0.0,
            value=0.0,
            interpolation="Linear"
        )
        add_animation_keyframe(
            widget_name="WBP_TT_TrapSelector",
            animation_name="FadeIn",
            track_id="track_0",
            time=0.3,
            value=1.0,
            interpolation="Linear"
        )
    """
```

---

## 6. add_play_animation_node

### Python 側

```python
@mcp.tool()
def add_play_animation_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    animation_name: str,
    position: List[float] = [0, 0],
    play_mode: str = "Forward",   # "Forward", "Reverse", "PingPong"
    num_loops: int = 1,           # 0 = infinite
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a PlayAnimation node to a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        animation_name: Name of the animation to play
        position: [X, Y] position in the graph
        play_mode: Animation play mode
        num_loops: Number of loops (0 for infinite)
        path: Content browser path to the widget

    Returns:
        Dict containing node_id and success status

    Example:
        add_play_animation_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="ShowSelector",
            animation_name="FadeIn",
            position=[300, 0],
            play_mode="Forward",
            path="/Game/TrapxTrap/UI"
        )
    """
```

---

## 7. add_set_timer_node

### Python 側

```python
@mcp.tool()
def add_set_timer_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    function_name: str,           # タイマーで呼び出す関数名
    time: float,                  # 待機時間（秒）
    looping: bool = False,
    timer_handle_variable: str = None,  # TimerHandle を保存する変数名
    position: List[float] = [0, 0],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a SetTimerByFunctionName node to a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        function_name: Name of the function to call when timer fires
        time: Time in seconds before the timer fires
        looping: Whether the timer should loop
        timer_handle_variable: Optional variable to store the timer handle
        position: [X, Y] position in the graph
        path: Content browser path to the widget

    Returns:
        Dict containing node_id and success status

    Example:
        # Set timer to hide after 3 seconds
        add_set_timer_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="ShowSelector",
            function_name="HideSelector",
            time=3.0,
            looping=False,
            timer_handle_variable="HideTimerHandle",
            position=[600, 0],
            path="/Game/TrapxTrap/UI"
        )
    """
```

### C++ 実装ポイント

```cpp
// UKismetSystemLibrary::K2_SetTimerDelegate または
// SetTimerByFunctionName ノードを作成
```

---

## 8. add_clear_timer_node

### Python 側

```python
@mcp.tool()
def add_clear_timer_node(
    ctx: Context,
    widget_name: str,
    graph_name: str,
    timer_handle_variable: str,   # クリアする TimerHandle 変数名
    position: List[float] = [0, 0],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a ClearTimer node to a Widget Blueprint's graph.

    Args:
        widget_name: Name of the Widget Blueprint
        graph_name: Name of the graph
        timer_handle_variable: Name of the TimerHandle variable to clear
        position: [X, Y] position in the graph
        path: Content browser path to the widget

    Returns:
        Dict containing node_id and success status

    Example:
        add_clear_timer_node(
            widget_name="WBP_TT_TrapSelector",
            graph_name="ShowSelector",
            timer_handle_variable="HideTimerHandle",
            position=[300, 0],
            path="/Game/TrapxTrap/UI"
        )
    """
```

---

## 使用例: トラップ選択 UI の完全構築

```python
# === Phase 1: Designer ===
create_umg_widget_blueprint("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")
add_vertical_box_to_widget(..., box_name="MainContainer", ...)
add_image_to_widget(..., image_name="TrapIcon", ...)
add_text_to_widget(..., text_name="TrapNameText", ...)
add_text_to_widget(..., text_name="TrapCountText", ...)
reparent_widget_element(..., "TrapIcon", "MainContainer")
reparent_widget_element(..., "TrapNameText", "MainContainer")
reparent_widget_element(..., "TrapCountText", "MainContainer")

# === Phase 2: 変数・関数 ===
add_widget_variable(..., "HideTimerHandle", "TimerHandle")
add_widget_function(..., "UpdateTrapSelection", inputs=[...])
add_widget_function(..., "HideSelector", inputs=[])
add_widget_function(..., "ShowSelector", inputs=[])

# === Phase 3: ノード ===
# UpdateTrapSelection 関数内のロジック
add_widget_element_reference_node(..., "TrapNameText", ...)
add_widget_function_node(..., target="TrapNameText", function_name="SetText", ...)
connect_widget_nodes(...)

# === Phase 4: 高度な機能 ===
# アニメーション
create_widget_animation(..., "FadeIn", length=0.3)
add_animation_track(..., "MainContainer", "RenderOpacity")
add_animation_keyframe(..., time=0.0, value=0.0)
add_animation_keyframe(..., time=0.3, value=1.0)

create_widget_animation(..., "FadeOut", length=0.3)
add_animation_track(..., "MainContainer", "RenderOpacity")
add_animation_keyframe(..., time=0.0, value=1.0)
add_animation_keyframe(..., time=0.3, value=0.0)

# ShowSelector 関数内
add_play_animation_node(..., "FadeIn", ...)
add_set_timer_node(..., function_name="HideSelector", time=3.0, timer_handle_variable="HideTimerHandle")

# HideSelector 関数内
add_play_animation_node(..., "FadeOut", ...)
```

---

## 実装順序

```
1. bind_widget_element_event   ← Button 等のイベント処理
2. get_widget_element_events   ← 利用可能イベントの確認
3. create_widget_animation     ← アニメーション基盤
4. add_animation_track         ← トラック追加
5. add_animation_keyframe      ← キーフレーム追加
6. add_play_animation_node     ← アニメーション再生
7. add_set_timer_node          ← タイマー設定
8. add_clear_timer_node        ← タイマークリア
```

---

## Phase 4 完了条件

以下ができるようになること：

1. Button の OnClicked イベントを関数にバインド
2. Widget アニメーション（Fade In/Out）を作成・再生
3. Timer を使った遅延処理
4. WBP_TT_TrapSelector が完全に MCP 経由で構築可能

---

## 修正日

2025-12-22
