# SpirrowBridge 実装サマリ

このドキュメントは、SpirrowBridge プラグインの C++ 実装概要をまとめたものです。
新しいチャットセッション開始時に、コードベースの全体像を把握するために参照してください。

> **最終更新**: 2026-01-03  
> **バージョン**: 0.6.5

---

## ファイル構成

### Commands ディレクトリ

#### Blueprint 系（3分割 + ルーター）

| ファイル | サイズ | 役割 |
|----------|--------|------|
| `SpirrowBridgeBlueprintCoreCommands.cpp` | 23 KB | Blueprint 作成/コンパイル/スポーン/複製/グラフ取得 |
| `SpirrowBridgeBlueprintComponentCommands.cpp` | 26 KB | コンポーネント追加/プロパティ設定/物理 |
| `SpirrowBridgeBlueprintPropertyCommands.cpp` | 21 KB | クラススキャン/配列プロパティ |
| `SpirrowBridgeBlueprintCommands.cpp` | 1.7 KB | ルーター（上記3ファイルへ委譲） |

#### BlueprintNode 系（3分割 + ルーター）

| ファイル | サイズ | 役割 |
|----------|--------|------|
| `SpirrowBridgeBlueprintNodeCoreCommands.cpp` | 24 KB | 接続/検索/イベント/関数呼び出し |
| `SpirrowBridgeBlueprintNodeVariableCommands.cpp` | 14 KB | 変数/Get/Set/Self参照/InputAction |
| `SpirrowBridgeBlueprintNodeControlFlowCommands.cpp` | 21 KB | Branch/Sequence/Delay/Loop/Math/Print |
| `SpirrowBridgeBlueprintNodeCommands.cpp` | 1.7 KB | ルーター（上記3ファイルへ委譲） |

#### UMG Widget 系（4分割）

| ファイル | サイズ | 役割 |
|----------|--------|------|
| `SpirrowBridgeUMGWidgetCommands.cpp` | 64 KB | Widget 追加 |
| `SpirrowBridgeUMGVariableCommands.cpp` | 40 KB | 変数/バインディング |
| `SpirrowBridgeUMGLayoutCommands.cpp` | 32 KB | レイアウト |
| `SpirrowBridgeUMGAnimationCommands.cpp` | 23 KB | アニメーション |

#### その他

| ファイル | サイズ | 役割 |
|----------|--------|------|
| `SpirrowBridgeGASCommands.cpp` | 55 KB | GAS |
| `SpirrowBridgeCommonUtils.cpp` | 35 KB | 共通ユーティリティ |
| `SpirrowBridgeEditorCommands.cpp` | 29 KB | アクター・エディタ操作 |
| `SpirrowBridgeProjectCommands.cpp` | 25 KB | プロジェクト・入力設定 |
| `SpirrowBridgeMaterialCommands.cpp` | 8 KB | マテリアル作成 |
| `SpirrowBridgeConfigCommands.cpp` | 8 KB | Config（INI）操作 |

**合計**: 18 ファイル（Blueprint系6分割、UMG系4分割完了）

---

## クラス別関数一覧

### FSpirrowBridgeBlueprintCoreCommands (23 KB)

Blueprint の作成・基本操作を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleCreateBlueprint` | `create_blueprint` | Blueprint 作成 |
| `HandleCompileBlueprint` | `compile_blueprint` | コンパイル |
| `HandleSpawnBlueprintActor` | `spawn_blueprint_actor` | Blueprint アクター生成 |
| `HandleSetBlueprintProperty` | `set_blueprint_property` | Blueprint プロパティ設定 |
| `HandleDuplicateBlueprint` | `duplicate_blueprint` | Blueprint 複製 |
| `HandleGetBlueprintGraph` | `get_blueprint_graph` | ノードグラフ取得 |

---

### FSpirrowBridgeBlueprintComponentCommands (26 KB)

コンポーネント追加・プロパティ設定を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleAddComponentToBlueprint` | `add_component_to_blueprint` | コンポーネント追加 |
| `HandleSetComponentProperty` | `set_component_property` | コンポーネントプロパティ設定 |
| `HandleSetPhysicsProperties` | `set_physics_properties` | 物理設定 |
| `HandleSetStaticMeshProperties` | `set_static_mesh_properties` | StaticMesh 設定 |
| `HandleSetPawnProperties` | - | Pawn プロパティ設定（内部用） |

#### ヘルパー
| 関数 | 説明 |
|------|------|
| `AddComponentToBlueprint` | コンポーネント追加の実装 |

---

### FSpirrowBridgeBlueprintPropertyCommands (21 KB)

クラススキャン・配列プロパティを担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleScanProjectClasses` | `scan_project_classes` | プロジェクトクラススキャン |
| `HandleSetBlueprintClassArray` | `set_blueprint_class_array` | クラス配列設定 |
| `HandleSetStructArrayProperty` | `set_struct_array_property` | 構造体配列設定 |

---

### FSpirrowBridgeBlueprintNodeCoreCommands (24 KB)

Blueprint ノードの接続・検索・基本操作を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleConnectBlueprintNodes` | `connect_blueprint_nodes` | ノード接続 |
| `HandleFindBlueprintNodes` | `find_blueprint_nodes` | ノード検索 |
| `HandleSetNodePinValue` | `set_node_pin_value` | ピン値設定 |
| `HandleDeleteNode` | `delete_blueprint_node` | ノード削除 |
| `HandleMoveNode` | `move_blueprint_node` | ノード移動 |
| `HandleAddBlueprintEvent` | `add_blueprint_event_node` | イベントノード追加 |
| `HandleAddBlueprintFunctionCall` | `add_blueprint_function_node` | 関数呼び出しノード追加 |

---

### FSpirrowBridgeBlueprintNodeVariableCommands (14 KB)

変数・Self参照・入力アクションを担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleAddBlueprintVariable` | `add_blueprint_variable` | 変数追加 |
| `HandleAddVariableGetNode` | `add_variable_get_node` | 変数 Get ノード追加 |
| `HandleAddVariableSetNode` | `add_variable_set_node` | 変数 Set ノード追加 |
| `HandleAddBlueprintGetSelfComponentReference` | `add_blueprint_get_self_component_reference` | コンポーネント参照追加 |
| `HandleAddBlueprintSelfReference` | `add_blueprint_self_reference` | Self 参照追加 |
| `HandleAddBlueprintInputActionNode` | `add_blueprint_input_action_node` | 入力アクションノード追加 |

---

### FSpirrowBridgeBlueprintNodeControlFlowCommands (21 KB)

制御フロー・ユーティリティノードを担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleAddBranchNode` | `add_branch_node` | Branch ノード追加 |
| `HandleAddSequenceNode` | `add_sequence_node` | Sequence ノード追加 |
| `HandleAddDelayNode` | `add_delay_node` | Delay ノード追加 |
| `HandleAddForEachLoopNode` | `add_foreach_loop_node` | **非推奨** |
| `HandleAddForLoopWithBreakNode` | `add_forloop_with_break_node` | ForLoopWithBreak 追加 |
| `HandleAddPrintStringNode` | `add_print_string_node` | PrintString ノード追加 |
| `HandleAddMathNode` | `add_math_node` | 演算ノード追加 |
| `HandleAddComparisonNode` | `add_comparison_node` | 比較ノード追加 |

---

### FSpirrowBridgeUMGWidgetCommands (64 KB)

Widget の追加を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleCreateUMGWidgetBlueprint` | `create_umg_widget_blueprint` | Widget Blueprint 作成 |
| `HandleAddTextToWidget` | `add_text_to_widget` | TextBlock 追加 |
| `HandleAddTextBlockToWidget` | `add_text_block_to_widget` | Legacy API |
| `HandleAddImageToWidget` | `add_image_to_widget` | Image 追加 |
| `HandleAddProgressBarToWidget` | `add_progressbar_to_widget` | ProgressBar 追加 |
| `HandleAddButtonToWidget` | `add_button_to_widget` | Button 追加 |
| `HandleAddButtonToWidgetV2` | - | 内部用 |
| `HandleAddSliderToWidget` | `add_slider_to_widget` | Slider 追加 |
| `HandleAddCheckBoxToWidget` | `add_checkbox_to_widget` | CheckBox 追加 |
| `HandleAddComboBoxToWidget` | `add_combobox_to_widget` | ComboBox 追加 |
| `HandleAddEditableTextToWidget` | `add_editabletext_to_widget` | EditableText 追加 |
| `HandleAddSpinBoxToWidget` | `add_spinbox_to_widget` | SpinBox 追加 |
| `HandleAddScrollBoxToWidget` | `add_scrollbox_to_widget` | ScrollBox 追加 |
| `HandleAddWidgetToViewport` | `add_widget_to_viewport` | Legacy API |

---

### FSpirrowBridgeUMGLayoutCommands (32 KB)

レイアウト操作を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleAddVerticalBoxToWidget` | `add_vertical_box_to_widget` | VerticalBox 追加 |
| `HandleAddHorizontalBoxToWidget` | `add_horizontal_box_to_widget` | HorizontalBox 追加 |
| `HandleGetWidgetElements` | `get_widget_elements` | 要素一覧取得 |
| `HandleSetWidgetSlotProperty` | `set_widget_slot_property` | Canvas Slot 設定 |
| `HandleSetWidgetElementProperty` | `set_widget_element_property` | 要素プロパティ設定 |
| `HandleReparentWidgetElement` | `reparent_widget_element` | 親変更 |
| `HandleRemoveWidgetElement` | `remove_widget_element` | 要素削除 |

---

### FSpirrowBridgeUMGAnimationCommands (23 KB)

Widget アニメーションを担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleCreateWidgetAnimation` | `create_widget_animation` | アニメーション作成 |
| `HandleAddAnimationTrack` | `add_animation_track` | トラック追加 |
| `HandleAddAnimationKeyframe` | `add_animation_keyframe` | キーフレーム追加 |
| `HandleGetWidgetAnimations` | `get_widget_animations` | アニメーション一覧 |

---

### FSpirrowBridgeUMGVariableCommands (40 KB)

Widget 変数・関数・バインディングを担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleAddWidgetVariable` | `add_widget_variable` | 変数追加 |
| `HandleAddWidgetArrayVariable` | `add_widget_array_variable` | 配列変数追加 |
| `HandleSetWidgetVariableDefault` | `set_widget_variable_default` | デフォルト値設定 |
| `HandleAddWidgetFunction` | `add_widget_function` | 関数作成 |
| `HandleAddWidgetEvent` | `add_widget_event` | イベント作成 |
| `HandleBindWidgetToVariable` | `bind_widget_to_variable` | バインディング関数作成 |
| `HandleBindWidgetEvent` | `bind_widget_event` | イベントバインディング |
| `HandleSetTextBlockBinding` | `set_text_block_binding` | テキストバインディング |
| `HandleBindWidgetComponentEvent` | `bind_widget_component_event` | コンポーネントイベント |

#### ヘルパー
| 関数 | 説明 |
|------|------|
| `SetupPinType` | ピン型の設定ヘルパー |

---

### FSpirrowBridgeGASCommands (55 KB)

Gameplay Ability System 関連の操作を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleAddGameplayTags` | `add_gameplay_tags` | Gameplay Tag 追加 |
| `HandleListGameplayTags` | `list_gameplay_tags` | Gameplay Tag 一覧 |
| `HandleRemoveGameplayTag` | `remove_gameplay_tag` | Gameplay Tag 削除 |
| `HandleListGASAssets` | `list_gas_assets` | GAS アセット一覧 |
| `HandleCreateGameplayEffect` | `create_gameplay_effect` | GameplayEffect 作成 |
| `HandleCreateGASCharacter` | `create_gas_character` | GAS Character 作成 |
| `HandleSetAbilitySystemDefaults` | `set_ability_system_defaults` | ASC デフォルト設定 |
| `HandleCreateGameplayAbility` | `create_gameplay_ability` | GameplayAbility 作成 |

#### ヘルパー
| 関数 | 説明 |
|------|------|
| `SetGameplayTagContainerFromArray` | JSON 配列から TagContainer 設定 |
| `GetGameplayTagsConfigPath` | Config パス取得 |
| `ParseExistingTags` | 既存タグのパース |
| `WriteTagsToConfig` | タグを Config に書き込み |

---

### FSpirrowBridgeEditorCommands (29 KB)

アクター・エディタ操作を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleGetActorsInLevel` | `get_actors_in_level` | レベル内アクター取得 |
| `HandleFindActorsByName` | `find_actors_by_name` | 名前でアクター検索 |
| `HandleSpawnActor` | `spawn_actor` | アクター生成 |
| `HandleDeleteActor` | `delete_actor` | アクター削除 |
| `HandleSetActorTransform` | `set_actor_transform` | Transform 設定 |
| `HandleGetActorProperties` | `get_actor_properties` | プロパティ取得 |
| `HandleSetActorProperty` | `set_actor_property` | プロパティ設定 |
| `HandleGetActorComponents` | `get_actor_components` | コンポーネント一覧 |
| `HandleRenameActor` | `rename_actor` | アクターリネーム |
| `HandleSpawnBlueprintActor` | - | Blueprint アクター生成（内部用） |
| `HandleFocusViewport` | - | ビューポートフォーカス |
| `HandleTakeScreenshot` | - | スクリーンショット |
| `HandleRenameAsset` | `rename_asset` | アセットリネーム |

---

### FSpirrowBridgeProjectCommands (25 KB)

プロジェクト設定・入力システムを担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleCreateInputMapping` | `create_input_mapping` | 入力マッピング作成 |
| `HandleCreateInputAction` | `create_input_action` | Enhanced Input Action 作成 |
| `HandleCreateInputMappingContext` | `create_input_mapping_context` | IMC 作成 |
| `HandleAddActionToMappingContext` | `add_action_to_mapping_context` | IMC にアクション追加 |
| `HandleDeleteAsset` | `delete_asset` | アセット削除 |
| `HandleAddMappingContextToBlueprint` | - | Blueprint に IMC 追加（内部用） |
| `HandleSetDefaultMappingContext` | - | デフォルト IMC 設定（内部用） |

---

### FSpirrowBridgeConfigCommands (8 KB)

Config（INI）ファイル操作を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleGetConfigValue` | `get_config_value` | Config 値取得 |
| `HandleSetConfigValue` | `set_config_value` | Config 値設定 |
| `HandleListConfigSections` | `list_config_sections` | セクション一覧 |

---

### FSpirrowBridgeMaterialCommands (8 KB)

マテリアル作成を担当。

| 関数 | MCPコマンド | 説明 |
|------|-------------|------|
| `HandleCreateSimpleMaterial` | `create_simple_material` | シンプルマテリアル作成 |

> **Note**: テンプレートベースのマテリアル作成は Python 側（`material_tools.py`）で処理。

---

### FSpirrowBridgeCommonUtils (35 KB)

共通ユーティリティ関数。

#### JSON ユーティリティ
| 関数 | 説明 |
|------|------|
| `CreateErrorResponse` | エラーレスポンス作成 |
| `CreateSuccessResponse` | 成功レスポンス作成 |
| `GetIntArrayFromJson` | JSON から int 配列取得 |
| `GetFloatArrayFromJson` | JSON から float 配列取得 |
| `GetVector2DFromJson` | JSON から Vector2D 取得 |
| `GetVectorFromJson` | JSON から Vector 取得 |
| `GetRotatorFromJson` | JSON から Rotator 取得 |

#### アクターユーティリティ
| 関数 | 説明 |
|------|------|
| `ActorToJson` | アクターを JSON に変換 |
| `ActorToJsonObject` | アクターを JSON オブジェクトに変換 |

#### Blueprint ユーティリティ
| 関数 | 説明 |
|------|------|
| `FindBlueprint` | Blueprint 検索 |
| `FindBlueprintByName` | 名前で Blueprint 検索 |
| `FindOrCreateEventGraph` | EventGraph 取得/作成 |

#### ノードユーティリティ
| 関数 | 説明 |
|------|------|
| `CreateEventNode` | イベントノード作成 |
| `CreateFunctionCallNode` | 関数呼び出しノード作成 |
| `CreateVariableGetNode` | 変数 Get ノード作成 |
| `CreateVariableSetNode` | 変数 Set ノード作成 |
| `CreateInputActionNode` | 入力アクションノード作成 |
| `CreateSelfReferenceNode` | Self 参照ノード作成 |
| `ConnectGraphNodes` | ノード接続 |
| `FindPin` | ピン検索 |
| `FindExistingEventNode` | 既存イベントノード検索 |

#### プロパティユーティリティ
| 関数 | 説明 |
|------|------|
| `SetObjectProperty` | オブジェクトプロパティ設定 |

---

## コマンドルーティング

### メインルーティング（SpirrowBridge.cpp）

```cpp
// ExecuteCommand() 内でカテゴリ別に振り分け
// Blueprint系 → BlueprintCommands（内部で3ファイルに委譲）
if (CommandType == "create_blueprint" || ...) {
    BlueprintCommands->HandleCommand(...)
}
// BlueprintNode系 → BlueprintNodeCommands（内部で3ファイルに委譲）
else if (CommandType == "add_blueprint_event_node" || ...) {
    BlueprintNodeCommands->HandleCommand(...)
}
// UMG Commands (4分割、SpirrowBridge.cppから直接ルーティング)
else if (CommandType == "create_umg_widget_blueprint" || ...) {
    UMGWidgetCommands->HandleCommand(...)
}
else if (CommandType == "add_vertical_box_to_widget" || ...) {
    UMGLayoutCommands->HandleCommand(...)
}
else if (CommandType == "create_widget_animation" || ...) {
    UMGAnimationCommands->HandleCommand(...)
}
else if (CommandType == "add_widget_variable" || ...) {
    UMGVariableCommands->HandleCommand(...)
}
else if (CommandType == "get_actors_in_level" || ...) {
    EditorCommands->HandleCommand(...)
}
else if (CommandType == "create_input_mapping" || ...) {
    ProjectCommands->HandleCommand(...)
}
else if (CommandType == "get_config_value" || ...) {
    ConfigCommands->HandleCommand(...)
}
else if (CommandType == "add_gameplay_tags" || ...) {
    GASCommands->HandleCommand(...)
}
else if (CommandType == "create_simple_material") {
    MaterialCommands->HandleCommand(...)
}
```

### Blueprint 系の内部ルーティング

```cpp
// FSpirrowBridgeBlueprintCommands::HandleCommand()
// → CoreCommands / ComponentCommands / PropertyCommands へ委譲

// FSpirrowBridgeBlueprintNodeCommands::HandleCommand()
// → CoreCommands / VariableCommands / ControlFlowCommands へ委譲
```

---

## 注意事項

### 新コマンド追加時のチェックリスト

1. ✅ `Commands/SpirrowBridge*Commands.h` - 関数宣言
2. ✅ `Commands/SpirrowBridge*Commands.cpp` - 関数実装
3. ✅ `Commands/SpirrowBridge*Commands.cpp` - HandleCommand 内ルーティング
4. ✅ **`SpirrowBridge.cpp`** - ExecuteCommand 内ルーティング（**忘れがち！**）
5. ✅ `Python/tools/*_tools.py` - MCP ツール定義

### 新コマンド追加時のハンドラ選択ガイド

新しいコマンドを追加する際は、以下の表を参考に適切なハンドラを選択する。

| コマンドの種類 | 追加先ハンドラ | 例 |
|---------------|-----------------|-----|
| **Blueprint** | | |
| Blueprint 作成・スポーン・複製・グラフ | `BlueprintCoreCommands` | create_blueprint, duplicate_blueprint |
| コンポーネント追加・プロパティ・物理 | `BlueprintComponentCommands` | add_component_to_blueprint, set_physics_properties |
| クラススキャン・配列プロパティ | `BlueprintPropertyCommands` | scan_project_classes, set_blueprint_class_array |
| **BlueprintNode** | | |
| ノード接続・検索・イベント・関数 | `BlueprintNodeCoreCommands` | connect_blueprint_nodes, add_blueprint_event_node |
| 変数・Get/Set・Self参照・入力アクション | `BlueprintNodeVariableCommands` | add_blueprint_variable, add_variable_get_node |
| Branch・Sequence・Delay・Loop・Math | `BlueprintNodeControlFlowCommands` | add_branch_node, add_math_node |
| **UMG Widget** | | |
| Widget 要素の追加（Text, Image, Button 等） | `UMGWidgetCommands` | add_text_to_widget, add_button_to_widget |
| レイアウト操作（Box 追加・親変更・要素取得） | `UMGLayoutCommands` | add_vertical_box_to_widget, get_widget_elements |
| アニメーション操作 | `UMGAnimationCommands` | create_widget_animation, add_animation_track |
| 変数・関数・バインディング | `UMGVariableCommands` | add_widget_variable, bind_widget_to_variable |
| **その他** | | |
| アクター・エディタ操作 | `EditorCommands` | spawn_actor, get_actor_properties |
| プロジェクト設定・入力システム | `ProjectCommands` | create_input_action, delete_asset |
| Config（INI）操作 | `ConfigCommands` | get_config_value, set_config_value |
| GAS（Gameplay Ability System） | `GASCommands` | add_gameplay_tags, create_gameplay_effect |
| マテリアル作成 | `MaterialCommands` | create_simple_material |

#### 判断のヒント

- **既存コマンドと似た操作** → 同じハンドラに追加
- **新しいカテゴリ** → 新規ハンドラ作成を検討
- **ファイルサイズが 60KB 超** → 分割を検討

### 大きいファイルの分割状況

| ファイル | 状態 | 備考 |
|----------|------|------|
| `SpirrowBridgeUMGCommands.cpp` | ✅ 完了 | 2026-01-02 に4分割 |
| `SpirrowBridgeBlueprintCommands.cpp` | ✅ 完了 | 2026-01-03 に3分割 |
| `SpirrowBridgeBlueprintNodeCommands.cpp` | ✅ 完了 | 2026-01-03 に3分割 |
| `SpirrowBridgeUMGWidgetCommands.cpp` | 📋 候補 | 64KB、将来的に分割検討 |
| `SpirrowBridgeGASCommands.cpp` | 📋 候補 | 55KB、将来的に分割検討 |

---

## 更新履歴

| 日付 | 内容 |
|------|------|
| 2026-01-03 | BlueprintCommands を3分割（Core/Component/Property） |
| 2026-01-03 | BlueprintNodeCommands を3分割（Core/Variable/ControlFlow） |
| 2026-01-02 | 新コマンド追加時のハンドラ選択ガイドを追加 |
| 2026-01-02 | UMGCommands を4分割（Widget/Layout/Animation/Variable） |
| 2026-01-02 | 初版作成 |
