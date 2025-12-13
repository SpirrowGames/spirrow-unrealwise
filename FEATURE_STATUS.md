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
| `set_actor_property` | ⚠️ 制限あり | コンポーネント内プロパティへのアクセス不可 |

### Blueprint操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_blueprint` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `spawn_blueprint_actor` | ❌ タイムアウト | 通信問題の可能性、要調査。pathパラメータ対応 |
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
| `create_umg_widget_blueprint` | 🔲 未確認 | |
| `add_text_block_to_widget` | 🔲 未確認 | |
| `add_button_to_widget` | 🔲 未確認 | |
| `bind_widget_event` | 🔲 未確認 | |
| `add_widget_to_viewport` | 🔲 未確認 | |
| `set_text_block_binding` | 🔲 未確認 | |

### アセット管理

| ツール | 状態 | 備考 |
|--------|------|------|
| `delete_asset` | ✅ 動作OK | Content Browserからアセット削除 |

### その他

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_input_mapping` | 🔲 未確認 | |

### RAG連携

| ツール | 状態 | 備考 |
|--------|------|------|
| `search_knowledge` | ✅ 動作OK | RAGサーバー連携、意味検索対応 |
| `add_knowledge` | ✅ 動作OK | ナレッジ追加、カテゴリ・タグ対応 |
| `list_knowledge` | ✅ 動作OK | 登録済みナレッジ一覧取得 |
| `delete_knowledge` | ✅ 動作OK | ID指定でナレッジ削除 |

---

## 確認された制限事項

1. **spawn_actor**: アクター作成のみ。StaticMeshの設定は別途Blueprint経由が必要
2. **set_actor_property**: コンポーネント内のプロパティ（StaticMeshComponent.StaticMesh等）にアクセス不可
3. **spawn_blueprint_actor**: タイムアウトエラー。MCPサーバー⇔UE間の通信に問題がある可能性

---

## 追加予定機能

### Phase 1: ナレッジアシスタント（優先度高）

目的: 「やりたいこと → 使うべきノード/クラス」の逆引き支援

| ツール | 説明 | 状態 |
|--------|------|------|
| `find_relevant_nodes` | 目的からBPノード/C++クラスを検索 | 📋 計画中 |
| `explain_node` | ノード/クラスの詳細解説 | 📋 計画中 |
| `scan_project_classes` | プロジェクト内のクラス/BP一覧取得 | 📋 計画中 |

### Phase 2: RAG統合（完了）

目的: プロジェクト固有のナレッジ蓄積と検索

| ツール | 説明 | 状態 |
|--------|------|------|
| `search_knowledge` | 蓄積されたナレッジの検索 | ✅ 完了 |
| `add_knowledge` | ナレッジの追加（カテゴリ・タグ対応） | ✅ 完了 |
| `list_knowledge` | 全ナレッジの一覧表示 | ✅ 完了 |
| `delete_knowledge` | ナレッジの削除 | ✅ 完了 |

### Phase 3: 既存機能の改善（優先度中）

| 項目 | 説明 | 状態 |
|------|------|------|
| `spawn_blueprint_actor` のタイムアウト修正 | 通信処理の改善 | 📋 計画中 |
| `set_actor_property` の拡張 | コンポーネントプロパティへのアクセス | 📋 計画中 |

### Phase 4: 追加ツール（優先度低）

| ツール | 説明 | 状態 |
|--------|------|------|
| `get_blueprint_graph` | 既存BPのノード構成取得 | 💡 アイデア |
| `duplicate_blueprint` | BP複製 | 💡 アイデア |
| `rename_actor` | アクター名変更 | 💡 アイデア |

---

## 最新の更新履歴

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
- **起動スクリプト**: start_mcp_server.ps1 / start_mcp_server.bat
- **設定管理**: config.local.ps1 / config.local.bat（環境固有設定）
- **最終確認日**: 2025-12-09

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
