# SpirrowUnrealWise

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5+-blue)](https://www.unrealengine.com/)
[![Python](https://img.shields.io/badge/Python-3.11+-green)](https://www.python.org/)
[![MCP](https://img.shields.io/badge/MCP-Model%20Context%20Protocol-purple)](https://modelcontextprotocol.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

**[English version / 英語版](README.md)**

AI（Claude）と Unreal Engine 5 を連携させる MCP サーバー。自然言語でBlueprint操作、レベルデザイン、UI作成、AIシステム構築を実現します。

## 機能 (25 MCPツール / 153コマンド)

**v0.9.1** でメタツール化を実施。コンテキスト消費量を ~170K → ~22K tokens に大幅削減。

| メタツール | コマンド数 | 説明 |
|-----------|-----------|------|
| `editor` | 13 | スポーン、Transform、プロパティ、コンポーネント、レベル(.umap)作成 🆕 |
| `blueprint` | 21 | 作成、コンパイル、プロパティ、DataAsset (LSB対応) 🆕 |
| `blueprint_node` | 24 | イベントノード、関数、変数、フロー制御 (LSB + 外部UPROPERTY + typed Subsystem) 🆕 |
| `umg_widget` | 18 | テキスト、画像、ボタン、スライダー等 |
| `umg_layout` | 5 | VBox/HBox、ScrollBox、リペアレント |
| `umg_variable` | 5 | Widget変数、関数、イベント |
| `umg_animation` | 4 | アニメーション、トラック、キーフレーム |
| `project` | 13 | Input Mapping、アセット、フォルダ、テクスチャ |
| `ai` | 22 | BehaviorTree、Blackboard、BT修復 |
| `perception` | 6 | 視覚、聴覚、ダメージ感知 |
| `eqs` | 5 | Environment Query System |
| `gas` | 8 | GameplayTags、Effect、Ability |
| `material` | 6 | テンプレートベースマテリアル作成 |
| `config` | 3 | INIファイル読み書き |

**+ 10 スタンドアロンツール**: RAG (4)、AI画像 (3)、プロジェクトコンテキスト (2)、関連ノード (1)

> 詳細は [FEATURE_STATUS.md](FEATURE_STATUS.md) を参照してください。

---

## クイックスタート

### 必要要件

- Unreal Engine 5.5+
- Python 3.11+ と [uv](https://github.com/astral-sh/uv)
- Claude Desktop または Claude Code

### インストール

```bash
# 1. リポジトリをクローン
git clone https://github.com/SpirrowGames/spirrow-unrealwise.git
cd spirrow-unrealwise

# 2. Python依存関係をインストール
cd Python && uv sync

# 3. UEプラグインをプロジェクトにコピー
# コピー元: MCPGameProject/Plugins/SpirrowBridge → 対象プロジェクト/Plugins/
```

### Claude Desktop設定

`claude_desktop_config.json` に追加:

```json
{
  "mcpServers": {
    "spirrow-unrealwise": {
      "command": "uv",
      "args": ["--directory", "C:/path/to/spirrow-unrealwise/Python", "run", "python", "unreal_mcp_server.py"],
      "env": {
        "SPIRROW_UE_HOST": "127.0.0.1",
        "SPIRROW_UE_PORT": "8080"
      }
    }
  }
}
```

### 動作確認

1. SpirrowBridgeプラグインを有効にしてUnreal Editorを起動
2. Claude Desktopで「レベル内のアクター一覧を取得して」と入力

---

## 使用例

```
「BP_Enemy という Actor Blueprint を作成して」

「BP_Enemy に SphereComponent を追加して、半径500に設定」

「WBP_HUD という Widget Blueprint を作成して、中央に ProgressBar を配置」

「BT_Enemy という BehaviorTree を作成して、Selector ノードを追加」

「AIPerception を設定して、視覚センスを追加、範囲2000、視野角90度」
```

---

## プロジェクト構成

```
spirrow-unrealwise/
├── Python/                    # MCPサーバー
│   ├── unreal_mcp_server.py   # メインエントリ
│   ├── tools/                 # ツール定義 (14メタ + スタンドアロン)
│   └── tests/                 # テストスイート
├── MCPGameProject/Plugins/    # UEプラグイン
│   └── SpirrowBridge/         # エディターモジュール
├── Docs/                      # ドキュメント
└── templates/                 # マテリアルテンプレート
```

---

## 開発

### テスト実行

```bash
cd Python && python tests/run_tests.py
```

### 新規コマンド追加

実装パターンとガイドラインは [Docs/PATTERNS.md](Docs/PATTERNS.md) を参照してください。

### 環境変数

| 変数 | デフォルト | 説明 |
|------|-----------|------|
| `SPIRROW_UE_HOST` | `127.0.0.1` | Unreal Editorホスト |
| `SPIRROW_UE_PORT` | `8080` | Unreal Editorポート |
| `AI_IMAGE_SERVER_URL` | `http://localhost:7860` | Stable Diffusionサーバー |

---

## バージョン履歴

**v0.9.2 (Beta)** - 2026-03-13
- BTノード堅牢性向上: 全BTノード作成時にnullポインタガード追加
- `repair_broken_bt_nodes` コマンド追加（NodeInstance再生成）
- コンパイル安全性改善

**v0.9.1 (Beta)** - 2026-03-07
- メタツールアーキテクチャ: 161個の個別ツール → 25 MCPツール（14メタ + 1ヘルプ + 10スタンドアロン）
- コンテキストトークン削減: ~170K → ~22K（~148K節約）

**v0.9.0 (Beta)** - 2026-02-15
- BehaviorTree健全性チェック: `detect_broken_bt_nodes` と `fix_broken_bt_nodes` を追加
- 壊れたノード（null NodeInstance）を自動検出・削除

**v0.8.11 (Beta)** - 2026-01-26
- `find_cpp_function_in_blueprints` 追加 - Blueprint内の関数呼び出し元を検索

**v0.8.10 (Beta)** - 2026-01-12
- AI画像生成統合（Stable Diffusion Forge）
- アセットユーティリティツール（テクスチャインポート、フォルダ管理）

> 詳細な履歴は [Docs/CHANGELOG.md](Docs/CHANGELOG.md) を参照してください。

---

## コントリビューション

Issue や Pull Request を歓迎します！

---

## ライセンス

MIT License - 詳細は [LICENSE](LICENSE) を参照してください。

---

## リンク

- [Model Context Protocol](https://modelcontextprotocol.io/)
- [Unreal Engine Documentation](https://docs.unrealengine.com/)
- [Claude](https://claude.ai/)
