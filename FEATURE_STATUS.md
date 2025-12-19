# spirrow-unrealwise 機能ステータス

## 概要

このドキュメントは、MCPツールの動作確認状況と今後追加予定の機能をまとめたものです。

---

## 確認済み機能

### Actor操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `get_actors_in_level` | ✅ 動作OK | レベル内の全アクター取得 |
| `find_actors_by_name` | 🔲 未確認 | |
| `spawn_actor` | ✅ 動作OK | アクター作成のみ、メッシュ設定は別途必要 |
| `delete_actor` | 🔲 未確認 | |
| `set_actor_transform` | 🔲 未確認 | |
| `get_actor_properties` | ✅ 動作OK | |
| `set_actor_property` | ✅ 動作OK | アクター自体のプロパティを設定。rationale対応 |
| `set_actor_component_property` | ✅ 動作OK | アクターのコンポーネントのプロパティを設定。rationale対応 |
| `rename_actor` | ✅ 動作OK | アクター名変更（ActorLabel/Name両対応） |
| `get_actor_components` | 🔲 未確認 | アクターのコンポーネント一覧取得 |

### Blueprint操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_blueprint` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `spawn_blueprint_actor` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `add_component_to_blueprint` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `set_static_mesh_properties` | ✅ 動作OK | Engine標準メッシュで確認。pathパラメータ対応 |
| `set_component_property` | 🔲 未確認 | pathパラメータ対応 |
| `set_physics_properties` | 🔲 未確認 | pathパラメータ対応 |
| `compile_blueprint` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `set_blueprint_property` | 🔲 未確認 | pathパラメータ対応 |

### BPノードグラフ操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_blueprint_event_node` | ✅ 動作OK | ReceiveBeginPlay確認。pathパラメータ対応 |
| `add_blueprint_input_action_node` | 🔲 未確認 | pathパラメータ対応 |
| `add_blueprint_function_node` | ✅ 動作OK | target指定が重要（self, KismetSystemLibrary等）。pathパラメータ対応 |
| `connect_blueprint_nodes` | ✅ 動作OK | ピン名: then → execute。pathパラメータ対応 |
| `add_blueprint_variable` | 🔲 未確認 | pathパラメータ対応 |
| `add_blueprint_get_self_component_reference` | 🔲 未確認 | pathパラメータ対応 |
| `add_blueprint_self_reference` | 🔲 未確認 | pathパラメータ対応 |
| `find_blueprint_nodes` | 🔲 未確認 | pathパラメータ対応 |

### UMG Widget操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_umg_widget_blueprint` | 🔲 未確認 | Widget Blueprint作成 |
| `add_text_block_to_widget` | 🔲 未確認 | |
| `add_button_to_widget` | 🔲 未確認 | |
| `bind_widget_event` | 🔲 未確認 | |
| `add_widget_to_viewport` | 🔲 未確認 | |
| `set_text_block_binding` | 🔲 未確認 | |
| `add_text_to_widget` | ✅ 実装完了 | Text要素追加、アンカー・フォントサイズ・色設定対応 |
| `add_image_to_widget` | ✅ 実装完了 | Image要素追加、テクスチャ・サイズ・色調設定対応 |
| `add_progressbar_to_widget` | ✅ 実装完了 | ProgressBar追加、パーセント・色・背景色設定対応 |

### アセット管理

| ツール | 状態 | 備考 |
|--------|------|------|
| `delete_asset` | ✅ 動作OK | Content Browserからアセット削除 |
| `duplicate_blueprint` | ✅ 実装完了 | Blueprint複製、カスタムパス対応 |
| `get_blueprint_graph` | ✅ 実装完了 | Blueprintのノードグラフ構成取得、ノード・接続・変数・コンポーネント情報 |
| `rename_asset` | ✅ 実装完了 | Content Browser内のアセットをリネーム、参照自動更新 |

### その他

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_input_mapping` | 🔲 未確認 | |

### プロジェクト設定（Config）

| ツール | 状態 | 備考 |
|--------|------|------|
| `get_config_value` | ✅ 実装完了 | プロジェクト設定値の取得 |
| `set_config_value` | ✅ 実装完了 | プロジェクト設定値の変更 |
| `list_config_sections` | ✅ 実装完了 | Config ファイルのセクション一覧取得 |

### GAS（Gameplay Ability System）

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_gameplay_tags` | ✅ 実装完了 | Gameplay Tags の追加（バッチ対応、コメント設定可能） |
| `list_gameplay_tags` | ✅ 実装完了 | Gameplay Tags の一覧取得（プレフィックスフィルタ対応） |
| `remove_gameplay_tag` | ✅ 実装完了 | Gameplay Tag の削除 |
| `list_gas_assets` | ✅ 実装完了 | GAS関連アセット一覧取得（Effect/Ability/Cue/AttributeSet） |

### RAG連携

| ツール | 状態 | 備考 |
|--------|------|------|
| `search_knowledge` | ✅ 動作OK | RAGサーバー連携、意味検索対応 |
| `add_knowledge` | ✅ 動作OK | ナレッジ追加、カテゴリ・タグ対応 |
| `list_knowledge` | ✅ 動作OK | 登録済みナレッジ一覧取得 |
| `delete_knowledge` | ✅ 動作OK | ID指定でナレッジ削除 |

### Material（マテリアル）

| ツール | 状態 | 備考 |
|--------|------|------|
| `list_material_templates` | ✅ 実装完了 | ビルトイン＆ユーザー定義テンプレート一覧取得 |
| `get_material_template` | ✅ 実装完了 | 指定したテンプレートの詳細取得 |
| `save_material_template` | ✅ 実装完了 | ユーザー定義テンプレートをRAGに保存 |
| `delete_material_template` | ✅ 実装完了 | ユーザー定義テンプレートを削除 |
| `create_material_from_template` | ✅ 実装完了 | テンプレートベースのマテリアル作成、パラメータ上書き対応 |
| `create_simple_material` | ✅ 実装完了 | 詳細設定によるマテリアル作成 |

### ナレッジアシスタント

| ツール | 状態 | 備考 |
|--------|------|------|
| `find_relevant_nodes` | ✅ 動作OK | RAG検索+プロジェクトクラススキャン統合、日英対応 |
| `scan_project_classes` | ✅ 動作OK | C++/BP一覧取得、親クラス・モジュール・タイプフィルタ対応 |

---

## 確認された制限事項

1. **spawn_actor**: アクター作成のみ。StaticMeshの設定は別途Blueprint経由が必要

---

## 追加予定機能

### Phase 1: ナレッジアシスタント（完了）

目的: 「やりたいこと → 使うべきノード/クラス」の逆引き支援

| ツール | 説明 | 状態 |
|--------|------|------|
| `find_relevant_nodes` | 目的からBPノード/C++クラスを検索、RAG統合 | ✅ 完了 |
| `scan_project_classes` | プロジェクト内のクラス/BP一覧取得 | ✅ 完了 |
| `explain_node` | ノード/クラスの詳細解説 | 📋 計画中 |

### Phase 2: RAG統合（完了）

目的: プロジェクト固有のナレッジ蓄積と検索

| ツール | 説明 | 状態 |
|--------|------|------|
| `search_knowledge` | 蓄積されたナレッジの検索 | ✅ 完了 |
| `add_knowledge` | ナレッジの追加（カテゴリ・タグ対応） | ✅ 完了 |
| `list_knowledge` | 全ナレッジの一覧表示 | ✅ 完了 |
| `delete_knowledge` | ナレッジの削除 | ✅ 完了 |

### Phase 3: 既存機能の改善（完了）

| 項目 | 説明 | 状態 |
|------|------|------|
| `spawn_blueprint_actor` の修正 | pathパラメータ対応、通信問題解決 | ✅ 完了 |
| `set_actor_property` 分離 | アクター用とコンポーネント用に分離、rationale対応 | ✅ 完了 |

### Phase 4: 追加ツール（優先度低）

| ツール | 説明 | 状態 |
|--------|------|------|
| `get_blueprint_graph` | 既存BPのノード構成取得 | ✅ 完了 |
| `duplicate_blueprint` | BP複製 | ✅ 完了 |
| `rename_actor` | アクター名変更 | ✅ 完了 |

---

## 最新の更新履歴

### 2025-12-20: Material Tools 実装 - 2層テンプレートシステム

**新機能**:
- マテリアル作成ツールの実装（6つのMCPツール）
  - `list_material_templates`: ビルトイン＆ユーザー定義テンプレート一覧
  - `get_material_template`: テンプレート詳細取得
  - `save_material_template`: ユーザー定義テンプレートをRAGに保存
  - `delete_material_template`: ユーザー定義テンプレートを削除
  - `create_material_from_template`: テンプレートベースでマテリアル作成
  - `create_simple_material`: 詳細設定でマテリアル作成

**2層テンプレートシステム**:
- **Layer 1 - ビルトインテンプレート**: JSONファイルで定義（templates/materials/）
  - `solid.json` - 基本的な不透明マテリアル
  - `translucent.json` - 半透明マテリアル（透明度調整可能）
  - `unlit.json` - ライティング影響を受けない不透明マテリアル
  - `unlit_translucent.json` - ライティング影響を受けない半透明マテリアル（エフェクト、検出範囲表示用）
  - `emissive.json` - 発光マテリアル（強度調整可能）
- **Layer 2 - ユーザー定義テンプレート**: RAG（ChromaDB）に保存
  - プロジェクト固有のマテリアルパターンを蓄積
  - 意味検索による関連テンプレート検索
  - カテゴリ・タグによる分類

**RAG内部関数**:
- 非同期ツールから同期的にRAGアクセス可能にする内部関数を追加
  - `search_knowledge_internal`: 同期版RAG検索
  - `add_knowledge_internal`: 同期版ナレッジ追加
  - `delete_knowledge_internal`: 同期版ナレッジ削除

**マテリアル作成機能**:
- Shading Model: DefaultLit（標準）、Unlit（ライティングなし）
- Blend Mode: Opaque（不透明）、Translucent（半透明）、Masked（マスク）
- Base Color、Emissive Color、Opacity設定
- Two Sided（両面描画）対応
- MaterialExpressionノード自動作成

**使用例**:
```python
# テンプレート一覧取得
list_material_templates()

# テンプレートからマテリアル作成（パラメータ上書き）
create_material_from_template(
    template_name="unlit_translucent",
    name="M_DetectionSphere",
    path="/Game/Materials",
    overrides={
        "base_color": [0.0, 1.0, 0.0],  # 緑色
        "opacity": 0.3
    }
)

# 詳細設定でマテリアル作成
create_simple_material(
    name="M_Glow",
    path="/Game/Materials",
    shading_model="Unlit",
    blend_mode="Translucent",
    emissive_color=[1.0, 0.5, 0.0],
    emissive_strength=5.0,
    opacity=0.8
)

# カスタムテンプレートをRAGに保存
save_material_template(
    name="glass",
    description="透明ガラスマテリアル",
    template_data={
        "shading_model": "DefaultLit",
        "blend_mode": "Translucent",
        "base_color": [0.9, 0.95, 1.0],
        "opacity": 0.2,
        "two_sided": true
    },
    category="materials"
)
```

**変更範囲**:
- Python tools/material_tools.py: 新規作成（6 MCPツール、446行）
- Python tools/rag_tools.py: RAG内部関数追加（search/add/delete_knowledge_internal）
- Python unreal_mcp_server.py: register_material_tools追加
- C++ SpirrowBridgeMaterialCommands.h/.cpp: 新規作成（HandleCreateSimpleMaterial実装）
- C++ SpirrowBridge.h/.cpp: MaterialCommandsメンバー追加、ルーティング追加
- templates/materials/*.json: 5つのビルトインテンプレート追加

**技術詳細**:
- UMaterialFactoryNew による Material 生成
- MaterialExpressionConstant3Vector（BaseColor/EmissiveColor）
- MaterialExpressionConstant（Opacity）
- GetEditorOnlyData()->BaseColor/EmissiveColor/Opacity への接続
- MSM_Unlit/MSM_DefaultLit によるシェーディングモデル設定
- BLEND_Opaque/BLEND_Translucent/BLEND_Masked によるブレンドモード設定

### 2025-12-20: UMG Widget Tools 拡張 - Text/Image/ProgressBar追加

**新機能**:
- UMG Widget操作ツールを3つ追加
  - `add_text_to_widget`: Text要素追加（アンカー・フォントサイズ・色設定対応）
  - `add_image_to_widget`: Image要素追加（テクスチャ・サイズ・色調設定対応）
  - `add_progressbar_to_widget`: ProgressBar追加（パーセント・色・背景色設定対応）

**9ポジションアンカーシステム**:
- 全ツールで統一された9つのアンカープリセット対応
  - TopLeft, TopCenter, TopRight
  - CenterLeft, Center, CenterRight
  - BottomLeft, BottomCenter, BottomRight

**ProgressBar機能詳細**:
- Fill Color（進捗バーの色）設定
- Background Color（背景色）設定
- Percent（初期値）設定
- WidgetStyle による色設定

**変更範囲**:
- Python tools/umg_tools.py: 3ツール追加（add_text_to_widget, add_image_to_widget, add_progressbar_to_widget）
- C++ SpirrowBridgeUMGCommands.h/.cpp: 3ハンドラ実装
- C++ SpirrowBridge.cpp: ルーティング追加

### 2025-12-16: Widget Blueprint 対応 - create_blueprint で UUserWidget 親クラスをサポート

**新機能**:
- `create_blueprint` コマンドで UUserWidget を親クラスとして指定可能に
- Widget Blueprint の自動生成に対応
- UUserWidget 派生クラス（カスタムWidget基底クラス）にも対応

**親クラス検索の強化**:
- Method 1: UserWidget および UUserWidget の直接参照に対応
- Method 2: U プレフィックス付きクラスの TObjectIterator 検索を追加
- Method 3: LoadClass<AActor> から LoadObject<UClass> への変更により、非Actor系クラスをサポート
- /Script/UMG モジュールパスの追加

**Blueprint タイプ判定**:
- 親クラスが UUserWidget を継承している場合、UWidgetBlueprintFactory を使用
- それ以外の場合、従来通り UBlueprintFactory を使用

**使用例**:
```python
# UUserWidget ベースの Widget Blueprint を作成
create_blueprint(
    name="WBP_MainMenu",
    parent_class="UserWidget",  # または "UUserWidget"
    path="/Game/UI"
)

# カスタム Widget 基底クラスから派生
create_blueprint(
    name="WBP_Crosshair",
    parent_class="CrosshairWidget",  # プロジェクト内の UUserWidget 派生クラス
    path="/Game/UI/HUD"
)
```

**変更範囲**:
- C++ SpirrowBridgeBlueprintCommands.cpp: HandleCreateBlueprint 関数を更新
  - 親クラス検索ロジックの拡張（U プレフィックス対応）
  - Widget Blueprint 作成分岐の追加
- インクルード追加: Blueprint/WidgetBlueprint.h, WidgetBlueprintFactory.h
- Build.cs: UMGEditor モジュール依存は既に存在（変更不要）

**技術詳細**:
- UWidgetBlueprintFactory による Widget Blueprint 生成
- IsChildOf(UUserWidget::StaticClass()) による型判定
- 複数の検索メソッド（StaticClass、TObjectIterator、LoadObject）による堅牢な親クラス解決

**下位互換性**:
- Actor系 Blueprint の作成動作は変更なし
- 既存コードへの影響なし

### 2025-12-15: GAS Phase 1-B - GAS アセット一覧取得機能

**新機能**:
- GAS関連アセットの一覧取得機能を追加
  - `list_gas_assets`: プロジェクト内のGameplayEffect、GameplayAbility、GameplayCue、AttributeSetを検索

**対応アセットタイプ**:
- GameplayEffect Blueprints
- GameplayAbility Blueprints
- GameplayCue actors and notifies
- AttributeSet classes

**主な機能**:
- アセットタイプフィルタリング（effect/ability/cue/attribute_set/all）
- コンテンツパスフィルタリング（再帰的検索）
- クラス階層走査によるGASアセット識別
- 各アセットの詳細情報取得（名前、パス、クラス、タイプ、親クラス）

**使用例**:
```python
# 全GASアセット一覧
list_gas_assets()

# GameplayEffectのみ
list_gas_assets(asset_type="effect")

# 特定フォルダ配下
list_gas_assets(asset_type="ability", path_filter="/Game/Abilities/")
```

**変更範囲**:
- Python gas_tools.py: list_gas_assets ツール追加
- C++ SpirrowBridgeGASCommands: HandleListGASAssets 実装
- C++ SpirrowBridge: コマンドルーティング更新
- C++ SpirrowBridge.Build.cs: GameplayAbilities モジュール依存追加

**技術詳細**:
- AssetRegistry を使用したBlueprint検索
- クラス階層走査による型判定（Contains チェック）
- アセットロードによる親クラス取得

### 2025-12-15: GAS Phase 1-A - Gameplay Tags 管理機能

**新機能**:
- Gameplay Tags の管理機能を追加（DefaultGameplayTags.ini 操作）
  - `add_gameplay_tags`: 複数のタグを一括追加、コメント設定対応
  - `list_gameplay_tags`: 全タグの一覧取得、プレフィックスフィルタ対応
  - `remove_gameplay_tag`: 指定したタグを削除

**INI フォーマット対応**:
- `+GameplayTagList=(Tag="TagName",DevComment="Comment")` 形式の解析と書き込み
- 既存タグの保持（追加時の重複チェック）
- セクション `[/Script/GameplayTags.GameplayTagsSettings]` の自動作成

**主な用途**:
- GAS（Gameplay Ability System）の Gameplay Tags 定義
- タグの階層構造管理（例: `Ability.Attack.Melee`, `Ability.Attack.Range`）
- タグへのコメント追加による設計意図の記録

**使用例**:
```python
# タグを追加（コメント付き）
add_gameplay_tags(
    tags=["Ability.Attack.Melee", "Ability.Attack.Range", "State.Dead"],
    comments={
        "Ability.Attack.Melee": "近接攻撃アビリティ",
        "Ability.Attack.Range": "遠距離攻撃アビリティ",
        "State.Dead": "死亡状態タグ"
    }
)

# プレフィックスでフィルタして一覧取得
list_gameplay_tags(filter_prefix="Ability.Attack")

# タグを削除
remove_gameplay_tag(tag="State.Dead")
```

**変更範囲**:
- C++ SpirrowBridgeGASCommands: 新規作成（ヘッダー・実装）
- C++ SpirrowBridge: ルーティング追加（ExecuteCommand、メンバー変数、初期化・クリーンアップ）
- Python gas_tools.py: 新規作成
- Python unreal_mcp_server.py: register_gas_tools追加

**技術詳細**:
- INI ファイル直接操作（FFileHelper による読み書き）
- 正規表現を使わない文字列パース（Tag/DevComment 抽出）
- 既存タグの TSet による重複管理

### 2025-12-15: Config（ini）ファイル操作対応

**新機能**:
- プロジェクト設定ファイル（.ini）の読み書きをMCP経由で実行可能に
  - `get_config_value`: 設定値の取得
  - `set_config_value`: 設定値の変更
  - `list_config_sections`: セクション一覧取得

**対応ファイル**:
- DefaultEngine.ini
- DefaultGame.ini
- DefaultEditor.ini
- DefaultInput.ini

**主な用途**:
- デフォルトGameModeの設定
- マップ設定の変更
- プロジェクト設定の自動化

**使用例**:
```python
# デフォルトGameModeを設定
set_config_value(
    section="/Script/EngineSettings.GameMapsSettings",
    key="GlobalDefaultGameMode",
    value="/Game/MyGame/BP_GameMode.BP_GameMode_C"
)

# 現在の設定を取得
get_config_value(
    section="/Script/EngineSettings.GameMapsSettings",
    key="GlobalDefaultGameMode"
)

# セクション一覧を取得
list_config_sections(config_file="DefaultEngine")
```

**変更範囲**:
- C++ SpirrowBridgeConfigCommands: 新規作成（ヘッダー・実装）
- C++ SpirrowBridge: ルーティング追加（ExecuteCommand、メンバー変数）
- Python config_tools.py: 新規作成
- Python unreal_mcp_server.py: register_config_tools追加

**注意事項**:
- Blueprintクラスをパスとして設定する場合は `_C` サフィックスが必要
- 一部設定はエディタ再起動が必要

### 2025-12-15: ObjectProperty（アセット参照）対応

**新機能**:
- `SetObjectProperty`関数にアセット参照プロパティ対応を追加
  - `FObjectProperty`: `TObjectPtr<T>` 型（例: `UInputAction`, `UStaticMesh`）
  - `FSoftObjectProperty`: `TSoftObjectPtr<T>` 型（ソフト参照）
  - `FClassProperty`: `TSubclassOf<T>` 型（クラス参照）

**機能詳細**:
- アセットパス指定でのプロパティ設定
- 自動パス補完（`.AssetName` サフィックス自動追加）
- 型検証（期待されるクラスとの互換性確認）
- 空文字列またはnullで参照をクリア可能
- 詳細なエラーメッセージ（型不一致、ロード失敗時）

**対応ツール**:
- `set_blueprint_property`
- `set_actor_property`
- `set_actor_component_property`
- `set_component_property`

**使用例**:
```python
# Input Mapping Context の設定
set_blueprint_property(
    blueprint_name="BP_PlayerCharacter",
    property_name="DefaultMappingContext",
    property_value="/Game/Input/IMC_Default.IMC_Default"
)

# Input Action の設定
set_blueprint_property(
    blueprint_name="BP_PlayerCharacter",
    property_name="MoveAction",
    property_value="/Game/Input/IA_Move.IA_Move"
)

# 参照をクリア
set_blueprint_property(
    blueprint_name="BP_PlayerCharacter",
    property_name="MoveAction",
    property_value=""  # または null
)
```

**変更範囲**:
- C++ SpirrowBridgeCommonUtils.cpp: SetObjectProperty関数に3つのプロパティタイプ対応を追加

### 2025-12-15: get_blueprint_graph ツール追加

**新機能**:
- `get_blueprint_graph`: 既存Blueprintのノードグラフ構成を取得
  - ノード一覧（ID、タイプ、クラス、タイトル、座標、ピン情報）
  - ノード間の接続情報（ソース/ターゲットのノードID・ピン名）
  - 変数一覧（名前、型、公開設定）
  - コンポーネント一覧（名前、クラス）
  - 親クラス情報
  - pathパラメータ対応（デフォルト: /Game/Blueprints）

**ノードタイプ判定**:
- Event（K2Node_Event）
- Function（K2Node_CallFunction）
- VariableGet/VariableSet（K2Node_VariableGet/Set）
- Other（その他のノード）

**変更範囲**:
- Python blueprint_tools.py: get_blueprint_graph追加
- C++ SpirrowBridgeBlueprintCommands: HandleGetBlueprintGraph実装
- ルーティング追加: HandleCommand、SpirrowBridge.cpp ExecuteCommand

### 2025-12-15: duplicate_blueprint & rename_actor ツール追加

**新機能**:
- `duplicate_blueprint`: 既存Blueprintを複製する機能
  - ソース・複製先のパス指定対応（デフォルト: /Game/Blueprints）
  - AssetToolsモジュールを使用した安全な複製
  - 既存アセットチェックとエラーハンドリング
- `rename_actor`: レベル内のアクター名を変更する機能
  - ActorLabelとNameの両方で検索

**変更範囲**:
- Python blueprint_tools.py: duplicate_blueprint追加
- Python editor_tools.py: rename_actor追加
- C++ SpirrowBridgeBlueprintCommands: HandleDuplicateBlueprint実装、AssetToolsモジュール追加
- C++ SpirrowBridgeEditorCommands: HandleRenameActor実装
- ルーティング追加: SpirrowBridge.cpp ExecuteCommand更新

### 2025-12-15: set_actor_property 分離 & rationale 自動蓄積機能

**新機能**:
- `set_actor_component_property`: アクターのコンポーネントプロパティ設定用の専用ツール
- `rationale` パラメータ: 設計判断の自動RAG蓄積機能
  - 対応ツール: create_blueprint, add_component_to_blueprint, set_physics_properties, spawn_actor, add_blueprint_event_node, add_blueprint_function_node, add_blueprint_variable, set_actor_property, set_actor_component_property

**変更**:
- `set_actor_property`: コンポーネント指定機能を分離、アクター自体のプロパティ専用に
- RAGサーバー接続: 環境変数 `RAG_SERVER_URL` で設定可能

**変更範囲**:
- Python editor_tools.py: set_actor_property分離、set_actor_component_property追加
- Python blueprint_tools.py: rationaleパラメータ追加（3ツール）
- Python node_tools.py: rationaleパラメータ追加（3ツール）
- Python rag_tools.py: record_rationale関数追加
- AGENTS.md: rationale使用ガイド追加

### 2025-12-14: SSE Transport サポート & spawn_blueprint_actor 修正

**新機能**:
- SSE（Server-Sent Events）トランスポートサポート追加
  - 開発時のMCPサーバー再起動が容易に（Claude Desktop再起動不要）
  - `--sse` オプションでSSEモード起動
  - `--port=XXXX` でカスタムポート指定可能
- SSE用起動スクリプト追加
  - `start_mcp_server_sse.bat`（コマンドプロンプト用）
  - `start_mcp_server_sse.ps1`（PowerShell用）

**バグ修正**:
- `spawn_blueprint_actor`: pathパラメータがC++側で無視されていた問題を修正
- `create_input_mapping_context`: 既存アセットチェック追加（UEクラッシュ防止）

**変更範囲**:
- Python unreal_mcp_server.py: SSE/stdioトランスポート切り替え対応
- C++ SpirrowBridgeEditorCommands.cpp: HandleSpawnBlueprintActorにpathパラメータ追加
- C++ SpirrowBridgeProjectCommands.cpp: HandleCreateInputMappingContextに既存チェック追加
- 新規: start_mcp_server_sse.bat, start_mcp_server_sse.ps1
- README.md: SSEモードのドキュメント追加
- FEATURE_STATUS.md: ステータス更新

### 2025-12-14: ナレッジアシスタント実装 & バグ修正

**新機能**:
- `scan_project_classes`: プロジェクト内C++クラス/Blueprint一覧取得
  - 親クラス・モジュール・パスフィルタ対応
  - Blueprintタイプフィルタ（Actor/Widget/Anim/Interface等）
  - REINST_*クラス（Live Coding一時クラス）自動除外
- `find_relevant_nodes`: やりたいことから関連ノード/クラスを検索
  - RAGナレッジ検索とプロジェクトクラススキャンを統合
  - 日本語・英語キーワード自動抽出（30+マッピング）
  - スマート親クラスフィルタリング

**バグ修正**:
- `find_relevant_nodes`: async関数を同期関数に変更（httpx → requests）
- `find_relevant_nodes`: `scan_project_classes`レスポンス解析を修正
- `create_input_action`: 既存アセットチェック追加（UEクラッシュ防止）

**変更範囲**:
- C++ SpirrowBridgeBlueprintCommands: `HandleScanProjectClasses`実装
- C++ SpirrowBridgeProjectCommands: `HandleCreateInputAction`に既存チェック
- Python knowledge_tools.py: 新規作成（UEクラスマッピング、find_relevant_nodes実装）
- Python pyproject.toml: httpx依存追加（後にrequests使用に変更）
- README.md: ナレッジアシスタント機能セクション追加

### 2025-12-14: 全Blueprint関連ツールにpathパラメータ追加

全てのBlueprint関連ツール（26個のツール）にpathパラメータを追加し、カスタムフォルダでのBlueprint操作を可能にしました。

**変更範囲**:
- C++共通ユーティリティ: FindBlueprint/FindBlueprintByName関数にpath引数追加
- C++ Blueprint Commands: 6ハンドラ更新
- C++ Blueprint Node Commands: 8ハンドラ更新
- Python blueprint_tools.py: 4ツール更新
- Python node_tools.py: 8ツール更新

**デフォルト動作**: pathパラメータ省略時は従来通り `/Game/Blueprints` を使用するため、既存コードとの互換性を保持

---

## テスト環境

- **Unreal Engine**: 5.5+
- **プロジェクト**: TrapxTrapCpp
- **RAGサーバー**: AIサーバー :8100
- **Embedding**: BGE-M3
- **ベクトルDB**: ChromaDB
- **トランスポート**: stdio（デフォルト）/ SSE（開発用）
- **起動スクリプト**: start_mcp_server.ps1 / start_mcp_server.bat
- **設定管理**: config.local.ps1 / config.local.bat（環境固有設定）
- **最終確認日**: 2025-12-15

---

## 凡例

| 記号 | 意味 |
|------|------|
| ✅ | 動作確認済み |
| ⚠️ | 制限あり/部分的に動作 |
| ❌ | 動作せず/要修正 |
| 🔲 | 未確認 |
| 📋 | 計画中 |
| 💡 | アイデア段階 |
