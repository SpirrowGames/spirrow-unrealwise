# 機能名: RenderTransform トラック対応

## 概要

`add_animation_track` と `add_animation_keyframe` を拡張して、
Widget の RenderTransform プロパティ（Translation, Scale, Angle）のアニメーションに対応する。

スライドイン/アウト、スケールアニメーション、回転アニメーションが可能になる。

## 仕様

### 新規サポートプロパティ

| プロパティ名 | 値の形式 | トラッククラス | 用途 |
|-------------|---------|---------------|------|
| `RenderTransform.Translation` | [X, Y] | UMovieScene2DTransformTrack または Float x2 | 移動アニメーション |
| `RenderTransform.Scale` | [X, Y] | UMovieScene2DTransformTrack または Float x2 | 拡大縮小 |
| `RenderTransform.Angle` | float (度) | UMovieSceneFloatTrack | 回転 |

### 既存サポートプロパティ（変更なし）

| プロパティ名 | 値の形式 | トラッククラス |
|-------------|---------|---------------|
| `Opacity` / `RenderOpacity` | float (0.0-1.0) | UMovieSceneFloatTrack |
| `ColorAndOpacity` | [R, G, B, A] | UMovieSceneColorTrack |

## 実装方針

### 方針A: 個別Floatトラック（推奨）

RenderTransform の各コンポーネントを個別の Float トラックとして追加。
UMG の WidgetAnimation がサポートする標準的な方法。

```
RenderTransform.Translation.X → Float Track
RenderTransform.Translation.Y → Float Track
RenderTransform.Scale.X → Float Track
RenderTransform.Scale.Y → Float Track
RenderTransform.Angle → Float Track
```

### 方針B: 2DTransformトラック

UMovieScene2DTransformTrack を使用（MovieSceneTracksモジュール）。
Translation, Scale, Shear, Angle を一括管理。

→ 方針Aの方がシンプルで確実なため、方針Aを採用。

## 実装内容

### 1. Python側 (`Python/tools/umg_tools.py`)

`add_animation_track` と `add_animation_keyframe` の docstring を更新:

```python
@mcp.tool()
def add_animation_track(
    ctx: Context,
    widget_name: str,
    animation_name: str,
    target_widget: str,
    property_name: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add an animation track to a Widget Animation.

    Args:
        widget_name: Name of the Widget Blueprint
        animation_name: Name of the animation to add track to
        target_widget: Name of the widget element to animate (e.g., "MainContainer")
        property_name: Property to animate:
            - "Opacity" or "RenderOpacity": Float track for opacity
            - "ColorAndOpacity": Color track for tint color
            - "RenderTransform.Translation.X": Float track for X translation
            - "RenderTransform.Translation.Y": Float track for Y translation
            - "RenderTransform.Scale.X": Float track for X scale
            - "RenderTransform.Scale.Y": Float track for Y scale
            - "RenderTransform.Angle": Float track for rotation angle (degrees)
        path: Content browser path to the widget (default: "/Game/UI")

    Returns:
        Dict containing success status and track details

    Example:
        # Slide animation X
        add_animation_track(
            widget_name="WBP_TT_TrapSelector",
            animation_name="SlideIn",
            target_widget="MainContainer",
            property_name="RenderTransform.Translation.X",
            path="/Game/TrapxTrap/UI"
        )
    """
    # 実装は変更なし（C++側で処理）
```

`add_animation_keyframe` の docstring 更新:

```python
@mcp.tool()
def add_animation_keyframe(
    # ... 既存パラメータ
) -> Dict[str, Any]:
    """
    Add a keyframe to an animation track.

    Args:
        # ... 既存パラメータ
        property_name: Property being animated (must match existing track)
        value: Value for the keyframe:
            - Opacity: float (0.0 - 1.0)
            - ColorAndOpacity: [R, G, B, A] array (each 0.0 - 1.0)
            - RenderTransform.Translation.X/Y: float (pixels)
            - RenderTransform.Scale.X/Y: float (1.0 = 100%)
            - RenderTransform.Angle: float (degrees)
        # ...

    Example:
        # Slide from left
        add_animation_keyframe(
            widget_name="WBP_TT_TrapSelector",
            animation_name="SlideIn",
            target_widget="MainContainer",
            property_name="RenderTransform.Translation.X",
            time=0.0,
            value=-200.0,  # Start 200px left
            path="/Game/TrapxTrap/UI"
        )
        add_animation_keyframe(
            widget_name="WBP_TT_TrapSelector",
            animation_name="SlideIn",
            target_widget="MainContainer",
            property_name="RenderTransform.Translation.X",
            time=0.3,
            value=0.0,  # End at origin
            interpolation="Cubic",
            path="/Game/TrapxTrap/UI"
        )
    """
```

### 2. Unreal側 (`SpirrowBridgeUMGCommands.cpp`)

#### HandleAddAnimationTrack 拡張:

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params)
{
    // ... 既存のパラメータ取得コード ...

    // Determine track type based on property name
    FString TrackDisplayName = PropertyName;
    
    if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
    {
        // 既存のOpacityトラック処理
        TrackDisplayName = TEXT("RenderOpacity");
        // ... Float track 作成 ...
    }
    else if (PropertyName == TEXT("ColorAndOpacity"))
    {
        // 既存のColorトラック処理
        // ... Color track 作成 ...
    }
    else if (PropertyName.StartsWith(TEXT("RenderTransform.")))
    {
        // RenderTransform プロパティ
        // Translation.X, Translation.Y, Scale.X, Scale.Y, Angle
        
        // Float track を作成
        UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
        if (!Track)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create float track for RenderTransform"));
        }
        
        // プロパティ名をトラック名として設定
        Track->SetTrackName(FName(*PropertyName));
        
        // プロパティパスを設定
        // "RenderTransform.Translation.X" → PropertyBinding に設定
        Track->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);
        
        // Section 追加
        UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
        if (Section)
        {
            Section->SetRange(TRange<FFrameNumber>::All());
            Track->AddSection(*Section);
        }
        
        TrackDisplayName = PropertyName;
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported property: %s"), *PropertyName));
    }

    // ... レスポンス作成 ...
}
```

#### HandleAddAnimationKeyframe 拡張:

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddAnimationKeyframe(const TSharedPtr<FJsonObject>& Params)
{
    // ... 既存のパラメータ取得コード ...

    if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
    {
        // 既存のOpacityキーフレーム処理
        // ...
    }
    else if (PropertyName == TEXT("ColorAndOpacity"))
    {
        // 既存のColorキーフレーム処理
        // ...
    }
    else if (PropertyName.StartsWith(TEXT("RenderTransform.")))
    {
        // RenderTransform キーフレーム処理
        double Value = Params->GetNumberField(TEXT("value"));
        
        // Find Float track by name
        UMovieSceneFloatTrack* Track = nullptr;
        for (UMovieSceneTrack* TestTrack : MovieScene->GetTracks())
        {
            if (UMovieSceneFloatTrack* FloatTrack = Cast<UMovieSceneFloatTrack>(TestTrack))
            {
                if (FloatTrack->GetTrackName().ToString() == PropertyName)
                {
                    Track = FloatTrack;
                    break;
                }
            }
        }
        
        // バインディングGUIDでも検索
        if (!Track)
        {
            Track = MovieScene->FindTrack<UMovieSceneFloatTrack>(BindingGuid);
            // トラック名でフィルタ
            if (Track && Track->GetTrackName().ToString() != PropertyName)
            {
                Track = nullptr;
            }
        }
        
        if (!Track)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Float track not found for property: %s"), *PropertyName));
        }
        
        // キーフレーム追加（Opacityと同じロジック）
        if (Track->GetAllSections().Num() == 0)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in track"));
        }
        
        UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections()[0]);
        if (!Section)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid section"));
        }
        
        FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
        if (Channel)
        {
            FMovieSceneFloatValue KeyValue(Value);
            KeyValue.InterpMode = InterpMode;
            Channel->AddKeys({FrameNumber}, {KeyValue});
        }
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported property: %s"), *PropertyName));
    }

    // ... レスポンス作成 ...
}
```

### 3. トラック検索の改善

複数の Float トラックがある場合に正しいトラックを見つけるため、
トラック名（プロパティ名）でフィルタリングする必要がある。

```cpp
// 複数トラック対応：プロパティ名でトラックを検索
UMovieSceneFloatTrack* FindFloatTrackByPropertyName(UMovieScene* MovieScene, const FGuid& BindingGuid, const FString& PropertyName)
{
    // まずバインディングに紐づくトラックを取得
    const FMovieSceneBinding* Binding = MovieScene->FindBinding(BindingGuid);
    if (!Binding)
    {
        return nullptr;
    }
    
    for (UMovieSceneTrack* Track : Binding->GetTracks())
    {
        if (UMovieSceneFloatTrack* FloatTrack = Cast<UMovieSceneFloatTrack>(Track))
        {
            if (FloatTrack->GetTrackName().ToString() == PropertyName ||
                FloatTrack->GetPropertyPath().ToString() == PropertyName)
            {
                return FloatTrack;
            }
        }
    }
    
    return nullptr;
}
```

## テスト

### テスト1: スライドインアニメーション

```python
# アニメーション作成
create_widget_animation(
    widget_name="WBP_TT_TrapSelector",
    animation_name="SlideInX",
    length=0.3,
    path="/Game/TrapxTrap/UI"
)

# Translation.X トラック追加
add_animation_track(
    widget_name="WBP_TT_TrapSelector",
    animation_name="SlideInX",
    target_widget="MainContainer",
    property_name="RenderTransform.Translation.X",
    path="/Game/TrapxTrap/UI"
)

# キーフレーム: 0秒で -200px（画面外左）
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="SlideInX",
    target_widget="MainContainer",
    property_name="RenderTransform.Translation.X",
    time=0.0,
    value=-200.0,
    path="/Game/TrapxTrap/UI"
)

# キーフレーム: 0.3秒で 0px（元の位置）
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="SlideInX",
    target_widget="MainContainer",
    property_name="RenderTransform.Translation.X",
    time=0.3,
    value=0.0,
    interpolation="Cubic",
    path="/Game/TrapxTrap/UI"
)
```

### テスト2: スケールアニメーション

```python
# Scale.X トラック追加
add_animation_track(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PopIn",
    target_widget="TrapIcon",
    property_name="RenderTransform.Scale.X",
    path="/Game/TrapxTrap/UI"
)

# Scale.Y トラック追加
add_animation_track(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PopIn",
    target_widget="TrapIcon",
    property_name="RenderTransform.Scale.Y",
    path="/Game/TrapxTrap/UI"
)

# キーフレーム: 0秒で 0（見えない）
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PopIn",
    target_widget="TrapIcon",
    property_name="RenderTransform.Scale.X",
    time=0.0,
    value=0.0,
    path="/Game/TrapxTrap/UI"
)

# キーフレーム: 0.2秒で 1.2（オーバーシュート）
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PopIn",
    target_widget="TrapIcon",
    property_name="RenderTransform.Scale.X",
    time=0.2,
    value=1.2,
    interpolation="Cubic",
    path="/Game/TrapxTrap/UI"
)

# キーフレーム: 0.3秒で 1.0（最終サイズ）
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PopIn",
    target_widget="TrapIcon",
    property_name="RenderTransform.Scale.X",
    time=0.3,
    value=1.0,
    interpolation="Cubic",
    path="/Game/TrapxTrap/UI"
)
```

### テスト3: 回転アニメーション

```python
# Angle トラック追加
add_animation_track(
    widget_name="WBP_TT_TrapSelector",
    animation_name="Spin",
    target_widget="TrapIcon",
    property_name="RenderTransform.Angle",
    path="/Game/TrapxTrap/UI"
)

# キーフレーム: 0秒で 0度
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="Spin",
    target_widget="TrapIcon",
    property_name="RenderTransform.Angle",
    time=0.0,
    value=0.0,
    path="/Game/TrapxTrap/UI"
)

# キーフレーム: 0.5秒で 360度
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="Spin",
    target_widget="TrapIcon",
    property_name="RenderTransform.Angle",
    time=0.5,
    value=360.0,
    interpolation="Linear",
    path="/Game/TrapxTrap/UI"
)
```

## 補足事項

### RenderTransform プロパティパス

UMG Widget の RenderTransform は以下の構造:
```
RenderTransform (FWidgetTransform)
├── Translation (FVector2D)
│   ├── X (float)
│   └── Y (float)
├── Scale (FVector2D)
│   ├── X (float)
│   └── Y (float)
├── Shear (FVector2D) ※今回は未対応
│   ├── X (float)
│   └── Y (float)
└── Angle (float)
```

### 注意点

1. **トラック名の一意性**: 同じウィジェットに複数のRenderTransformプロパティを
   アニメーションする場合、各プロパティごとに別のトラックが必要

2. **プロパティパス**: UE のプロパティシステムでは `RenderTransform.Translation.X` 
   のようなドット区切りパスでプロパティにアクセス

3. **初期値**: アニメーション開始時の初期値はウィジェットの現在の値が使用される

4. **Shear**: 今回は未対応。必要に応じて追加可能

---

**作成日**: 2025-12-24
**フェーズ**: UMG Phase 3 - RenderTransform Animation
