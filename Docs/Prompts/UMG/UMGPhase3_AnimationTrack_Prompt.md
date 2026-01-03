# UMG Phase 3 - Animation Track & Keyframe 実装プロンプト

## 概要

Widget Animation にトラックとキーフレームを追加する機能を実装する。

## プロジェクト情報

- **プロジェクトパス**: `C:\Users\takahito ito\Documents\Unreal Projects\TrapxTrapCpp 5.7`
- **プラグイン**: `Plugins/SpirrowBridge`
- **対象ファイル**: 
  - `Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGCommands.cpp`
  - `Source/SpirrowBridge/Public/Commands/SpirrowBridgeUMGCommands.h`
- **Python MCP ツール**: `Python/tools/umg_tools.py`

---

## 実装ツール

### 1. add_animation_track

アニメーションにプロパティトラックを追加する。

**パラメータ**:
```python
add_animation_track(
    widget_name: str,           # Widget BP 名
    animation_name: str,        # アニメーション名
    target_widget: str,         # 対象 Widget 要素名（例: "MainContainer"）
    property_name: str,         # プロパティ名（Opacity, RenderTransform, ColorAndOpacity）
    path: str = "/Game/UI"      # Widget BP のパス
)
```

**サポートするプロパティ**:
| プロパティ | 型 | トラッククラス |
|-----------|-----|---------------|
| `Opacity` / `RenderOpacity` | float | UMovieSceneFloatTrack |
| `ColorAndOpacity` | FLinearColor | UMovieSceneColorTrack |
| `RenderTransform` | FWidgetTransform | 複数トラック（Translation, Scale, Shear, Angle） |

### 2. add_animation_keyframe

トラックにキーフレームを追加する。

**パラメータ**:
```python
add_animation_keyframe(
    widget_name: str,           # Widget BP 名
    animation_name: str,        # アニメーション名
    target_widget: str,         # 対象 Widget 要素名
    property_name: str,         # プロパティ名
    time: float,                # 時間（秒）
    value: Any,                 # 値（プロパティ型による）
    interpolation: str = "Linear",  # Linear, Cubic, Constant
    path: str = "/Game/UI"
)
```

**値の形式**:
| プロパティ | 値の形式 |
|-----------|---------|
| `Opacity` | float (0.0 - 1.0) |
| `ColorAndOpacity` | [R, G, B, A] (各 0.0 - 1.0) |
| `RenderTransform.Translation` | [X, Y] |
| `RenderTransform.Scale` | [X, Y] |
| `RenderTransform.Angle` | float (degrees) |

---

## C++ 実装

### 必要な include

```cpp
// SpirrowBridgeUMGCommands.cpp に追加
#include "Animation/WidgetAnimation.h"
#include "MovieScene.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Sections/MovieSceneColorSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneColorChannel.h"
```

### Build.cs に追加（既に追加済みなら不要）

```csharp
"MovieScene",
"MovieSceneTracks"
```

---

### HandleAddAnimationTrack 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString WidgetName = Params->GetStringField(TEXT("widget_name"));
    FString AnimationName = Params->GetStringField(TEXT("animation_name"));
    FString TargetWidgetName = Params->GetStringField(TEXT("target_widget"));
    FString PropertyName = Params->GetStringField(TEXT("property_name"));
    FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

    // Widget Blueprint をロード
    FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
    UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
    }

    // アニメーションを検索
    UWidgetAnimation* Animation = nullptr;
    for (UWidgetAnimation* Anim : WidgetBP->Animations)
    {
        if (Anim && Anim->GetName() == AnimationName)
        {
            Animation = Anim;
            break;
        }
    }
    
    if (!Animation)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
    }

    UMovieScene* MovieScene = Animation->GetMovieScene();
    if (!MovieScene)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("MovieScene not found in animation"));
    }

    // 対象 Widget を検索
    UWidget* TargetWidget = nullptr;
    if (WidgetBP->WidgetTree)
    {
        TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*TargetWidgetName));
    }
    
    if (!TargetWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Target widget not found: %s"), *TargetWidgetName));
    }

    // Possessable (バインディング対象) を検索または作成
    FGuid BindingGuid;
    bool bFoundBinding = false;
    
    for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
    {
        const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
        if (Possessable.GetName() == TargetWidgetName)
        {
            BindingGuid = Possessable.GetGuid();
            bFoundBinding = true;
            break;
        }
    }
    
    if (!bFoundBinding)
    {
        // 新しい Possessable を作成
        BindingGuid = MovieScene->AddPossessable(TargetWidgetName, TargetWidget->GetClass());
        
        // Animation の SlotAnimations にバインディングを追加
        FWidgetAnimationBinding Binding;
        Binding.WidgetName = FName(*TargetWidgetName);
        Binding.AnimationGuid = BindingGuid;
        Animation->AnimationBindings.Add(Binding);
    }

    // プロパティに応じたトラックを作成
    FString TrackDisplayName;
    
    if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
    {
        // Float トラック（Opacity）
        UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
        if (Track)
        {
            Track->SetPropertyNameAndPath(FName("RenderOpacity"), "RenderOpacity");
            
            // セクションを追加
            UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
            if (Section)
            {
                Section->SetRange(TRange<FFrameNumber>::All());
                Track->AddSection(*Section);
            }
            
            TrackDisplayName = TEXT("RenderOpacity");
        }
    }
    else if (PropertyName == TEXT("ColorAndOpacity"))
    {
        // Color トラック
        UMovieSceneColorTrack* Track = MovieScene->AddTrack<UMovieSceneColorTrack>(BindingGuid);
        if (Track)
        {
            Track->SetPropertyNameAndPath(FName("ColorAndOpacity"), "ColorAndOpacity");
            
            UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Track->CreateNewSection());
            if (Section)
            {
                Section->SetRange(TRange<FFrameNumber>::All());
                Track->AddSection(*Section);
            }
            
            TrackDisplayName = TEXT("ColorAndOpacity");
        }
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported property: %s. Supported: Opacity, RenderOpacity, ColorAndOpacity"), *PropertyName));
    }

    // 保存
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // レスポンス作成
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("widget_name"), WidgetName);
    Response->SetStringField(TEXT("animation_name"), AnimationName);
    Response->SetStringField(TEXT("target_widget"), TargetWidgetName);
    Response->SetStringField(TEXT("property_name"), TrackDisplayName);
    Response->SetStringField(TEXT("binding_guid"), BindingGuid.ToString());
    
    return Response;
}
```

---

### HandleAddAnimationKeyframe 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddAnimationKeyframe(const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString WidgetName = Params->GetStringField(TEXT("widget_name"));
    FString AnimationName = Params->GetStringField(TEXT("animation_name"));
    FString TargetWidgetName = Params->GetStringField(TEXT("target_widget"));
    FString PropertyName = Params->GetStringField(TEXT("property_name"));
    double Time = Params->GetNumberField(TEXT("time"));
    FString Interpolation = Params->HasField(TEXT("interpolation")) ? 
        Params->GetStringField(TEXT("interpolation")) : TEXT("Linear");
    FString Path = Params->HasField(TEXT("path")) ? 
        Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

    // Widget Blueprint をロード
    FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
    UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
    }

    // アニメーションを検索
    UWidgetAnimation* Animation = nullptr;
    for (UWidgetAnimation* Anim : WidgetBP->Animations)
    {
        if (Anim && Anim->GetName() == AnimationName)
        {
            Animation = Anim;
            break;
        }
    }
    
    if (!Animation)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
    }

    UMovieScene* MovieScene = Animation->GetMovieScene();
    if (!MovieScene)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("MovieScene not found"));
    }

    // Possessable のバインディング GUID を検索
    FGuid BindingGuid;
    bool bFoundBinding = false;
    
    for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
    {
        const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
        if (Possessable.GetName() == TargetWidgetName)
        {
            BindingGuid = Possessable.GetGuid();
            bFoundBinding = true;
            break;
        }
    }
    
    if (!bFoundBinding)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("No binding found for widget: %s. Add a track first."), *TargetWidgetName));
    }

    // フレーム番号を計算
    FFrameRate TickResolution = MovieScene->GetTickResolution();
    FFrameNumber FrameNumber = (Time * TickResolution).FloorToFrame();

    // 補間タイプを決定
    ERichCurveInterpMode InterpMode = RCIM_Linear;
    if (Interpolation == TEXT("Cubic"))
    {
        InterpMode = RCIM_Cubic;
    }
    else if (Interpolation == TEXT("Constant"))
    {
        InterpMode = RCIM_Constant;
    }

    // プロパティに応じてキーフレームを追加
    if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
    {
        // Float 値を取得
        double Value = Params->GetNumberField(TEXT("value"));
        
        // Float トラックを検索
        UMovieSceneFloatTrack* Track = MovieScene->FindTrack<UMovieSceneFloatTrack>(BindingGuid);
        if (!Track)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                TEXT("Float track not found. Add a track first using add_animation_track."));
        }

        // セクションを取得
        if (Track->GetAllSections().Num() == 0)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in track"));
        }

        UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections()[0]);
        if (!Section)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid section"));
        }

        // チャンネルを取得してキーフレーム追加
        FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
        if (Channel)
        {
            FMovieSceneFloatValue KeyValue(Value);
            KeyValue.InterpMode = InterpMode;
            Channel->AddKeys({FrameNumber}, {KeyValue});
        }
    }
    else if (PropertyName == TEXT("ColorAndOpacity"))
    {
        // Color 値を取得 [R, G, B, A]
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (!Params->TryGetArrayField(TEXT("value"), ColorArray) || ColorArray->Num() < 4)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                TEXT("ColorAndOpacity requires [R, G, B, A] array"));
        }

        float R = (*ColorArray)[0]->AsNumber();
        float G = (*ColorArray)[1]->AsNumber();
        float B = (*ColorArray)[2]->AsNumber();
        float A = (*ColorArray)[3]->AsNumber();

        // Color トラックを検索
        UMovieSceneColorTrack* Track = MovieScene->FindTrack<UMovieSceneColorTrack>(BindingGuid);
        if (!Track)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                TEXT("Color track not found. Add a track first."));
        }

        if (Track->GetAllSections().Num() == 0)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in color track"));
        }

        UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Track->GetAllSections()[0]);
        if (!Section)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid color section"));
        }

        // 各チャンネルにキーフレーム追加 (R, G, B, A)
        TArrayView<FMovieSceneFloatChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
        if (Channels.Num() >= 4)
        {
            FMovieSceneFloatValue RValue(R); RValue.InterpMode = InterpMode;
            FMovieSceneFloatValue GValue(G); GValue.InterpMode = InterpMode;
            FMovieSceneFloatValue BValue(B); BValue.InterpMode = InterpMode;
            FMovieSceneFloatValue AValue(A); AValue.InterpMode = InterpMode;
            
            Channels[0]->AddKeys({FrameNumber}, {RValue});
            Channels[1]->AddKeys({FrameNumber}, {GValue});
            Channels[2]->AddKeys({FrameNumber}, {BValue});
            Channels[3]->AddKeys({FrameNumber}, {AValue});
        }
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported property for keyframe: %s"), *PropertyName));
    }

    // 保存
    WidgetBP->MarkPackageDirty();

    // レスポンス
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("widget_name"), WidgetName);
    Response->SetStringField(TEXT("animation_name"), AnimationName);
    Response->SetStringField(TEXT("target_widget"), TargetWidgetName);
    Response->SetStringField(TEXT("property_name"), PropertyName);
    Response->SetNumberField(TEXT("time"), Time);
    Response->SetNumberField(TEXT("frame"), FrameNumber.Value);
    Response->SetStringField(TEXT("interpolation"), Interpolation);
    
    return Response;
}
```

---

## コマンドルーター登録

```cpp
// SpirrowBridgeUMGCommands.cpp の RegisterCommands() に追加

CommandHandlers.Add(TEXT("add_animation_track"), 
    FCommandHandler::CreateRaw(this, &FSpirrowBridgeUMGCommands::HandleAddAnimationTrack));
    
CommandHandlers.Add(TEXT("add_animation_keyframe"), 
    FCommandHandler::CreateRaw(this, &FSpirrowBridgeUMGCommands::HandleAddAnimationKeyframe));
```

---

## Python MCP ツール定義

```python
# Python/tools/umg_tools.py に追加

@mcp.tool()
async def add_animation_track(
    widget_name: str,
    animation_name: str,
    target_widget: str,
    property_name: str,
    path: str = "/Game/UI"
) -> dict:
    """
    Add an animation track to a Widget Animation.

    Args:
        widget_name: Name of the Widget Blueprint
        animation_name: Name of the animation to add track to
        target_widget: Name of the widget element to animate (e.g., "MainContainer")
        property_name: Property to animate:
            - "Opacity" or "RenderOpacity": Float track for opacity
            - "ColorAndOpacity": Color track for tint color
        path: Content browser path to the widget (default: "/Game/UI")

    Returns:
        Dict containing success status and track details

    Example:
        add_animation_track(
            widget_name="WBP_TT_TrapSelector",
            animation_name="FadeIn",
            target_widget="MainContainer",
            property_name="Opacity",
            path="/Game/TrapxTrap/UI"
        )
    """
    return await send_command("add_animation_track", {
        "widget_name": widget_name,
        "animation_name": animation_name,
        "target_widget": target_widget,
        "property_name": property_name,
        "path": path
    })


@mcp.tool()
async def add_animation_keyframe(
    widget_name: str,
    animation_name: str,
    target_widget: str,
    property_name: str,
    time: float,
    value: any,
    interpolation: str = "Linear",
    path: str = "/Game/UI"
) -> dict:
    """
    Add a keyframe to an animation track.

    Args:
        widget_name: Name of the Widget Blueprint
        animation_name: Name of the animation
        target_widget: Name of the widget element being animated
        property_name: Property being animated (must match existing track)
        time: Time in seconds for the keyframe
        value: Value for the keyframe:
            - Opacity: float (0.0 - 1.0)
            - ColorAndOpacity: [R, G, B, A] array (each 0.0 - 1.0)
        interpolation: Interpolation mode - "Linear", "Cubic", or "Constant"
        path: Content browser path to the widget

    Returns:
        Dict containing success status and keyframe details

    Example:
        # Fade from transparent to opaque
        add_animation_keyframe(
            widget_name="WBP_TT_TrapSelector",
            animation_name="FadeIn",
            target_widget="MainContainer",
            property_name="Opacity",
            time=0.0,
            value=0.0,
            path="/Game/TrapxTrap/UI"
        )
        add_animation_keyframe(
            widget_name="WBP_TT_TrapSelector",
            animation_name="FadeIn",
            target_widget="MainContainer",
            property_name="Opacity",
            time=0.5,
            value=1.0,
            path="/Game/TrapxTrap/UI"
        )
    """
    return await send_command("add_animation_keyframe", {
        "widget_name": widget_name,
        "animation_name": animation_name,
        "target_widget": target_widget,
        "property_name": property_name,
        "time": time,
        "value": value,
        "interpolation": interpolation,
        "path": path
    })
```

---

## ヘッダー宣言

```cpp
// SpirrowBridgeUMGCommands.h の private セクションに追加

TSharedPtr<FJsonObject> HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params);
TSharedPtr<FJsonObject> HandleAddAnimationKeyframe(const TSharedPtr<FJsonObject>& Params);
```

---

## テストケース

実装完了後、以下のテストを実施：

```python
# Test 1: Opacity トラック追加
add_animation_track(
    widget_name="WBP_TT_TrapSelector",
    animation_name="FadeIn",
    target_widget="MainContainer",
    property_name="Opacity",
    path="/Game/TrapxTrap/UI"
)

# Test 2: キーフレーム追加（0秒で透明）
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="FadeIn",
    target_widget="MainContainer",
    property_name="Opacity",
    time=0.0,
    value=0.0,
    path="/Game/TrapxTrap/UI"
)

# Test 3: キーフレーム追加（0.5秒で不透明）
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="FadeIn",
    target_widget="MainContainer",
    property_name="Opacity",
    time=0.5,
    value=1.0,
    path="/Game/TrapxTrap/UI"
)

# Test 4: ColorAndOpacity トラック
add_animation_track(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PulseLoop",
    target_widget="TrapNameText",
    property_name="ColorAndOpacity",
    path="/Game/TrapxTrap/UI"
)

# Test 5: Color キーフレーム
add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PulseLoop",
    target_widget="TrapNameText",
    property_name="ColorAndOpacity",
    time=0.0,
    value=[1.0, 1.0, 1.0, 1.0],  # White
    path="/Game/TrapxTrap/UI"
)

add_animation_keyframe(
    widget_name="WBP_TT_TrapSelector",
    animation_name="PulseLoop",
    target_widget="TrapNameText",
    property_name="ColorAndOpacity",
    time=0.5,
    value=[1.0, 0.5, 0.0, 1.0],  # Orange
    path="/Game/TrapxTrap/UI"
)
```

---

## 注意事項

1. **FWidgetAnimationBinding**: Widget とアニメーションをバインドするために必要
2. **TickResolution**: MovieScene のフレームレート解像度を使用して時間→フレーム変換
3. **ERichCurveInterpMode**: UE5 では補間モードが RichCurve ベース
4. **セクション範囲**: `TRange<FFrameNumber>::All()` で全範囲カバー

---

**作成日**: 2025-12-22
**フェーズ**: UMG Phase 3 - Animation Track & Keyframe
