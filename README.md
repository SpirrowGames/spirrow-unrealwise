# SpirrowUnrealWise

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5+-blue)](https://www.unrealengine.com/)
[![Python](https://img.shields.io/badge/Python-3.11+-green)](https://www.python.org/)
[![MCP](https://img.shields.io/badge/MCP-Model%20Context%20Protocol-purple)](https://modelcontextprotocol.io/)
[![Status](https://img.shields.io/badge/Status-Beta-yellow)]()

**SpirrowUnrealWise** は、AI（Claude）と Unreal Engine 5 を連携させる MCP (Model Context Protocol) サーバーです。自然言語でのBlueprint操作、レベルデザイン、UI作成を実現し、AIドリブンなゲーム開発ワークフローを提供します。

## ✨ 主な機能

### 🎮 Actor操作
- レベル内アクターの一覧取得・検索
- アクターのスポーン・削除・Transform操作
- コンポーネントの取得・プロパティ設定

### 📘 Blueprint自動化
- Blueprintの作成・複製・コンパイル
- コンポーネントの追加（StaticMesh, Collision, Physics等）
- 変数・関数・イベントノードの追加
- ノード接続・グラフ操作
- プロパティ設定（ObjectProperty, ClassProperty対応）

### 🎮 Enhanced Input対応
- Input Action / Input Mapping Context の作成
- キーバインディング設定
- Blueprintへのマッピングコンテキスト自動設定

### 🖼️ UMG Widget操作
- Widget Blueprintの作成
- UI要素の追加（Text, Image, Button, ProgressBar, Slider等）
- レイアウト・アンカー設定
- アニメーション作成
- 変数・関数・イベントバインディング

### ⚔️ Gameplay Ability System (GAS)
- Gameplay Tags の追加・管理
- GameplayEffect / GameplayAbility の作成
- AbilitySystemComponent の設定
- GAS対応キャラクターの作成

### 🎨 マテリアル作成
- テンプレートベースのマテリアル作成
- カスタムテンプレートの保存・再利用

### ⚙️ プロジェクト設定
- Config (ini) ファイルの読み書き
- GameMode / DefaultMap 等の設定

### 🧠 RAG知識ベース
- プロジェクト固有の知識を蓄積
- 設計決定の自動記録（rationale）
- プロジェクトコンテキストの管理

---

## 🚀 クイックスタート

### 必要要件

- **Unreal Engine 5.5+**
- **Python 3.11+**
- **uv** (Python パッケージマネージャー)
- **Claude Desktop** または MCP対応クライアント

### 1. リポジトリのクローン

```bash
git clone https://github.com/your-repo/spirrow-unrealwise.git
cd spirrow-unrealwise
```

### 2. Python環境のセットアップ

```bash
# uvのインストール（未インストールの場合）
pip install uv

# 依存関係のインストール
cd Python
uv sync
```

### 3. Unreal Engineプラグインのセットアップ

1. `MCPGameProject/Plugins/SpirrowBridge` フォルダを対象プロジェクトの `Plugins` フォルダにコピー
2. Unreal Editorでプロジェクトを開く
3. **Edit > Plugins** で "SpirrowBridge" を有効化
4. エディタを再起動

### 4. Claude Desktop設定

`claude_desktop_config.json` に以下を追加:

```json
{
  "mcpServers": {
    "spirrow-unrealwise": {
      "command": "uv",
      "args": [
        "--directory",
        "C:/path/to/spirrow-unrealwise/Python",
        "run",
        "python",
        "-m",
        "spirrow_unrealwise"
      ],
      "env": {
        "SPIRROW_UE_HOST": "127.0.0.1",
        "SPIRROW_UE_PORT": "8080"
      }
    }
  }
}
```

### 5. 動作確認

1. Unreal Editorを起動（SpirrowBridgeプラグイン有効）
2. Claude Desktopを起動
3. Claudeに「レベル内のアクター一覧を取得して」と依頼

---

## 📖 使用例

### Blueprintの作成

```
「BP_Enemy という Actor Blueprint を /Game/Blueprints/Characters に作成して」
```

### コンポーネントの追加

```
「BP_Enemy に SphereComponent を DetectionSphere という名前で追加して、半径を500に設定して」
```

### Enhanced Input設定

```
「IA_Attack という Digital タイプの Input Action を作成して、
IMC_Default に左クリックでバインドして」
```

### Widget作成

```
「WBP_HealthBar という Widget Blueprint を作成して、
中央に ProgressBar を配置して」
```

### GAS設定

```
「GE_Damage という Instant タイプの GameplayEffect を作成して、
Health 属性を -25 する Modifier を追加して」
```

---

## 🛠️ ツール一覧

### Actor操作 (8ツール)
| ツール | 説明 |
|--------|------|
| `get_actors_in_level` | レベル内の全アクター取得 |
| `find_actors_by_name` | 名前パターンでアクター検索 |
| `spawn_actor` | アクターをスポーン |
| `delete_actor` | アクターを削除 |
| `set_actor_transform` | Transform設定 |
| `get_actor_properties` | プロパティ取得 |
| `set_actor_property` | プロパティ設定 |
| `get_actor_components` | コンポーネント一覧 |

### Blueprint操作 (25+ツール)
| ツール | 説明 |
|--------|------|
| `create_blueprint` | Blueprint作成 |
| `duplicate_blueprint` | Blueprint複製 |
| `compile_blueprint` | コンパイル |
| `add_component_to_blueprint` | コンポーネント追加 |
| `set_component_property` | コンポーネントプロパティ設定 |
| `set_blueprint_property` | BPプロパティ設定 |
| `add_blueprint_variable` | 変数追加 |
| `add_blueprint_event_node` | イベントノード追加 |
| `add_blueprint_function_node` | 関数ノード追加 |
| `connect_blueprint_nodes` | ノード接続 |
| ... | その他多数 |

### UMG Widget操作 (30+ツール)
| ツール | 説明 |
|--------|------|
| `create_umg_widget_blueprint` | Widget BP作成 |
| `add_text_to_widget` | テキスト追加 |
| `add_image_to_widget` | 画像追加 |
| `add_button_to_widget` | ボタン追加 |
| `add_progressbar_to_widget` | プログレスバー追加 |
| `create_widget_animation` | アニメーション作成 |
| `add_widget_variable` | 変数追加 |
| `add_widget_function` | 関数追加 |
| ... | その他多数 |

### Enhanced Input (5ツール)
| ツール | 説明 |
|--------|------|
| `create_input_action` | Input Action作成 |
| `create_input_mapping_context` | IMC作成 |
| `add_action_to_mapping_context` | マッピング追加 |
| `add_mapping_context_to_blueprint` | BPにIMC設定 |
| `set_default_mapping_context` | デフォルトIMC設定 |

### GAS (8ツール)
| ツール | 説明 |
|--------|------|
| `add_gameplay_tags` | タグ追加 |
| `list_gameplay_tags` | タグ一覧 |
| `create_gameplay_effect` | GameplayEffect作成 |
| `create_gameplay_ability` | GameplayAbility作成 |
| `create_gas_character` | GAS対応キャラクター作成 |
| ... | その他 |

### Config・ユーティリティ
| ツール | 説明 |
|--------|------|
| `get_config_value` | Config値取得 |
| `set_config_value` | Config値設定 |
| `scan_project_classes` | プロジェクトクラス検索 |
| `find_relevant_nodes` | 関連ノード検索 |

### RAG知識ベース
| ツール | 説明 |
|--------|------|
| `search_knowledge` | 知識検索 |
| `add_knowledge` | 知識追加 |
| `list_knowledge` | 知識一覧 |
| `get_project_context` | プロジェクトコンテキスト取得 |
| `update_project_context` | コンテキスト更新 |

---

## 📁 プロジェクト構造

```
spirrow-unrealwise/
├── Python/                          # MCPサーバー (Python)
│   ├── spirrow_unrealwise/
│   │   ├── __main__.py              # エントリーポイント
│   │   ├── server.py                # MCPサーバー本体
│   │   ├── ue_client.py             # UE通信クライアント
│   │   └── tools/                   # ツール定義
│   │       ├── actor_tools.py
│   │       ├── blueprint_tools.py
│   │       ├── widget_tools.py
│   │       ├── gas_tools.py
│   │       └── ...
│   ├── tests/                       # テストスイート
│   └── pyproject.toml
│
├── MCPGameProject/                  # UEプラグイン
│   └── Plugins/
│       └── SpirrowBridge/
│           ├── Source/
│           │   └── SpirrowBridge/
│           │       ├── Private/
│           │       │   ├── SpirrowBridgeModule.cpp
│           │       │   ├── SpirrowBridgeRouter.cpp
│           │       │   ├── Commands/      # 18コマンドファイル
│           │       │   │   ├── SpirrowBridgeActorCommands.cpp
│           │       │   │   ├── SpirrowBridgeBlueprintCommands.cpp
│           │       │   │   ├── SpirrowBridgeUMGWidgetCommands.cpp
│           │       │   │   └── ...
│           │       │   └── Utils/
│           │       └── Public/
│           │           └── SpirrowBridge.h
│           └── SpirrowBridge.uplugin
│
├── Docs/                            # ドキュメント
│   ├── PATTERNS.md                  # 実装パターン集
│   ├── IMPLEMENTATION_SUMMARY.md
│   └── ...
│
├── CLAUDE.md                        # AI向けプロジェクト概要
├── AGENTS.md                        # AIエージェント向けガイド
└── FEATURE_STATUS.md                # 機能実装状況
```

---

## 🔧 開発者向け情報

### アーキテクチャ

```
┌─────────────────┐     MCP      ┌─────────────────┐    TCP/JSON    ┌─────────────────┐
│  Claude/AI      │◄────────────►│  Python Server  │◄──────────────►│  UE Plugin      │
│  (MCP Client)   │              │  (MCP Server)   │                │  (SpirrowBridge)│
└─────────────────┘              └─────────────────┘                └─────────────────┘
```

### 新規コマンドの追加

1. `Python/spirrow_unrealwise/tools/` に新規ツールを定義
2. `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/` にC++実装を追加
3. `SpirrowBridgeRouter.cpp` にルーティングを追加
4. `FEATURE_STATUS.md` を更新

詳細は `Docs/PATTERNS.md` を参照。

### テスト

```bash
cd Python

# 基本動作確認
uv run python -m pytest tests/smoke_test.py -v

# 全テスト
uv run python -m pytest tests/ -v
```

### エラーハンドリング

全コマンドで統一されたエラーコード体系を使用:
- `ESpirrowErrorCode::Success` - 成功
- `ESpirrowErrorCode::AssetNotFound` - アセット未発見
- `ESpirrowErrorCode::InvalidParameter` - パラメータ不正
- `ESpirrowErrorCode::OperationFailed` - 操作失敗
- など12種類

---

## 📋 バージョン履歴

### v0.6.6+ (Beta)
- UMGWidgetCommands 3ファイル分割
- Phase E: 全18 Commands エラーハンドリング統一
- Blueprint系6ファイル分割完了

### v0.6.0
- GAS (Gameplay Ability System) 対応
- GameplayTags / GameplayEffect / GameplayAbility ツール

### v0.5.0
- UMG Widget操作ツール
- アニメーション対応
- 変数・関数バインディング

### v0.4.0
- Enhanced Input対応
- Config操作ツール

### v0.3.0
- Blueprint ノードグラフ操作
- RAG知識ベース統合

---

## 🤝 貢献

Issue・Pull Request 歓迎します。

---

## 📄 ライセンス

MIT License

---

## 🔗 関連リンク

- [Model Context Protocol](https://modelcontextprotocol.io/)
- [Unreal Engine Documentation](https://docs.unrealengine.com/)
- [Claude](https://claude.ai/)
