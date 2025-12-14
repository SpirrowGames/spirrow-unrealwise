<div align="center">

# Spirrow-UnrealWise
<span style="color: #555555">Unreal Engine 5 ナレッジアシスタント MCP サーバー</span>

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5%2B-orange)](https://www.unrealengine.com)
[![Python](https://img.shields.io/badge/Python-3.12%2B-yellow)](https://www.python.org)
[![Status](https://img.shields.io/badge/Status-Experimental-red)](https://github.com/SpirrowGames/spirrow-unrealwise)

</div>

このプロジェクトは、Cursor、Windsurf、Claude DesktopなどのAIアシスタントクライアントが、Model Context Protocol (MCP)を通じて自然言語でUnreal Engineを制御できるようにします。

## ⚠️ 実験的ステータス

このプロジェクトは現在**実験段階**です。API、機能、実装の詳細は大きく変更される可能性があります。テストやフィードバックは歓迎しますが、以下の点にご注意ください：

- 予告なく破壊的変更が発生する可能性があります
- 機能が不完全または不安定な場合があります
- ドキュメントが古いまたは欠けている可能性があります
- 現時点では本番環境での使用は推奨されません

## 🎯 プロジェクトの目標

Spirrow-UnrealWiseは、単なるBlueprint生成ツールではなく、**UE5ナレッジアシスタント**として機能することを目指します。

### 解決する課題
- 「実現したい機能に対して、UE5でどのノード/クラス/APIを使えばいいかわからない」
- 選択肢の存在自体を知らない問題

### 3層アーキテクチャ（予定）
```
Layer 1: ナレッジアシスタント
  - 目的からの逆引き検索
  - ノード/クラスの解説
  - プロジェクトコンテキストを考慮した提案

Layer 2: RAGサーバー
  - プロジェクト固有のナレッジ蓄積
  - 過去の意思決定・失敗の記録
  - 使うほど賢くなる

Layer 3: Blueprint生成（オプション）
  - 確定したノード構成をBlueprintに生成
  - 対話的に構成を決めてから生成
```

## 🌟 概要

Unreal MCP統合は、自然言語を通じてUnreal Engineを制御するための包括的なツールを提供します：

| カテゴリ | 機能 |
|----------|-------------|
| **アクター管理** | • アクターの作成と削除（キューブ、球体、ライト、カメラなど）<br>• アクターのトランスフォーム設定（位置、回転、スケール）<br>• アクタープロパティのクエリと名前による検索<br>• 現在のレベル内の全アクターをリスト表示 |
| **Blueprint開発** | • カスタムコンポーネントを持つ新しいBlueprintクラスの作成<br>• コンポーネントの追加と設定（メッシュ、カメラ、ライトなど）<br>• コンポーネントプロパティと物理設定の設定<br>• Blueprintのコンパイルとアクターのスポーン<br>• カスタムフォルダパスでのBlueprint作成・操作（pathパラメータ対応）<br>• C++親クラスのプロパティへのアクセス対応 |
| **Blueprint ノードグラフ** | • イベントノードの追加（BeginPlay、Tickなど）<br>• 関数呼び出しノードの作成と接続<br>• カスタム型とデフォルト値を持つ変数の追加<br>• コンポーネントと自己参照の作成<br>• グラフ内のノードの検索と管理<br>• 全ノード操作ツールでpathパラメータ対応 |
| **入力システム** | • Enhanced Input Actionアセットの作成（Digital/Axis1D/2D/3D対応）<br>• Input Mapping Contextアセットの作成<br>• アクションマッピングの追加（トリガー、モディファイア対応）<br>• レガシー入力マッピングの作成（Action/Axis） |
| **アセット管理** | • Content Browserからのアセット削除<br>• アセット存在確認とエラーハンドリング |
| **エディタコントロール** | • 特定のアクターまたは位置へのビューポートフォーカス<br>• ビューポートカメラの向きと距離の制御 |
| **プロジェクト構造解析** | • C++クラスとBlueprintアセットのスキャン<br>• 親クラス・モジュール・パスによるフィルタリング<br>• Blueprintタイプフィルタ（Actor/Widget/Anim/Interface等）<br>• REINST_*クラス（Live Coding一時クラス）の自動除外<br>• プロジェクト固有のクラス階層の可視化<br>• Engineクラスの除外オプション |
| **ナレッジアシスタント** | • やりたいことから関連ノード/クラス/アセットを検索<br>• RAGナレッジとプロジェクトクラスを統合検索<br>• 日本語・英語キーワード自動抽出<br>• UEクラス/関数の推奨提案<br>• プロジェクト固有のクラスマッチング |
| **RAGナレッジ管理** | • プロジェクト固有のナレッジをRAGサーバーに保存・検索<br>• カテゴリとタグによる整理<br>• 過去の設計判断や問題解決策を蓄積<br>• 使うほど賢くなるナレッジベース |

これらすべての機能は、AIアシスタントを介した自然言語コマンドでアクセスでき、Unreal Engineワークフローの自動化と制御を簡単にします。

### ナレッジアシスタント機能について

`find_relevant_nodes` ツールは、「やりたいこと」から逆引きで関連するUEノード、クラス、アセットを提案します。RAGナレッジベースとプロジェクトクラスを統合検索し、日本語・英語の両方に対応しています。

**主な機能**:
- **キーワード抽出**: クエリから自動的にキーワードを抽出（日本語→英語変換対応）
- **UEクラス推奨**: 30以上のキーワードマッピングに基づき、適切なエンジンクラス/関数を提案
- **RAG検索**: プロジェクト固有のナレッジベースから関連情報を検索（関連度スコア付き）
- **プロジェクトクラススキャン**: キーワードに基づいてスマートにフィルタリングされたプロジェクトクラスを検索

**使用例**:
```python
# 日本語クエリ - ジャンプ機能の実装方法を調べる
find_relevant_nodes("ジャンプを実装したい")
# 推奨: Character, CharacterMovementComponent, Jump, LaunchCharacter

# 英語クエリ - AI敵の追跡機能を調べる
find_relevant_nodes("make enemy chase player")
# 推奨: AIController, AIMoveTo, MoveToActor, BehaviorTree

# プロジェクトクラスのみ検索 - RAGを使わずに既存クラスを探す
find_relevant_nodes("character movement", include_rag=False)

# RAGナレッジのみ検索 - プロジェクトクラスをスキャンせずに知識を探す
find_relevant_nodes("UI widget design patterns", include_project=False)
```

**対応キーワード例**:
- **移動系**: jump/ジャンプ, move/移動, walk/歩く, run/走る, sprint, fly/飛ぶ, swim/泳ぐ, crouch/しゃがむ
- **戦闘系**: damage/ダメージ, health/体力, attack/攻撃, shoot/撃つ, projectile/弾, hit/当たり
- **AI系**: ai, chase/追いかける, patrol/巡回, follow/ついていく, enemy/敵
- **物理系**: physics/物理, collision/衝突, overlap/重なり, trigger/トリガー
- **その他**: animation/アニメーション, input/入力, ui/widget, camera/カメラ, save/保存, timer/タイマー, network/ネットワーク

### Blueprint親クラス指定について

`create_blueprint`でC++親クラスを指定する際、Aプレフィックスの有無を気にする必要はありません。システムは以下の3段階で親クラスを検索します：

**検索優先順位**:
1. **直接StaticClass参照** - 一般的なエンジンクラス（APawn、AActor、ACharacter）
2. **TObjectIterator検索** - ロード済みの全クラスを検索（最も確実）
3. **LoadClass検索** - 複数のモジュールパスとプレフィックスパターンを試行

**Method 3の検索パターン**（9通り試行）:
- Aプレフィックスあり: `/Script/Engine.AClassName`、`/Script/Game.AClassName`、`/Script/ProjectName.AClassName`
- Aプレフィックスなし: `/Script/Engine.ClassName`、`/Script/Game.ClassName`、`/Script/ProjectName.ClassName` ← **UEリフレクション標準**
- ユーザー入力そのまま: `/Script/Engine.UserInput`、`/Script/Game.UserInput`、`/Script/ProjectName.UserInput`

**重要**: UE5のリフレクションシステムは、C++クラスをAプレフィックス**なし**で登録します（例：`APlayerCharacterBase` → `/Script/TrapxTrapCpp.PlayerCharacterBase`）。そのため、Method 3ではAプレフィックスなしのパスが最も成功しやすいです。

**使用例**:
```python
# どちらの指定でも動作します
create_blueprint(name="BP_Player", parent_class="PlayerCharacterBase")
create_blueprint(name="BP_Player", parent_class="APlayerCharacterBase")
```

## 🧩 コンポーネント

### サンプルプロジェクト (MCPGameProject) `MCPGameProject`
- Blank Projectをベースに、SpirrowBridgeプラグインを追加したもの。

### プラグイン (SpirrowBridge) `MCPGameProject/Plugins/SpirrowBridge`
- MCP通信用のネイティブTCPサーバー
- Unreal Editorサブシステムとの統合
- アクター操作ツールの実装
- コマンド実行とレスポンス処理

### Python MCPサーバー `Python/unreal_mcp_server.py`
- `unreal_mcp_server.py`に実装
- C++プラグインへのTCPソケット接続管理（ポート55557）
- コマンドのシリアライズとレスポンスのパース
- エラーハンドリングと接続管理
- `tools`ディレクトリからツールモジュールのロードと登録
- FastMCPライブラリを使用してModel Context Protocolを実装

### RAGサーバー連携 `Python/tools/rag_tools.py`
- 外部RAGサーバーとの統合によるナレッジ管理
- プロジェクト固有の知識を蓄積・検索
- カテゴリとタグによる整理機能
- 環境変数 `RAG_SERVER_URL` で接続先を設定（デフォルト: `http://localhost:8100`）

**利用可能なツール**:
- `search_knowledge` - ナレッジベースを検索
- `add_knowledge` - 新しいナレッジを追加
- `list_knowledge` - 全ナレッジを一覧表示
- `delete_knowledge` - ナレッジを削除

## 📂 ディレクトリ構造

- **MCPGameProject/** - サンプルUnrealプロジェクト
  - **Plugins/SpirrowBridge/** - C++プラグインソース
    - **Source/SpirrowBridge/** - プラグインソースコード
    - **SpirrowBridge.uplugin** - プラグイン定義

- **Python/** - Pythonサーバーとツール
  - **tools/** - アクター、エディタ、Blueprint操作用ツールモジュール
  - **scripts/** - サンプルスクリプトとデモ

- **Docs/** - 包括的なドキュメント
  - ドキュメントインデックスは [Docs/README.md](Docs/README.md) を参照

## 🚀 クイックスタートガイド

### 前提条件
- Unreal Engine 5.5+
- Python 3.12+
- MCPクライアント（例：Claude Desktop、Cursor、Windsurf）

### サンプルプロジェクト

すぐに始めるには、`MCPGameProject`のスタータープロジェクトを使用してください。これは`SpirrowBridge.uplugin`が既に設定されたUE 5.5 Blank Starter Projectです。

1. **プロジェクトの準備**
   - .uprojectファイルを右クリック
   - Visual Studioプロジェクトファイルを生成
2. **プロジェクトのビルド（プラグインを含む）**
   - ソリューション（`.sln`）を開く
   - ターゲットとして`Development Editor`を選択
   - ビルド

### プラグイン
既存のプロジェクトでプラグインを使用したい場合：

1. **プラグインをプロジェクトにコピー**
   - `MCPGameProject/Plugins/SpirrowBridge`をプロジェクトのPluginsフォルダにコピー

2. **プラグインを有効化**
   - Edit > Plugins
   - Editorカテゴリで「SpirrowBridge」を検索
   - プラグインを有効化
   - プロンプトが表示されたらエディタを再起動

3. **プラグインのビルド**
   - .uprojectファイルを右クリック
   - Visual Studioプロジェクトファイルを生成
   - ソリューション（`.sln`）を開く
   - ターゲットプラットフォームと出力設定でビルド

### Pythonサーバーのセットアップ

Python環境のセットアップ、MCPサーバーの実行、直接またはサーバーベースの接続の使用など、詳細な手順は [Python/README.md](Python/README.md) を参照してください。

#### クイック起動（Windows）

プロジェクトルートに、便利な起動スクリプトを用意しています：

**PowerShell**:
```powershell
.\start_mcp_server.ps1
```

**コマンドプロンプト**:
```cmd
start_mcp_server.bat
```

起動スクリプトは以下を自動で行います：
1. ローカル設定ファイルの読み込み（存在する場合）
2. Pythonディレクトリへの移動
3. uvのインストール確認
4. MCPサーバーの起動

#### ローカル設定ファイル（環境固有の設定）

環境固有の設定（RAGサーバーURLなど）は、ローカル設定ファイルで管理できます。これらのファイルはgitに追跡されないため、各環境で異なる設定を安全に保存できます。

**セットアップ方法**:
1. テンプレートをコピー：
   ```powershell
   # PowerShell用
   cp config.example.ps1 config.local.ps1

   # バッチファイル用
   copy config.example.bat config.local.bat
   ```

2. `config.local.ps1` または `config.local.bat` を編集して設定を変更：
   ```powershell
   # config.local.ps1 の例
   $env:RAG_SERVER_URL = "http://your-rag-server:8100"
   ```

3. 起動スクリプトを実行すると、自動的に設定が読み込まれます

**利用可能な設定**:
- `RAG_SERVER_URL` - RAGサーバーの接続先（デフォルト: `http://localhost:8100`）
- 他の環境変数も追加可能

### 起動モード

Spirrow-UnrealWiseは2つのトランスポートモードをサポートしています：

#### Stdio モード（デフォルト）

MCPクライアント（Claude Desktop等）のサブプロセスとして動作します。
設定が簡単ですが、Python側のコード変更時にはMCPクライアントの再起動が必要です。

**起動**: MCPクライアントの設定で自動起動（後述）

#### SSE モード（開発推奨）

独立したHTTPサーバーとして動作します。
Python側のコード変更時にMCPサーバーだけ再起動すれば反映されるため、開発効率が向上します。

**起動**:
```powershell
# PowerShell
.\start_mcp_server_sse.ps1

# コマンドプロンプト
start_mcp_server_sse.bat
```

**エンドポイント**: `http://localhost:8000/sse`

#### ワークフロー比較

| 操作 | Stdio モード | SSE モード |
|------|-------------|-----------|
| Python変更の反映 | MCPクライアント再起動 | MCPサーバーのみ再起動（Ctrl+C → 再実行） |
| 初期設定 | JSON設定のみ | サーバー手動起動 + JSON設定 |
| 推奨用途 | 本番利用 | 開発・デバッグ |

### MCPクライアントの設定

#### Stdio モード（デフォルト）

MCPクライアントに応じて、以下のJSONをmcp設定に使用してください。

```json
{
  "mcpServers": {
    "spirrow-unrealwise": {
      "command": "uv",
      "args": [
        "--directory",
        "<path/to/the/folder/PYTHON>",
        "run",
        "unreal_mcp_server.py"
      ]
    }
  }
}
```

#### SSE モード（開発推奨）

1. まずMCPサーバーを手動起動：
```powershell
   .\start_mcp_server_sse.ps1
```

2. MCPクライアントの設定：
```json
   {
     "mcpServers": {
       "spirrow-unrealwise": {
         "url": "http://localhost:8000/sse"
       }
     }
   }
```

3. Python変更時の反映手順：
   - サーバーを Ctrl+C で停止
   - `.\start_mcp_server_sse.ps1` で再起動
   - MCPクライアントの再起動は不要！

例は`mcp.json`にあります。

### MCP設定ファイルの場所

使用するMCPクライアントによって、設定ファイルの場所が異なります：

| MCPクライアント | 設定ファイルの場所 | 備考 |
|------------|------------------------------|-------|
| Claude Desktop | `~/.config/claude-desktop/mcp.json` | Windows: `%USERPROFILE%\.config\claude-desktop\mcp.json` |
| Cursor | `.cursor/mcp.json` | プロジェクトルートディレクトリに配置 |
| Windsurf | `~/.config/windsurf/mcp.json` | Windows: `%USERPROFILE%\.config\windsurf\mcp.json` |

各クライアントは上記の例と同じJSON形式を使用します。
MCPクライアントに応じて、適切な場所に設定を配置してください。

### RAGサーバーの設定（オプション）

RAGナレッジ管理機能を使用する場合、環境変数でRAGサーバーのURLを指定できます：

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "<path/to/the/folder/PYTHON>",
        "run",
        "unreal_mcp_server.py"
      ],
      "env": {
        "RAG_SERVER_URL": "http://your-rag-server:8100"
      }
    }
  }
}
```

環境変数を設定しない場合、デフォルトで `http://localhost:8100` に接続を試みます。

## 📝 ライセンス

このプロジェクトはMITライセンスの下で公開されています。

### 帰属表示

このプロジェクトは [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp) （MIT License）をベースに、SpirrowGames向けに拡張したものです。

```
Original work Copyright (c) 2024 Chong-U Lim (chongdashu)
Modified work Copyright (c) 2025 SpirrowGames
```

オリジナルプロジェクトの作者に感謝します。

## 💡 質問・問い合わせ

このプロジェクトに関する質問は、以下までお願いします：
- **オリジナルプロジェクト**: [@chongdashu](https://www.x.com/chongdashu)
- **このフォーク**: SpirrowGames
