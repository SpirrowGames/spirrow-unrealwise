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
| `get_blueprint_graph` | 既存BPのノード構成取得 | 💡 アイデア |
| `duplicate_blueprint` | BP複製 | 💡 アイデア |
| `rename_actor` | アクター名変更 | 💡 アイデア |

---

## 最新の更新履歴

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
- **最終確認日**: 2025-12-14

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
