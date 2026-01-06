# SpirrowUnrealWise

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5+-blue)](https://www.unrealengine.com/)
[![Python](https://img.shields.io/badge/Python-3.11+-green)](https://www.python.org/)
[![MCP](https://img.shields.io/badge/MCP-Model%20Context%20Protocol-purple)](https://modelcontextprotocol.io/)
[![Status](https://img.shields.io/badge/Status-Beta-yellow)]()

AI（Claude）と Unreal Engine 5 を連携させる MCP サーバー。自然言語でBlueprint操作、レベルデザイン、UI作成を実現します。

## ✨ 機能 (108ツール)

| カテゴリ | 説明 |
|---------|------|
| 🎮 **Actor** (10) | スポーン、Transform、プロパティ、コンポーネント |
| 📘 **Blueprint** (16) | 作成、コンポーネント追加、ノードグラフ操作 |
| 🖼️ **UMG Widget** (29) | UI要素、レイアウト、アニメーション、バインディング |
| 🎮 **Enhanced Input** (5) | Input Action、Mapping Context |
| ⚔️ **GAS** (8) | GameplayTags、Effect、Ability |
| 🤖 **AI** (28) | BehaviorTree、Blackboard、AIPerception、EQS |
| 🎨 **Material** (5) | テンプレートベース作成 |
| ⚙️ **Config** (3) | ini設定読み書き |
| 🧠 **RAG** (4) | 知識ベース、プロジェクトコンテキスト |

> 詳細: [FEATURE_STATUS.md](FEATURE_STATUS.md)

---

## 🚀 クイックスタート

### 必要要件
- Unreal Engine 5.5+
- Python 3.11+ / uv
- Claude Desktop

### セットアップ

```bash
# 1. クローン
git clone https://github.com/your-repo/spirrow-unrealwise.git
cd spirrow-unrealwise

# 2. Python依存関係
cd Python && uv sync

# 3. UEプラグイン
# MCPGameProject/Plugins/SpirrowBridge を対象プロジェクトにコピー
```

### Claude Desktop設定

`claude_desktop_config.json`:
```json
{
  "mcpServers": {
    "spirrow-unrealwise": {
      "command": "uv",
      "args": ["--directory", "C:/path/to/Python", "run", "python", "unreal_mcp_server.py"],
      "env": { "SPIRROW_UE_HOST": "127.0.0.1", "SPIRROW_UE_PORT": "8080" }
    }
  }
}
```

### 動作確認
1. Unreal Editor起動（SpirrowBridge有効）
2. Claude Desktopで「レベル内のアクター一覧を取得して」

---

## 📖 使用例

```
「BP_Enemy という Actor Blueprint を作成して」

「BP_Enemy に SphereComponent を追加して、半径500に設定」

「WBP_HUD に ProgressBar を中央に配置」

「BT_Enemy という BehaviorTree を作成して、Selector ノードを追加」
```

---

## 📁 構造

```
spirrow-unrealwise/
├── Python/                    # MCPサーバー
│   ├── unreal_mcp_server.py   # メイン
│   ├── tools/                 # ツール定義 (12ファイル)
│   └── tests/                 # テスト
├── MCPGameProject/Plugins/    # UEプラグイン
│   └── SpirrowBridge/
└── Docs/                      # ドキュメント
```

---

## 🔧 開発

```bash
# テスト実行
cd Python && python tests/run_tests.py

# 新規コマンド追加
# → Docs/PATTERNS.md 参照
```

---

## 📋 バージョン

**v0.8.0 (Beta)** - 2026-01-06
- Phase H: AIPerception & EQS対応 (11ツール追加)
- AI システム完全対応 (合計28ツール)
- UE 5.6+ API互換

> 履歴: [Docs/CHANGELOG.md](Docs/CHANGELOG.md)

---

## 📄 ライセンス

MIT License

## 🔗 リンク

[MCP](https://modelcontextprotocol.io/) | [Unreal Engine](https://docs.unrealengine.com/) | [Claude](https://claude.ai/)
