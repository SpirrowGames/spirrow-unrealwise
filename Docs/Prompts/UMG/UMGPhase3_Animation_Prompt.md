# UMG Phase 3 設計ドキュメント: アニメーション & 高度なバインディング

## 概要

Phase 3 では以下の機能を実装する：

1. **Widget Animation** - UMG アニメーション作成・制御
2. **配列型変数サポート** - TArray 型の変数操作
3. **完全なプロパティバインディング** - UMG 内部 API を使用した自動バインディング
4. **動的 Widget 操作** - ランタイム Widget 生成ノード

---

## 実装ツール一覧

### 3.1 Widget Animation（優先度: 高）

| ツール | 説明 |
|--------|------|
| `create_widget_animation` | Widget Animation 新規作成 |
| `add_animation_track` | アニメーショントラック追加 |
| `add_animation_keyframe` | キーフレーム追加 |
| `get_widget_animations` | アニメーション一覧取得 |

### 3.2 配列型変数（優先度: 高）

| ツール | 説明 |
|--------|------|
| `add_widget_array_variable` | 配列型変数追加 |
| `set_widget_array_default` | 配列デフォルト値設定 |

### 3.3 高度なバインディング（優先度: 中）

| ツール | 説明 |
|--------|------|
| `create_property_binding` | 完全なプロパティバインディング設定 |
| `create_visibility_binding` | Visibility バインディング |

### 3.4 ノード操作（優先度: 中）

| ツール | 説明 |
|--------|------|
| `add_widget_graph_node` | Widget BP グラフにノード追加 |
| `connect_widget_nodes` | ノード接続 |

---

## 詳細設計

### 3.1.1 create_widget_animation

Widget Blueprint に新しいアニメーションを作成する。

**パラメータ**:
```python
create_widget_animation(
    widget_name: str,           # Widget BP 名
    animation_name: str,        # アニメーション名
    length: float = 1.0,        # 長さ（秒）
    loop: bool = False,         # ループ再生
    path: str = "/Game/UI"      # パス
)
```

**C++ 実装概要**:
```cpp
TSharedPtr<FJsonObject> HandleCreateWidgetAnimation(const TSharedPtr<FJsonObject>& Params)
{
    // Widget Blueprint をロード
    UWidgetBlueprint* WidgetBP = ...;
    
    // 新しいアニメーションを作成
    UWidgetAnimation* NewAnimation = NewObject<UWidgetAnimation>(
        WidgetBP, 
        UWidgetAnimation::StaticClass(), 
        FName(*AnimationName)
    );
    
    // MovieScene を設定
    UMovieScene* MovieScene = NewObject<UMovieScene>(NewAnimation);
    NewAnimation->MovieScene = MovieScene;
    
    // 長さを設定
    MovieScene->SetPlaybackRange(
        FFrameNumber(0), 
        FFrameNumber(Length * MovieScene->GetTickResolution().AsDecimal())
    );
    
    // ループ設定
    if (bLoop)
    {
        // ループフラグ設定
    }
    
    // Blueprint に追加
    WidgetBP->Animations.Add(NewAnimation);
    
    // コンパイル
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);
}
```

**戻り値**:
```json
{
    "success": true,
    "animation_name": "FadeInAnimation",
    "animation_id": "GUID",
    "length": 1.0
}
```

---

### 3.1.2 add_animation_track

アニメーションにトラック（プロパティトラック）を追加する。

**パラメータ**:
```python
add_animation_track(
    widget_name: str,           # Widget BP 名
    animation_name: str,        # アニメーション名
    target_widget: str,         # 対象 Widget 要素名
    property_name: str,         # プロパティ名 (Opacity, RenderTransform, ColorAndOpacity 等)
    path: str = "/Game/UI"
)
```

**サポートするプロパティ**:
- `Opacity` (float) - 不透明度
- `RenderTransform` (FWidgetTransform) - 変形
- `ColorAndOpacity` (FLinearColor) - 色
- `Visibility` (ESlateVisibility) - 表示状態

**C++ 実装概要**:
```cpp
TSharedPtr<FJsonObject> HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params)
{
    // アニメーション取得
    UWidgetAnimation* Animation = FindAnimation(WidgetBP, AnimationName);
    UMovieScene* MovieScene = Animation->MovieScene;
    
    // 対象 Widget 取得
    UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*TargetWidgetName));
    
    // Widget をバインディング対象として登録
    FGuid BindingGuid = MovieScene->AddPossessable(TargetWidgetName, TargetWidget->GetClass());
    
    // プロパティに応じたトラックを追加
    if (PropertyName == TEXT("Opacity"))
    {
        UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
        Track->SetPropertyNameAndPath(FName("RenderOpacity"), "RenderOpacity");
    }
    else if (PropertyName == TEXT("RenderTransform"))
    {
        // Transform トラック
        UMovieScene2DTransformTrack* Track = MovieScene->AddTrack<UMovieScene2DTransformTrack>(BindingGuid);
    }
    // ...
}
```

---

### 3.1.3 add_animation_keyframe

トラックにキーフレームを追加する。

**パラメータ**:
```python
add_animation_keyframe(
    widget_name: str,
    animation_name: str,
    target_widget: str,
    property_name: str,
    time: float,                # 時間（秒）
    value: Any,                 # 値（プロパティ型による）
    interpolation: str = "Linear",  # Linear, Cubic, Constant
    path: str = "/Game/UI"
)
```

**使用例**:
```python
# フェードイン: 0秒で透明、1秒で不透明
add_animation_keyframe(
    widget_name="WBP_HUD",
    animation_name="FadeIn",
    target_widget="MainPanel",
    property_name="Opacity",
    time=0.0,
    value=0.0
)
add_animation_keyframe(
    widget_name="WBP_HUD",
    animation_name="FadeIn",
    target_widget="MainPanel",
    property_name="Opacity",
    time=1.0,
    value=1.0
)
```

---

### 3.2.1 add_widget_array_variable

配列型変数を追加する。

**パラメータ**:
```python
add_widget_array_variable(
    widget_name: str,
    variable_name: str,
    element_type: str,          # 要素の型 (String, Integer, etc.)
    is_exposed: bool = False,
    category: str = None,
    path: str = "/Game/UI"
)
```

**C++ 実装**:
```cpp
TSharedPtr<FJsonObject> HandleAddWidgetArrayVariable(const TSharedPtr<FJsonObject>& Params)
{
    // ... パラメータ取得 ...
    
    // 要素型の PinType を設定
    FEdGraphPinType ElementPinType;
    SetupPinType(ElementType, ElementPinType);
    
    // 配列型に変換
    FEdGraphPinType ArrayPinType = ElementPinType;
    ArrayPinType.ContainerType = EPinContainerType::Array;
    
    // 変数追加
    FBlueprintEditorUtils::AddMemberVariable(WidgetBP, FName(*VariableName), ArrayPinType);
    
    // ... 後処理 ...
}
```

---

### 3.3.1 create_property_binding

UMG の完全なプロパティバインディングを設定する。

**パラメータ**:
```python
create_property_binding(
    widget_name: str,
    element_name: str,          # バインド対象の Widget 要素
    property_name: str,         # バインドするプロパティ
    binding_type: str,          # "Variable", "Function"
    binding_source: str,        # 変数名 or 関数名
    path: str = "/Game/UI"
)
```

**C++ 実装概要**:

UMG のプロパティバインディングは `FDelegateRuntimeBinding` を使用する：

```cpp
TSharedPtr<FJsonObject> HandleCreatePropertyBinding(const TSharedPtr<FJsonObject>& Params)
{
    // Widget Blueprint をロード
    UWidgetBlueprint* WidgetBP = ...;
    
    // 対象 Widget を取得
    UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
    
    // バインディング情報を作成
    FDelegateRuntimeBinding Binding;
    Binding.ObjectName = ElementName;
    Binding.PropertyName = FName(*PropertyName);
    
    if (BindingType == TEXT("Function"))
    {
        Binding.FunctionName = FName(*BindingSource);
        Binding.Kind = EBindingKind::Function;
    }
    else // Variable
    {
        // 変数バインディングはゲッター関数経由
        FString GetterName = FString::Printf(TEXT("Get%s"), *BindingSource);
        Binding.FunctionName = FName(*GetterName);
        Binding.Kind = EBindingKind::Function;
    }
    
    // Blueprint にバインディングを追加
    WidgetBP->Bindings.Add(Binding);
    
    // コンパイル
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);
}
```

---

## 実装順序

### Week 1: アニメーション基盤
1. `create_widget_animation` - アニメーション作成
2. `get_widget_animations` - アニメーション一覧
3. `add_animation_track` - トラック追加（Opacity のみ）

### Week 2: アニメーション完成
4. `add_animation_keyframe` - キーフレーム追加
5. トラック拡張（RenderTransform, ColorAndOpacity）

### Week 3: 配列 & バインディング
6. `add_widget_array_variable` - 配列変数
7. `create_property_binding` - 完全バインディング

### Week 4: ノード操作
8. `add_widget_graph_node` - ノード追加
9. `connect_widget_nodes` - ノード接続

---

## 必要な include

```cpp
// Animation
#include "Animation/WidgetAnimation.h"
#include "MovieScene.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieScene2DTransformTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Channels/MovieSceneFloatChannel.h"

// Binding
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
// FDelegateRuntimeBinding は WidgetBlueprintGeneratedClass.h に含まれる
```

---

## テストケース

### Animation テスト

```python
# 1. アニメーション作成
create_widget_animation(
    widget_name="WBP_TT_TrapSelector",
    animation_name="FadeIn",
    length=0.5,
    path="/Game/TrapxTrap/UI"
)

# 2. Opacity トラック追加
add_animation_track(
    widget_name="WBP_TT_TrapSelector",
    animation_name="FadeIn",
    target_widget="MainContainer",
    property_name="Opacity",
    path="/Game/TrapxTrap/UI"
)

# 3. キーフレーム追加
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
```

### 配列変数テスト

```python
add_widget_array_variable(
    widget_name="WBP_TT_TrapSelector",
    variable_name="TrapNames",
    element_type="String",
    path="/Game/TrapxTrap/UI"
)
```

---

## 参考資料

- UE5 MovieScene API: `Engine/Source/Runtime/MovieScene/`
- UMG Animation: `Engine/Source/Runtime/UMG/Public/Animation/`
- Widget Blueprint Binding: `Engine/Source/Editor/UMGEditor/`

---

## 次のステップ

1. このドキュメントをレビュー
2. Claude Code で `create_widget_animation` から実装開始
3. 各ツール実装後にテスト

---

**作成日**: 2025-12-22
**フェーズ**: UMG Phase 3 - アニメーション & 高度なバインディング
