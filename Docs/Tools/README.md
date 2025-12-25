# Unreal MCP ツール

このドキュメントは、SpirrowUnrealWiseでサポートされているすべてのツールのインデックスです。

## ツールカテゴリ

### コアツール
- [Actor Tools](actor_tools.md) - アクター操作（スポーン、削除、トランスフォーム、プロパティ）
- [Editor Tools](editor_tools.md) - エディタユーティリティ（アセット管理、レベル操作）
- [Blueprint Tools](blueprint_tools.md) - Blueprint作成と管理

### Blueprintグラフツール
- [Node Tools](node_tools.md) - Blueprintノード操作
  - イベントノード (BeginPlay, Tick, Input Actions)
  - 関数呼び出しノード
  - 変数ノード (Get/Set)
  - Branchノード (if/else)
  - ノード接続
  - ピン値設定
  - ノード削除/移動
  - 制御フローノード (Sequence, Delay, ForLoopWithBreak)
  - 数学・比較ノード (Math, Comparison)

### Input & プロジェクトツール
- [Project Tools](project_tools.md) - Input System (Enhanced Input & Legacy)

## クイックリファレンス

### ノードツール (21ツール)

| ツール | 説明 | ステータス |
|------|------|----------|
| `add_blueprint_event_node` | イベントノード追加 (BeginPlay, Tick, など) | ✅ 実装済み |
| `add_blueprint_input_action_node` | インプットアクションイベントノード追加 | ✅ 実装済み |
| `add_blueprint_function_node` | 関数呼び出しノード追加 | ✅ 実装済み |
| `connect_blueprint_nodes` | 2つのノードを接続 | ✅ 実装済み |
| `add_blueprint_variable` | Blueprintに変数追加 | ✅ 実装済み |
| `add_blueprint_get_self_component_reference` | コンポーネント参照ノード取得 | ✅ 実装済み |
| `add_blueprint_self_reference` | Self参照ノード取得 | ✅ 実装済み |
| `find_blueprint_nodes` | グラフ内でノード検索 | ✅ 実装済み |
| `set_node_pin_value` | ピンにデフォルト値設定 | ✅ 実装済み |
| `add_variable_get_node` | 変数Getノード追加 | ✅ 実装済み |
| `add_variable_set_node` | 変数Setノード追加 | ✅ 実装済み |
| `add_branch_node` | Branchノード (if/else) 追加 | ✅ 実装済み |
| `delete_blueprint_node` | グラフからノード削除 | ✅ 実装済み |
| `move_blueprint_node` | ノード位置移動 | ✅ 実装済み |
| `add_sequence_node` | 複数実行ブランチ用のSequenceノード追加 | ✅ 実装済み |
| `add_delay_node` | 時間遅延実行用のDelayノード追加 | ✅ 実装済み |
| `add_foreach_loop_node` | ForEachループノード追加 | 🚫 非推奨 |
| `add_forloop_with_break_node` | ForLoopWithBreakノード追加 | ✅ 実装済み (UE 5.7) |
| `add_print_string_node` | デバッグ出力用のPrintStringノード追加 | ✅ 実装済み |
| `add_math_node` | 数学演算ノード追加 (Add, Subtract, など) | ✅ 実装済み (UE 5.7) |
| `add_comparison_node` | 比較ノード追加 (Greater, Less, など) | ✅ 実装済み (UE 5.7) |

## バージョン履歴

- **v0.7.0** (2025-01-26) - Math/Comparisonノード実装完了（UE 5.7対応、DoubleDouble関数使用）、ForLoopWithBreakノード追加、ForEachLoopノード非推奨化
- **v0.6.0** - 制御フロー＆ユーティリティノードツール追加（add_sequence_node, add_delay_node, add_print_string_node）
- **v0.5.0** - ノード操作ツール追加（set_node_pin_value, add_variable_get_node, add_variable_set_node, add_branch_node, delete_blueprint_node, move_blueprint_node）
- **v0.4.0** - UMGウィジェットツール追加（Phase 1-4）
- **v0.3.0** - GAS（Gameplay Ability System）ツール追加
- **v0.2.0** - Blueprintノードツール追加
- **v0.1.0** - アクター＆エディタツールでの初回リリース

## 最近の更新 (v0.7.0)

### 新規追加
- **ForLoopWithBreakノード**: 指定回数のイテレーション用マクロノード（UK2Node_MacroInstanceを使用）
- **Math/Comparisonノード**: KismetMathLibrary関数を使用した完全実装
  - UE 5.7のdouble型対応（FloatFloat→DoubleDouble）
  - Add, Subtract, Multiply, Divide演算
  - Greater, Less, Equal, NotEqual, GreaterEqual, LessEqual比較

### 非推奨化
- **add_foreach_loop_node**: Blueprintマクロのため実装不可。`add_forloop_with_break_node`を使用してください。

### 技術的な改善
- ノードGUID生成の修正（CreateNewGuid + PostPlacedNewNode）
- UE 5.7互換性対応（FindFunctionByNameによる実行時関数検索）
- コマンドルーティングの完全性確保

## ツール使用ガイドライン

### ノードツールを使用する際の推奨事項

1. **イベントノードから開始**: 常に`add_blueprint_event_node`でイベント（ReceiveBeginPlay、ReceiveTickなど）を追加してから、ロジックを構築します。

2. **ノード接続を確認**: `connect_blueprint_nodes`でノードを接続する際は、正しいピン名（`then`, `execute`, `True`, `False`など）を使用してください。

3. **変数の事前作成**: `add_variable_get_node`や`add_variable_set_node`を使用する前に、`add_blueprint_variable`で変数を作成してください。

4. **コンパイルを忘れずに**: ノード追加後は必ず`compile_blueprint`でBlueprintをコンパイルしてください。

5. **ForEachループの代替**: 配列イテレーションには`add_forloop_with_break_node`を使用し、配列の長さを取得してLastIndexピンに接続してください。

6. **Math/Comparisonノード**: UE 5.7では、Float演算にDoubleDouble関数が使用されます。整数演算にはIntInt関数を使用してください。

## トラブルシューティング

### よくある問題

**Q: ノードIDが00000000000000000000000000000000になる**
A: ノード作成後、`CreateNewGuid()`と`PostPlacedNewNode()`が呼ばれていることを確認してください。これはv0.7.0で修正済みです。

**Q: Math/Comparisonノードでコンパイルエラー**
A: UE 5.7では`FloatFloat`関数が`DoubleDouble`に変更されています。最新版では自動的に正しい関数が選択されます。

**Q: ForEachLoopが動作しない**
A: ForEachLoopはマクロノードのため実装できません。`add_forloop_with_break_node`を使用してください。

**Q: コマンドが認識されない**
A: SpirrowBridge.cppのExecuteCommand関数でコマンドが登録されているか確認してください。v0.7.0ですべてのコマンドが登録されています。

## 今後の開発予定

- タイムラインノード
- カスタムイベントノード
- マクロノードのさらなるサポート
- ノードのグループ化・整理機能
- Blueprint関数の作成サポート
