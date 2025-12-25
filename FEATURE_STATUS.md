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

### ノード操作（基本）

| ツール | 状態 | 備考 |
|--------|------|------|
| `set_node_pin_value` | ✅ 動作OK | ノードピンのデフォルト値設定 |
| `add_variable_get_node` | ✅ 動作OK | 変数Getノード追加 |
| `add_variable_set_node` | ✅ 動作OK | 変数Setノード追加 |
| `add_branch_node` | ✅ 動作OK | Branch（if/else）ノード追加 |
| `delete_blueprint_node` | ✅ 動作OK | ノード削除 |
| `move_blueprint_node` | ✅ 動作OK | ノード位置移動 |

### ノード操作（制御フロー・ユーティリティ）🆕

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_sequence_node` | ✅ 動作OK | Sequenceノード追加（2-10出力対応）|
| `add_delay_node` | ✅ 動作OK | Delayノード追加（秒指定）|
| `add_print_string_node` | ✅ 動作OK | PrintStringノード追加（デバッグ用）|
| `add_foreach_loop_node` | ⚠️ 未対応 | マクロ実装のため別アプローチが必要 |
| `add_math_node` | ⚠️ 未対応 | UK2Node_CommutativeAssociativeBinaryOperator実装が必要 |
| `add_comparison_node` | ⚠️ 未対応 | 同上 |

### UMG Widget操作

#### Phase 1: Designer操作 (11ツール)

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_umg_widget_blueprint` | ✅ 実装完了 | Widget Blueprint作成 |
| `add_text_to_widget` | ✅ 実装完了 | Text要素追加、アンカー・フォントサイズ・色設定対応 |
| `add_image_to_widget` | ✅ 実装完了 | Image要素追加、テクスチャ・サイズ・色調設定対応 |
| `add_progressbar_to_widget` | ✅ 実装完了 | ProgressBar追加、パーセント・色・背景色設定対応 |
| `add_vertical_box_to_widget` | ✅ 実装完了 | VerticalBox追加 |
| `add_horizontal_box_to_widget` | ✅ 実装完了 | HorizontalBox追加 |
| `get_widget_elements` | ✅ 実装完了 | 要素一覧取得 |
| `set_widget_slot_property` | ✅ 実装完了 | Canvas Slot設定 |
| `set_widget_element_property` | ✅ 実装完了 | 要素プロパティ設定 |
| `reparent_widget_element` | ✅ 実装完了 | 親変更 |
| `remove_widget_element` | ✅ 実装完了 | 要素削除 |

#### Phase 2: 変数・関数操作 (5ツール)

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_widget_variable` | ✅ 実装完了 | 変数追加（各種型対応） |
| `set_widget_variable_default` | ✅ 実装完了 | デフォルト値設定 |
| `add_widget_function` | ✅ 実装完了 | 関数作成 |
| `add_widget_event` | ✅ 実装完了 | イベント作成 |
| `bind_widget_to_variable` | ✅ 実装完了 | バインディング関数作成 |

#### Phase 3: Animation (4ツール)

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_widget_animation` | ✅ 実装完了 | アニメーション作成 |
| `add_animation_track` | ✅ 実装完了 | トラック追加（7プロパティ対応） |
| `add_animation_keyframe` | ✅ 実装完了 | キーフレーム追加（Linear/Cubic/Constant） |
| `get_widget_animations` | ✅ 実装完了 | アニメーション一覧取得 |

#### Phase 3: Array Variables (1ツール)

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_widget_array_variable` | ✅ 実装完了 | 配列型変数追加（TArray<T>） |

#### Phase 4-A: Interactive Widgets (4ツール)

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_button_to_widget` | ✅ 実装完了 | 新API（anchor/alignment/path対応）、V2内部コマンド |
| `bind_widget_component_event` | ✅ 実装完了 | イベントバインディング（OnClicked/OnHovered等） |
| `add_slider_to_widget` | ✅ 実装完了 | Slider追加（value/min/max/orientation対応） |
| `add_checkbox_to_widget` | ✅ 実装完了 | CheckBox追加（label_text対応、HorizontalBoxコンテナ） |

#### Phase 4-B: Additional Interactive Widgets (4ツール) 🆕

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_combobox_to_widget` | ✅ 実装完了 | ドロップダウン選択（options/selected_index対応） |
| `add_editabletext_to_widget` | ✅ 実装完了 | テキスト入力（hint_text/password/multiline対応） |
| `add_spinbox_to_widget` | ✅ 実装完了 | 数値入力（min/max/delta対応） |
| `add_scrollbox_to_widget` | ✅ 実装完了 | スクロールコンテナ（orientation/visibility対応） |

#### 旧API（参考）

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_text_block_to_widget` | 🔲 未確認 | 旧API |
| `bind_widget_event` | 🔲 未確認 | 旧API |
| `add_widget_to_viewport` | 🔲 未確認 | 旧API |
| `set_text_block_binding` | 🔲 未確認 | 旧API |

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

### 2025-12-25: 制御フロー・ユーティリティノードツール追加 🆕

**完了機能**:
- Sequenceノード、Delayノード、PrintStringノードの追加
- ノードツール合計20個

**実装済みツール**:
- `add_sequence_node` - 複数実行ブランチの順次実行（2-10出力対応）
- `add_delay_node` - 遅延実行（秒指定）
- `add_print_string_node` - デバッグ出力（メッセージ指定）

**未実装（別アプローチが必要）**:
- `add_foreach_loop_node` - Blueprintマクロとして実装されているため
- `add_math_node` - UK2Node_CommutativeAssociativeBinaryOperator使用が必要
- `add_comparison_node` - 同上

**Sequenceノードのピン名**:
- 入力: `execute`
- 出力: `then_0`, `then_1`, `then_2`, ... （アンダースコア区切り）

**ドキュメント**:
- `Docs/Tools/node_tools.md` - Control Flow & Utility Toolsセクション追加
- `Docs/Tools/README.md` - v0.6.0更新

---

### 2025-12-25: UMG Phase 4-B 完了 - 29ツール（Additional Interactive Widgets 4ツール追加）

**完了機能**:
- UMG Widget Blueprint 操作ツール Phase 4-B 実装完了
- 合計29ツール実装

**Phase 4-B: Additional Interactive Widgets (4ツール)**:
- `add_combobox_to_widget` - ドロップダウン選択（UComboBoxString使用）
- `add_editabletext_to_widget` - テキスト入力フィールド（UEditableTextBox使用）
- `add_spinbox_to_widget` - 数値入力（+/-ボタン付き）
- `add_scrollbox_to_widget` - スクロール可能コンテナ

**テスト結果**:
- add_combobox_to_widget ✅ DifficultyCombo (TopCenter、4オプション、Normal選択)
- add_editabletext_to_widget ✅ PlayerNameInput (Center、プレースホルダー付き)
- add_spinbox_to_widget ✅ VolumeSpinBox (BottomCenter、0-100範囲、ステップ5)
- add_scrollbox_to_widget ✅ LogScrollBox (MiddleRight、縦スクロール)

**ドキュメント**:
- `Docs/UMGPhase4B_Prompt.md` - 実装プロンプト

**次の実装候補（Phase 4-C/5）**:
- ListView（動的リスト表示）
- TileView（タイル表示）
- Border（背景付きコンテナ）
- Overlay（重ね合わせコンテナ）

---

### 2025-12-25: UMG Phase 4-A 完了 - 25ツール（Interactive Widgets 4ツール追加）

**完了機能**:
- UMG Widget Blueprint 操作ツール Phase 4-A 実装完了
- 合計25ツール実装

**Phase 4-A: Interactive Widgets (4ツール)**:
- `add_button_to_widget` - 新API Button追加（anchor/alignment/path対応）
- `bind_widget_component_event` - イベントバインディング（OnClicked/OnPressed/OnReleased/OnHovered/OnUnhovered/OnValueChanged）
- `add_slider_to_widget` - Slider追加（value/min_value/max_value/step_size/orientation対応）
- `add_checkbox_to_widget` - CheckBox追加（label_text対応、HorizontalBoxコンテナ自動生成）

**サポートするイベントタイプ**:
| イベント | 対応ウィジェット | 説明 |
|----------|----------------|------|
| `OnClicked` | Button | クリック時 |
| `OnPressed` | Button | 押下時 |
| `OnReleased` | Button | リリース時 |
| `OnHovered` | Button | ホバー開始 |
| `OnUnhovered` | Button | ホバー終了 |
| `OnValueChanged` | Slider, CheckBox | 値変更時 |

**テスト結果**:
- add_slider_to_widget ✅ VolumeSlider (BottomCenter配置)
- add_checkbox_to_widget ✅ MuteCheckbox + Label (HorizontalBoxコンテナ)
- add_button_to_widget (V2) ✅ StartButton + Text (Center配置)
- bind_widget_component_event ✅ OnClicked → HandleStartButtonClicked

**修正内容**:
- SpirrowBridge.cpp: Phase 4-A コマンドルーティング追加
- umg_tools.py: 旧add_button_to_widget定義削除（重複解消）

**ドキュメント**:
- `Docs/UMGPhase4A_Prompt.md` - 実装プロンプト
- `Docs/UMGPhase4A_Fix_Prompt.md` - 修正プロンプト

**次の実装予定（Phase 4-B候補）**:
- ComboBox（ドロップダウン選択）
- EditableText（テキスト入力フィールド）
- SpinBox（数値入力）
- ListView（リスト表示）
- ScrollBox（スクロール可能コンテナ）

---

### 2025-12-24: UMG Phase 3 完了 - 21ツール + RenderTransform拡張

**完了機能**:
- UMG Widget Blueprint 操作ツール Phase 1-3 実装完了
- 合計21ツール実装 + RenderTransformアニメーション拡張

**Phase 1: Designer操作 (11ツール)**:
- `create_umg_widget_blueprint` - Widget Blueprint作成
- `add_text_to_widget` - TextBlock追加
- `add_image_to_widget` - Image追加
- `add_progressbar_to_widget` - ProgressBar追加
- `add_vertical_box_to_widget` - VerticalBox追加
- `add_horizontal_box_to_widget` - HorizontalBox追加
- `get_widget_elements` - 要素一覧取得
- `set_widget_slot_property` - Canvas Slot設定
- `set_widget_element_property` - 要素プロパティ設定
- `reparent_widget_element` - 親変更
- `remove_widget_element` - 要素削除

**Phase 2: 変数・関数操作 (5ツール)**:
- `add_widget_variable` - 変数追加（各種型対応）
- `set_widget_variable_default` - デフォルト値設定
- `add_widget_function` - 関数作成
- `add_widget_event` - イベント作成
- `bind_widget_to_variable` - バインディング関数作成

**Phase 3: Animation (4ツール)**:
- `create_widget_animation` - アニメーション作成
- `add_animation_track` - トラック追加（7プロパティ対応）
- `add_animation_keyframe` - キーフレーム追加（Linear/Cubic/Constant）
- `get_widget_animations` - アニメーション一覧取得

**Phase 3: Array Variables (1ツール)**:
- `add_widget_array_variable` - 配列型変数追加（TArray<T>）

**サポートするアニメーションプロパティ**:
| プロパティ | 値の形式 | 用途 |
|-----------|---------|------|
| `Opacity` / `RenderOpacity` | float (0-1) | フェード |
| `ColorAndOpacity` | [R,G,B,A] | 色変化 |
| `RenderTransform.Translation.X` | float (px) | 横移動 |
| `RenderTransform.Translation.Y` | float (px) | 縦移動 |
| `RenderTransform.Scale.X` | float | 横スケール |
| `RenderTransform.Scale.Y` | float | 縦スケール |
| `RenderTransform.Angle` | float (度) | 回転 |

**テスト結果**:
- SlideInX (Translation.X) ✅
- PopIn (Scale.X/Y) ✅
- Spin (Angle) ✅
- 配列変数 (String/Integer/Texture2D/LinearColor) ✅

**ドキュメント**:
- `Docs/UMGPhase3_Handover_Prompt.md` - 引き継ぎドキュメント
- `Docs/UMGPhase3_RenderTransform_Prompt.md` - RenderTransform実装プロンプト（完了）

---

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
```

---

## テスト環境

- **Unreal Engine**: 5.5+ / 5.7
- **プロジェクト**: TrapxTrapCpp
- **RAGサーバー**: AIサーバー :8100
- **Embedding**: BGE-M3
- **ベクトルDB**: ChromaDB
- **トランスポート**: stdio（デフォルト）/ SSE（開発用）
- **起動スクリプト**: start_mcp_server.ps1 / start_mcp_server.bat
- **設定管理**: config.local.ps1 / config.local.bat（環境固有設定）
- **最終確認日**: 2025-12-25

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
| 🆕 | 新規追加 |
