# Phase H: AIPerception & EQS ツール実装

## 概要

TrapxTrapCpp の AI 実装に必要な AIPerception と EQS (Environment Query System) の MCP ツールを追加する。

**目標**: AI の知覚設定と環境クエリを MCP 経由で完全に構築可能にし、手動設定を排除する。

---

## Phase H-1: AIPerception ツール

### 新規ツール一覧

| ツール名 | 説明 |
|----------|------|
| `add_ai_perception_component` | Blueprint に AIPerceptionComponent を追加 |
| `configure_sight_sense` | 視覚センス設定 |
| `configure_hearing_sense` | 聴覚センス設定 |
| `configure_damage_sense` | ダメージセンス設定 |
| `set_perception_dominant_sense` | 主要センス設定 |
| `add_perception_stimuli_source` | 被検知側のスティミュラス設定 |

### API 仕様

#### 1. add_ai_perception_component

AIController Blueprint に AIPerceptionComponent を追加。

```python
add_ai_perception_component(
    blueprint_name: str,           # "BP_AIController"
    component_name: str = "AIPerceptionComponent",
    path: str = "/Game/Blueprints"
) -> dict
```

**C++ 実装ポイント**:
```cpp
// AIPerceptionComponent は AIController の特殊メンバー
// SetPerceptionComponent() を使用する必要がある
UAIPerceptionComponent* PerceptionComp = NewObject<UAIPerceptionComponent>(Controller);
Controller->SetPerceptionComponent(*PerceptionComp);
```

**レスポンス**:
```json
{
    "success": true,
    "blueprint_name": "BP_AIController",
    "component_name": "AIPerceptionComponent"
}
```

---

#### 2. configure_sight_sense

視覚センスを設定。

```python
configure_sight_sense(
    blueprint_name: str,
    component_name: str = "AIPerceptionComponent",
    sight_radius: float = 3000.0,          # 検知距離
    lose_sight_radius: float = 3500.0,     # 見失う距離
    peripheral_vision_angle: float = 90.0, # 視野角（片側、度）
    detection_by_affiliation: dict = None, # {"enemies": True, "neutrals": True, "friendlies": False}
    auto_success_range: float = 500.0,     # 自動成功距離
    max_age: float = 5.0,                  # 記憶保持時間
    path: str = "/Game/Blueprints"
) -> dict
```

**C++ 実装ポイント**:
```cpp
#include "Perception/AISenseConfig_Sight.h"

UAISenseConfig_Sight* SightConfig = NewObject<UAISenseConfig_Sight>(PerceptionComp);
SightConfig->SightRadius = SightRadius;
SightConfig->LoseSightRadius = LoseSightRadius;
SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;
SightConfig->DetectionByAffiliation.bDetectEnemies = true;
SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
SightConfig->SetMaxAge(MaxAge);
SightConfig->AutoSuccessRangeFromLastSeenLocation = AutoSuccessRange;

PerceptionComp->ConfigureSense(*SightConfig);
```

**レスポンス**:
```json
{
    "success": true,
    "sense_type": "Sight",
    "sight_radius": 3000.0,
    "peripheral_vision_angle": 90.0
}
```

---

#### 3. configure_hearing_sense

聴覚センスを設定。

```python
configure_hearing_sense(
    blueprint_name: str,
    component_name: str = "AIPerceptionComponent",
    hearing_range: float = 3000.0,
    detection_by_affiliation: dict = None,
    max_age: float = 5.0,
    path: str = "/Game/Blueprints"
) -> dict
```

**C++ 実装ポイント**:
```cpp
#include "Perception/AISenseConfig_Hearing.h"

UAISenseConfig_Hearing* HearingConfig = NewObject<UAISenseConfig_Hearing>(PerceptionComp);
HearingConfig->HearingRange = HearingRange;
HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
HearingConfig->SetMaxAge(MaxAge);

PerceptionComp->ConfigureSense(*HearingConfig);
```

---

#### 4. configure_damage_sense

ダメージセンスを設定（ダメージを受けた時に検知）。

```python
configure_damage_sense(
    blueprint_name: str,
    component_name: str = "AIPerceptionComponent",
    max_age: float = 5.0,
    path: str = "/Game/Blueprints"
) -> dict
```

**C++ 実装ポイント**:
```cpp
#include "Perception/AISenseConfig_Damage.h"

UAISenseConfig_Damage* DamageConfig = NewObject<UAISenseConfig_Damage>(PerceptionComp);
DamageConfig->SetMaxAge(MaxAge);

PerceptionComp->ConfigureSense(*DamageConfig);
```

---

#### 5. set_perception_dominant_sense

主要センスを設定（複数センスがある場合の優先度）。

```python
set_perception_dominant_sense(
    blueprint_name: str,
    component_name: str = "AIPerceptionComponent",
    sense_type: str,  # "Sight", "Hearing", "Damage"
    path: str = "/Game/Blueprints"
) -> dict
```

**C++ 実装ポイント**:
```cpp
// DominantSense を設定
if (SenseType == "Sight")
{
    PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
}
else if (SenseType == "Hearing")
{
    PerceptionComp->SetDominantSense(UAISense_Hearing::StaticClass());
}
```

---

#### 6. add_perception_stimuli_source

被検知側（Player等）にスティミュラスソースを追加。

```python
add_perception_stimuli_source(
    blueprint_name: str,
    component_name: str = "AIPerceptionStimuliSourceComponent",
    register_as_source_for: list = None,  # ["Sight", "Hearing"]
    auto_register: bool = True,
    path: str = "/Game/Blueprints"
) -> dict
```

**C++ 実装ポイント**:
```cpp
#include "Perception/AIPerceptionStimuliSourceComponent.h"

UAIPerceptionStimuliSourceComponent* StimuliSource = NewObject<UAIPerceptionStimuliSourceComponent>(Actor);
StimuliSource->bAutoRegister = AutoRegister;

// センスを登録
for (const FString& SenseType : RegisterAsSourceFor)
{
    if (SenseType == "Sight")
    {
        StimuliSource->RegisterForSense(TSubclassOf<UAISense>(UAISense_Sight::StaticClass()));
    }
    else if (SenseType == "Hearing")
    {
        StimuliSource->RegisterForSense(TSubclassOf<UAISense>(UAISense_Hearing::StaticClass()));
    }
}
```

---

## Phase H-2: EQS ツール

### 新規ツール一覧

| ツール名 | 説明 |
|----------|------|
| `create_eqs_query` | EQS Query アセット作成 |
| `add_eqs_generator` | Generator 追加（Grid, Donut, Actors等） |
| `add_eqs_test` | Test 追加（Distance, Trace, Dot等） |
| `set_eqs_test_property` | Test のプロパティ設定 |
| `list_eqs_assets` | EQS Query 一覧取得 |

### API 仕様

#### 1. create_eqs_query

EQS Query アセットを作成。

```python
create_eqs_query(
    name: str,                    # "EQS_FindCover"
    path: str = "/Game/AI/EQS"
) -> dict
```

**C++ 実装ポイント**:
```cpp
#include "EnvironmentQuery/EnvQuery.h"

// アセット作成
UPackage* Package = CreatePackage(*PackagePath);
UEnvQuery* NewQuery = NewObject<UEnvQuery>(Package, *QueryName, RF_Public | RF_Standalone);

// アセット登録
FAssetRegistryModule::AssetCreated(NewQuery);
NewQuery->MarkPackageDirty();

// 保存
UPackage::SavePackage(Package, NewQuery, RF_Public | RF_Standalone, *PackageFileName);
```

**レスポンス**:
```json
{
    "success": true,
    "asset_path": "/Game/AI/EQS/EQS_FindCover",
    "name": "EQS_FindCover"
}
```

---

#### 2. add_eqs_generator

EQS Query に Generator を追加。

```python
add_eqs_generator(
    query_name: str,
    generator_type: str,  # "SimpleGrid", "Donut", "OnCircle", "ActorsOfClass", "CurrentLocation"
    
    # SimpleGrid 用
    grid_size: float = 1000.0,
    space_between: float = 100.0,
    
    # Donut 用
    inner_radius: float = 300.0,
    outer_radius: float = 1000.0,
    
    # OnCircle 用
    circle_radius: float = 500.0,
    number_of_points: int = 8,
    
    # ActorsOfClass 用
    searched_actor_class: str = None,  # "/Game/Blueprints/BP_CoverPoint.BP_CoverPoint_C"
    
    # 共通
    generate_around: str = "Querier",  # "Querier", "Player", カスタムContext名
    project_down: float = 1000.0,
    project_up: float = 1000.0,
    
    path: str = "/Game/AI/EQS"
) -> dict
```

**C++ 実装ポイント**:
```cpp
#include "EnvironmentQuery/Generators/EnvQueryGenerator_SimpleGrid.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_Donut.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_OnCircle.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ActorsOfClass.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_CurrentLocation.h"

UEnvQueryGenerator* Generator = nullptr;

if (GeneratorType == "SimpleGrid")
{
    UEnvQueryGenerator_SimpleGrid* GridGen = NewObject<UEnvQueryGenerator_SimpleGrid>(Query);
    GridGen->GridSize.DefaultValue = GridSize;
    GridGen->SpaceBetween.DefaultValue = SpaceBetween;
    Generator = GridGen;
}
else if (GeneratorType == "Donut")
{
    UEnvQueryGenerator_Donut* DonutGen = NewObject<UEnvQueryGenerator_Donut>(Query);
    DonutGen->InnerRadius.DefaultValue = InnerRadius;
    DonutGen->OuterRadius.DefaultValue = OuterRadius;
    Generator = DonutGen;
}
// ...

// Query に追加
Query->Options.Add(FEnvQueryOption(Generator));
```

**Generator 種類**:

| タイプ | 説明 | 主要パラメータ |
|--------|------|---------------|
| `SimpleGrid` | グリッド状のポイント | grid_size, space_between |
| `Donut` | ドーナツ状のポイント | inner_radius, outer_radius |
| `OnCircle` | 円周上のポイント | circle_radius, number_of_points |
| `ActorsOfClass` | 指定クラスのアクター | searched_actor_class |
| `CurrentLocation` | 現在位置 | なし |
| `PathingGrid` | ナビメッシュ上のグリッド | grid_size, space_between |

---

#### 3. add_eqs_test

Generator に Test を追加。

```python
add_eqs_test(
    query_name: str,
    generator_index: int = 0,       # 対象 Generator のインデックス
    test_type: str,                 # "Distance", "Trace", "Dot", "PathExists", "GameplayTags"
    
    # Distance 用
    distance_to: str = "Querier",   # Context 名
    
    # Trace 用
    trace_from: str = "Querier",
    trace_to: str = "Item",
    trace_channel: str = "Visibility",  # "Visibility", "Camera", "Custom"
    
    # Dot 用
    line_a_from: str = "Querier",
    line_a_to: str = "Item",
    line_b_direction: str = "Rotation",  # "Rotation", "ToContext"
    line_b_context: str = None,
    
    # 共通
    test_purpose: str = "Score",    # "Score", "Filter"
    filter_type: str = None,        # "Minimum", "Maximum", "Range"
    float_value_min: float = None,
    float_value_max: float = None,
    scoring_equation: str = "Linear",  # "Linear", "Square", "InverseLinear", "Constant"
    scoring_factor: float = 1.0,
    clamp_min: float = None,
    clamp_max: float = None,
    
    path: str = "/Game/AI/EQS"
) -> dict
```

**C++ 実装ポイント**:
```cpp
#include "EnvironmentQuery/Tests/EnvQueryTest_Distance.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Trace.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Dot.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_PathfindingBatch.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_GameplayTags.h"

UEnvQueryTest* Test = nullptr;

if (TestType == "Distance")
{
    UEnvQueryTest_Distance* DistTest = NewObject<UEnvQueryTest_Distance>(Query);
    DistTest->DistanceTo = DistanceToContext;  // UEnvQueryContext
    Test = DistTest;
}
else if (TestType == "Trace")
{
    UEnvQueryTest_Trace* TraceTest = NewObject<UEnvQueryTest_Trace>(Query);
    TraceTest->Context = TraceFromContext;
    TraceTest->TraceData.TraceChannel = TraceChannel;
    Test = TraceTest;
}
// ...

// Test の共通設定
Test->TestPurpose = (TestPurpose == "Score") ? EEnvTestPurpose::Score : EEnvTestPurpose::Filter;
Test->ScoringEquation = GetScoringEquation(ScoringEquation);
Test->ScoringFactor.DefaultValue = ScoringFactor;

// Generator に追加
Query->Options[GeneratorIndex].Tests.Add(Test);
```

**Test 種類**:

| タイプ | 説明 | 用途 |
|--------|------|------|
| `Distance` | 距離計算 | 近い/遠い位置を優先 |
| `Trace` | レイトレース | 視線が通るか |
| `Dot` | 内積計算 | 向いている方向 |
| `PathExists` | パス存在確認 | NavMesh 到達可能 |
| `GameplayTags` | タグ比較 | タグでフィルタ |
| `Overlap` | オーバーラップ | 障害物チェック |

---

#### 4. set_eqs_test_property

Test のプロパティを詳細設定。

```python
set_eqs_test_property(
    query_name: str,
    generator_index: int,
    test_index: int,
    property_name: str,
    property_value: any,
    path: str = "/Game/AI/EQS"
) -> dict
```

---

#### 5. list_eqs_assets

プロジェクト内の EQS Query を一覧取得。

```python
list_eqs_assets(
    path_filter: str = None  # "/Game/AI/EQS/"
) -> dict
```

**レスポンス**:
```json
{
    "success": true,
    "queries": [
        {
            "name": "EQS_FindCover",
            "path": "/Game/AI/EQS/EQS_FindCover",
            "generator_count": 1,
            "test_count": 3
        }
    ],
    "total_count": 1
}
```

---

## 実装ファイル

### 新規作成

| ファイル | 説明 |
|----------|------|
| `SpirrowBridgeAIPerceptionCommands.h` | AIPerception ヘッダ |
| `SpirrowBridgeAIPerceptionCommands.cpp` | AIPerception 実装 |
| `SpirrowBridgeEQSCommands.h` | EQS ヘッダ |
| `SpirrowBridgeEQSCommands.cpp` | EQS 実装 |
| `Python/tools/perception_tools.py` | Perception MCP ツール |
| `Python/tools/eqs_tools.py` | EQS MCP ツール |

### 変更

| ファイル | 変更内容 |
|----------|----------|
| `SpirrowBridge.cpp` | 新コマンドルーティング追加 |
| `SpirrowBridge.Build.cs` | AIModule 依存追加 |
| `Python/unreal_mcp_server.py` | 新ツールインポート |

---

## ビルド依存関係

### SpirrowBridge.Build.cs に追加

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    // 既存...
    "AIModule",           // AI 基本
    "GameplayTasks",      // BT タスク用（既存）
    "NavigationSystem"    // EQS ナビゲーション用
});
```

---

## TrapxTrapCpp での使用例

### AI Controller 設定

```python
# 1. AIController Blueprint 作成
create_blueprint(
    name="BP_TrapxTrapAIController",
    parent_class="AIController",
    path="/Game/TrapxTrap/Blueprints/AI"
)

# 2. AIPerceptionComponent 追加
add_ai_perception_component(
    blueprint_name="BP_TrapxTrapAIController",
    path="/Game/TrapxTrap/Blueprints/AI"
)

# 3. 視覚センス設定
configure_sight_sense(
    blueprint_name="BP_TrapxTrapAIController",
    sight_radius=3000.0,
    lose_sight_radius=3500.0,
    peripheral_vision_angle=75.0,
    detection_by_affiliation={"enemies": True, "neutrals": False, "friendlies": False},
    path="/Game/TrapxTrap/Blueprints/AI"
)

# 4. 聴覚センス設定（射撃音検知用）
configure_hearing_sense(
    blueprint_name="BP_TrapxTrapAIController",
    hearing_range=5000.0,
    path="/Game/TrapxTrap/Blueprints/AI"
)

# 5. 視覚を主要センスに設定
set_perception_dominant_sense(
    blueprint_name="BP_TrapxTrapAIController",
    sense_type="Sight",
    path="/Game/TrapxTrap/Blueprints/AI"
)
```

### Player にスティミュラス追加

```python
add_perception_stimuli_source(
    blueprint_name="BP_PlayerCharacter",
    register_as_source_for=["Sight", "Hearing"],
    path="/Game/TrapxTrap/Blueprints/Characters"
)
```

### EQS: 待ち伏せ位置検索

```python
# 1. EQS Query 作成
create_eqs_query(
    name="EQS_FindAmbushPoint",
    path="/Game/TrapxTrap/AI/EQS"
)

# 2. グリッド Generator 追加
add_eqs_generator(
    query_name="EQS_FindAmbushPoint",
    generator_type="SimpleGrid",
    grid_size=2000.0,
    space_between=200.0,
    generate_around="Querier",
    path="/Game/TrapxTrap/AI/EQS"
)

# 3. プレイヤーへの視線確保 Test
add_eqs_test(
    query_name="EQS_FindAmbushPoint",
    test_type="Trace",
    trace_from="Item",
    trace_to="Player",
    trace_channel="Visibility",
    test_purpose="Filter",
    path="/Game/TrapxTrap/AI/EQS"
)

# 4. プレイヤーから遠い位置を優先
add_eqs_test(
    query_name="EQS_FindAmbushPoint",
    test_type="Distance",
    distance_to="Player",
    test_purpose="Score",
    scoring_equation="Linear",
    scoring_factor=1.0,
    clamp_min=500.0,
    clamp_max=2000.0,
    path="/Game/TrapxTrap/AI/EQS"
)
```

---

## 実装順序

### Phase H-1: AIPerception（1日）

1. `SpirrowBridgeAIPerceptionCommands.h/cpp` 作成
2. `add_ai_perception_component` 実装
3. `configure_sight_sense` 実装
4. `configure_hearing_sense` 実装
5. `configure_damage_sense` 実装
6. `set_perception_dominant_sense` 実装
7. `add_perception_stimuli_source` 実装
8. `perception_tools.py` 作成
9. ルーティング追加・テスト

### Phase H-2: EQS（1日）

1. `SpirrowBridgeEQSCommands.h/cpp` 作成
2. `create_eqs_query` 実装
3. `add_eqs_generator` 実装（SimpleGrid, Donut, OnCircle）
4. `add_eqs_test` 実装（Distance, Trace, Dot）
5. `set_eqs_test_property` 実装
6. `list_eqs_assets` 実装
7. `eqs_tools.py` 作成
8. ルーティング追加・テスト

### Phase H-3: 統合テスト（0.5日）

1. TrapxTrapCpp で AIController 設定テスト
2. EQS Query 作成・実行テスト
3. ドキュメント更新

---

## 注意事項

### AIPerception

1. **AIController 専用**: AIPerceptionComponent は通常 AIController に追加する
2. **センス登録順序**: ConfigureSense() 後に SetDominantSense() を呼ぶ
3. **スティミュラス登録**: 被検知側は BeginPlay で RegisterForSense が必要

### EQS

1. **プラグイン有効化**: Project Settings > Plugins > Environment Query Editor を有効にする必要あり
2. **Context**: 組み込みの Querier, Item に加え、カスタム Context も使用可能
3. **Test 順序**: Filter → Score の順で処理される

---

## エラーコード追加

| コード | 名前 | 説明 |
|--------|------|------|
| 1700 | PERCEPTION_COMPONENT_NOT_FOUND | AIPerceptionComponent が見つからない |
| 1701 | SENSE_CONFIG_FAILED | センス設定失敗 |
| 1702 | INVALID_SENSE_TYPE | 無効なセンスタイプ |
| 1800 | EQS_QUERY_NOT_FOUND | EQS Query が見つからない |
| 1801 | EQS_GENERATOR_FAILED | Generator 追加失敗 |
| 1802 | EQS_TEST_FAILED | Test 追加失敗 |
| 1803 | INVALID_GENERATOR_TYPE | 無効な Generator タイプ |
| 1804 | INVALID_TEST_TYPE | 無効な Test タイプ |
