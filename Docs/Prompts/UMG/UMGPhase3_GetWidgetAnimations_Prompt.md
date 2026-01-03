# 機能名: get_widget_animations 実装

## 概要

Widget Blueprint内のすべてのアニメーション一覧を取得するユーティリティツール。
アニメーション名、長さ、トラック情報、キーフレーム数などを返す。

## 仕様

### Python側パラメータ

| パラメータ | 型 | 必須 | デフォルト | 説明 |
|-----------|-----|------|------------|------|
| widget_name | str | ✅ | - | Widget Blueprint名 |
| path | str | | "/Game/UI" | Content Browser パス |

### 戻り値

```json
{
  "success": true,
  "widget_name": "WBP_TT_TrapSelector",
  "animation_count": 2,
  "animations": [
    {
      "name": "FadeIn",
      "length": 0.5,
      "is_looping": false,
      "track_count": 1,
      "tracks": [
        {
          "target_widget": "MainContainer",
          "property": "RenderOpacity",
          "type": "Float",
          "keyframe_count": 2
        }
      ]
    },
    {
      "name": "PulseLoop",
      "length": 1.0,
      "is_looping": true,
      "track_count": 1,
      "tracks": [
        {
          "target_widget": "TrapNameText",
          "property": "ColorAndOpacity",
          "type": "Color",
          "keyframe_count": 3
        }
      ]
    }
  ]
}
```

## 実装内容

### 1. Python側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def get_widget_animations(
    ctx: Context,
    widget_name: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Get all animations in a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        path: Content browser path to the widget

    Returns:
        Dict containing list of animations with their details

    Example:
        get_widget_animations(
            widget_name="WBP_TT_TrapSelector",
            path="/Game/TrapxTrap/UI"
        )
    """
    from unreal_mcp_server import get_unreal_connection

    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.error("Failed to connect to Unreal Engine")
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        params = {
            "widget_name": widget_name,
            "path": path
        }

        logger.info(f"Getting widget animations with params: {params}")
        response = unreal.send_command("get_widget_animations", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Get widget animations response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error getting widget animations: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### 2. Unreal側 (`SpirrowBridgeUMGCommands.h`)

privateセクションに追加:

```cpp
// Phase 3: Animation Operations
TSharedPtr<FJsonObject> HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params);
```

### 3. Unreal側 (`SpirrowBridgeUMGCommands.cpp`)

#### HandleCommand内ルーティング追加:

```cpp
else if (CommandType == TEXT("get_widget_animations"))
{
    return HandleGetWidgetAnimations(Params);
}
```

#### 関数実装:

```cpp
// Phase 3: Get Widget Animations
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters
    FString WidgetName = Params->GetStringField(TEXT("widget_name"));
    FString Path = Params->HasField(TEXT("path")) ?
        Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

    // Load Widget Blueprint
    FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
    UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
    }

    // Build animations array
    TArray<TSharedPtr<FJsonValue>> AnimationsArray;
    
    for (UWidgetAnimation* Animation : WidgetBP->Animations)
    {
        if (!Animation) continue;
        
        TSharedPtr<FJsonObject> AnimObj = MakeShareable(new FJsonObject());
        AnimObj->SetStringField(TEXT("name"), Animation->GetName());
        
        UMovieScene* MovieScene = Animation->GetMovieScene();
        if (MovieScene)
        {
            // Get animation length
            FFrameRate TickResolution = MovieScene->GetTickResolution();
            FFrameRate DisplayRate = MovieScene->GetDisplayRate();
            TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
            
            double StartTime = TickResolution.AsSeconds(PlaybackRange.GetLowerBoundValue());
            double EndTime = TickResolution.AsSeconds(PlaybackRange.GetUpperBoundValue());
            double Length = EndTime - StartTime;
            
            AnimObj->SetNumberField(TEXT("length"), Length);
            
            // Check if looping (this info is typically stored externally, default to false)
            AnimObj->SetBoolField(TEXT("is_looping"), false);
            
            // Get tracks
            TArray<TSharedPtr<FJsonValue>> TracksArray;
            
            for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
            {
                const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
                FGuid BindingGuid = Possessable.GetGuid();
                FString TargetName = Possessable.GetName();
                
                // Check for Float tracks (Opacity)
                if (UMovieSceneFloatTrack* FloatTrack = MovieScene->FindTrack<UMovieSceneFloatTrack>(BindingGuid))
                {
                    TSharedPtr<FJsonObject> TrackObj = MakeShareable(new FJsonObject());
                    TrackObj->SetStringField(TEXT("target_widget"), TargetName);
                    TrackObj->SetStringField(TEXT("property"), FloatTrack->GetTrackName().ToString());
                    TrackObj->SetStringField(TEXT("type"), TEXT("Float"));
                    
                    int32 KeyframeCount = 0;
                    if (FloatTrack->GetAllSections().Num() > 0)
                    {
                        UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(FloatTrack->GetAllSections()[0]);
                        if (Section)
                        {
                            FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
                            if (Channel)
                            {
                                KeyframeCount = Channel->GetNumKeys();
                            }
                        }
                    }
                    TrackObj->SetNumberField(TEXT("keyframe_count"), KeyframeCount);
                    
                    TracksArray.Add(MakeShareable(new FJsonValueObject(TrackObj)));
                }
                
                // Check for Color tracks (ColorAndOpacity)
                if (UMovieSceneColorTrack* ColorTrack = MovieScene->FindTrack<UMovieSceneColorTrack>(BindingGuid))
                {
                    TSharedPtr<FJsonObject> TrackObj = MakeShareable(new FJsonObject());
                    TrackObj->SetStringField(TEXT("target_widget"), TargetName);
                    TrackObj->SetStringField(TEXT("property"), ColorTrack->GetTrackName().ToString());
                    TrackObj->SetStringField(TEXT("type"), TEXT("Color"));
                    
                    int32 KeyframeCount = 0;
                    if (ColorTrack->GetAllSections().Num() > 0)
                    {
                        UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(ColorTrack->GetAllSections()[0]);
                        if (Section)
                        {
                            // Get keyframe count from first channel (R)
                            TArrayView<FMovieSceneFloatChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
                            if (Channels.Num() > 0)
                            {
                                KeyframeCount = Channels[0]->GetNumKeys();
                            }
                        }
                    }
                    TrackObj->SetNumberField(TEXT("keyframe_count"), KeyframeCount);
                    
                    TracksArray.Add(MakeShareable(new FJsonValueObject(TrackObj)));
                }
            }
            
            AnimObj->SetNumberField(TEXT("track_count"), TracksArray.Num());
            AnimObj->SetArrayField(TEXT("tracks"), TracksArray);
        }
        else
        {
            AnimObj->SetNumberField(TEXT("length"), 0.0);
            AnimObj->SetBoolField(TEXT("is_looping"), false);
            AnimObj->SetNumberField(TEXT("track_count"), 0);
            AnimObj->SetArrayField(TEXT("tracks"), TArray<TSharedPtr<FJsonValue>>());
        }
        
        AnimationsArray.Add(MakeShareable(new FJsonValueObject(AnimObj)));
    }

    // Response
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("widget_name"), WidgetName);
    Response->SetNumberField(TEXT("animation_count"), AnimationsArray.Num());
    Response->SetArrayField(TEXT("animations"), AnimationsArray);

    return Response;
}
```

### 4. SpirrowBridge.cpp ルーティング追加

`ExecuteCommand` 内の UMG コマンド判定に追加:

```cpp
else if (CommandType == TEXT("get_widget_animations") ||
         // ... 他のUMGコマンド
        )
{
    ResultJson = UMGCommands->HandleCommand(CommandType, Params);
}
```

## テスト

```python
# テスト: アニメーション一覧取得
get_widget_animations(
    widget_name="WBP_TT_TrapSelector",
    path="/Game/TrapxTrap/UI"
)
```

期待される出力:
```json
{
  "success": true,
  "widget_name": "WBP_TT_TrapSelector",
  "animation_count": 2,
  "animations": [
    {
      "name": "FadeIn",
      "length": 0.5,
      "is_looping": false,
      "track_count": 1,
      "tracks": [...]
    }
  ]
}
```

## 補足事項

- UWidgetAnimation::bLoopはprivateのため、is_loopingは常にfalseを返す
- 将来的にリフレクションでループ情報を取得する拡張が可能
- トラックタイプは現在Float（Opacity）とColor（ColorAndOpacity）のみ対応

---

**作成日**: 2025-12-24
**フェーズ**: UMG Phase 3 - Animation
