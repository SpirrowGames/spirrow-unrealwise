# SpirrowUnrealWise UMG Phase 3 継続プロンプト

## プロジェクト情報

- **MCP プロジェクト**: `C:\Users\takahito ito\Documents\Unreal Projects\spirrow-unrealwise`
- **ゲームプロジェクト**: `C:\Users\takahito ito\Documents\Unreal Projects\TrapxTrapCpp 5.7`
- **プラグイン**: `Plugins/SpirrowBridge`
- **テスト用Widget**: `WBP_TT_TrapSelector` (`/Game/TrapxTrap/UI`)

---

## 現在の状況

### 完了済み

**Phase 1 (Designer操作)**: 11ツール ✅
**Phase 2 (変数・関数)**: 5ツール ✅
**Phase 3 (Animation)**: 3ツール ✅
- `create_widget_animation` - アニメーション作成
- `add_animation_track` - Opacity/ColorAndOpacity トラック追加
- `add_animation_keyframe` - キーフレーム追加（Linear/Cubic/Constant）

**合計: 19ツール実装完了**

### Phase 3 残り機能（未実装）

| 優先度 | ツール | 説明 |
|--------|--------|------|
| 高 | `get_widget_animations` | アニメーション一覧取得 |
| 高 | `add_widget_array_variable` | 配列型変数追加 |
| 中 | `set_widget_array_default` | 配列デフォルト値設定 |
| 中 | RenderTransform トラック | Translation/Scale/Angle 対応 |
| 低 | `create_property_binding` | 完全なプロパティバインディング |
| 低 | `add_widget_graph_node` | ノード追加 |
| 低 | `connect_widget_nodes` | ノード接続 |

---

## 設計ドキュメント

- **Phase 3 設計**: `Docs/UMGPhase3_Animation_Prompt.md`
- **Animation Track 詳細**: `Docs/UMGPhase3_AnimationTrack_Prompt.md`
- **引き継ぎ**: `Docs/UMGPhase3_Handover_Prompt.md`

---

## 開始手順

1. プロジェクトコンテキスト確認:
```
get_project_context("TrapxTrapCpp")
```

2. 設計ドキュメント確認:
```
Docs/UMGPhase3_Handover_Prompt.md を参照
```

3. 残り機能から実装を選択

---

## 次に実装推奨

### オプション A: 配列型変数サポート

```python
add_widget_array_variable(
    widget_name: str,
    variable_name: str,
    element_type: str,  # String, Integer, etc.
    is_exposed: bool = False,
    category: str = None,
    path: str = "/Game/UI"
)
```

### オプション B: RenderTransform トラック対応

`add_animation_track` を拡張して以下をサポート:
- `RenderTransform.Translation` - [X, Y]
- `RenderTransform.Scale` - [X, Y]  
- `RenderTransform.Angle` - float (degrees)

### オプション C: get_widget_animations

アニメーション一覧を取得するユーティリティツール。

---

## 技術メモ

- Build.cs に `"MovieScene"`, `"MovieSceneTracks"` 追加済み
- UE 5.7 では `TArray64<uint8>` が必要な箇所あり
- 非推奨 API は `PRAGMA_DISABLE_DEPRECATION_WARNINGS` で抑制済み

---

**最終更新**: 2025-12-24
