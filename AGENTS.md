# AGENTS.md - SpirrowUnrealWise

> AIエージェント向け実装ガイド（ツール非依存）

---

## 概要

SpirrowUnrealWiseは、Unreal Engine 5とMCP（Model Context Protocol）を接続し、LLMからUEエディタを操作するためのツール群です。

- **言語**: Python（MCP Server）+ C++（Unreal Plugin）
- **UE バージョン**: 5.5+ / 5.7
- **バージョン**: v0.9.3 (25 MCPツール / 152コマンド)

---

## ⚠️ 必須運用ルール: Python 変更後は MCP サーバーを再起動する

**`Python/` 配下のファイル (特に `Python/tools/*_meta.py`, `command_schemas.py`, `unreal_mcp_server.py`) を編集したら、その変更と必ずセットで実行中の MCP サーバープロセスを kill すること。**

ユーザーに「再起動してください」とお願いするのではなく、**エージェント自身が kill まで実行する**。kill 後はクライアント (Claude Desktop / Claude Code) が次のツール呼び出しで自動的に新プロセスを spawn し、最新の Python コードが読み込まれる。

### なぜ必須か

MCP サーバーは起動時に `register_*_meta_tool()` を呼んで `COMMANDS` dict を構築する。Python の import はファイル変更をホットリロードしないため、プロセスを再起動しない限り古い `COMMANDS` が保持され続ける。新コマンドを追加してもユーザー側で **`Unknown command: xxx` エラー** になる。

過去には 2 日間古い Python プロセスが走り続けたために `add_get_subsystem_node` 等の新コマンドがクライアントから見えなかった事例がある。

### 手順 (Windows)

```bash
# 1. 実行中の MCP サーバープロセスを列挙
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe', 'uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Select-Object ProcessId, ParentProcessId, Name, CreationDate | Format-Table -AutoSize"

# 2. 親 uv.exe の PID を kill (子 python も cascade で死ぬ)
cmd.exe //C "taskkill /F /PID <uv_pid>"

# 3. もう一度列挙して 0 件を確認
```

### いつやるか

以下のいずれかを変更した直後に必ず:

- `Python/tools/*_meta.py` の `COMMANDS` dict
- `Python/tools/command_schemas.py` のスキーマ定義
- `Python/tools/meta_utils.py`, `help_tool.py` などの共通基盤
- `Python/unreal_mcp_server.py` の register 呼び出し

**commit 前/後のどちらでもよいが、「ユーザーに動作検証を依頼する前に kill しておく」** のが鉄則。

### セルフチェック

Python を変更したら commit + push の前に:

- [ ] MCP サーバープロセスを kill したか
- [ ] 再列挙して 0 件になったか
- [ ] ユーザーに「次にツールを叩けば自動再接続される」と伝えたか

### 対象外 (このルールは適用されない)

- **C++ 側**: `MCPGameProject/Plugins/SpirrowBridge/` 配下の変更は UE エディタの Live Coding (`Ctrl+Alt+F11`) または `build_editor.bat` で反映する別手順。MCP サーバー再起動とは独立したビルド工程。
- **ドキュメントのみの変更**: README / FEATURE_STATUS / AGENTS 等の `.md` 編集は再起動不要。

---

## プロジェクト構造

```
spirrow-unrealwise/
├── Python/
│   ├── unreal_mcp_server.py    # MCPサーバー本体
│   ├── tools/                  # ツール定義 (12ファイル)
│   └── tests/                  # テストスイート
├── MCPGameProject/Plugins/SpirrowBridge/
│   └── Source/SpirrowBridge/
│       ├── Public/Commands/    # ヘッダ
│       └── Private/Commands/   # 実装 (27ファイル)
├── Docs/                       # ドキュメント
├── FEATURE_STATUS.md           # 機能一覧
└── AGENTS.md                   # このファイル
```

---

## 命名規則

| 種類 | プレフィックス | 例 |
|------|---------------|-----|
| Actor Blueprint | `BP_` | `BP_Enemy`, `BP_Projectile` |
| Widget Blueprint | `WBP_` | `WBP_MainMenu`, `WBP_HUD` |
| GameplayEffect | `GE_` | `GE_Damage`, `GE_HealOverTime` |
| GameplayAbility | `GA_` | `GA_Attack`, `GA_Dash` |
| BehaviorTree | `BT_` | `BT_Enemy`, `BT_Patrol` |
| Blackboard | `BB_` | `BB_Enemy` |

---

## ノード配置ルール

### 基本設定

- 水平間隔: 300px
- 垂直間隔: 150px
- 起点: [0, 0]

### パターン

**直列**:
```
[Event] → [Node A] → [Node B]
[0, 0]    [300, 0]   [600, 0]
```

**分岐**:
```
              → [True]  [600, 0]
[Event] → [Branch]
              → [False] [600, 150]
```

---

## 新コマンド追加チェックリスト

| # | ファイル | 更新内容 |
|---|----------|----------|
| 1 | `Commands/SpirrowBridge*Commands.h` | 関数宣言 |
| 2 | `Commands/SpirrowBridge*Commands.cpp` | 関数実装 |
| 3 | `Commands/SpirrowBridge*Commands.cpp` | HandleCommand内ルーティング |
| 4 | **`SpirrowBridge.cpp`** | **ExecuteCommand内ルーティング** ⚠️ |
| 5 | `Python/tools/*_tools.py` | Pythonツール定義 |

> ⚠️ #4を忘れると「Unknown command」エラー

---

## コーディング規約

### C++

```cpp
// 関数名: Handle{CommandName}
void HandleCreateBlueprint(TSharedPtr<FJsonObject> Params, FString& OutResponse);

// レスポンス作成
return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Result);
return FSpirrowBridgeCommonUtils::CreateErrorResponse(
    ESpirrowErrorCode::BlueprintNotFound, TEXT("Not found"));
```

### Python

```python
@mcp.tool()
async def create_blueprint(name: str, parent_class: str, path: str = "/Game/Blueprints"):
    result = await send_command("create_blueprint", {...})
    return result
```

---

## エラーハンドリング

### エラーコード範囲

| 範囲 | カテゴリ |
|------|----------|
| 1000-1099 | General |
| 1100-1199 | Asset |
| 1200-1299 | Blueprint |
| 1300-1399 | Widget |
| 1400-1499 | Actor |
| 1500-1599 | GAS |
| 1600-1699 | Config |

> 詳細: [Docs/ERROR_CODES.md](Docs/ERROR_CODES.md)

### レスポンス形式

```json
{
    "success": false,
    "error_code": 1200,
    "error": "Blueprint not found: BP_Test",
    "details": { "blueprint_name": "BP_Test", "path": "/Game/Test" }
}
```

---

## テスト実行

```bash
cd Python

# スモークテスト
python tests/smoke_test.py

# 全テスト
python tests/run_tests.py

# カテゴリ別
python tests/run_tests.py -m umg
python tests/run_tests.py -m blueprint
python tests/run_tests.py -m ai
```

---

## トラブルシューティング

| 問題 | 解決策 |
|------|--------|
| "Unknown command" | `SpirrowBridge.cpp`のルーティング確認 |
| タイムアウト | UEエディタがフリーズしていないか確認 |
| ビルド反映されない | `Binaries/`と`Intermediate/`を削除 |
| UE 5.7コンパイルエラー | `FloatFloat`→`DoubleDouble`等の型変更確認 |

---

## rationaleパラメータ

設計根拠を自動記録。対象ツール:

- `create_blueprint`, `add_component_to_blueprint`
- `spawn_actor`, `set_physics_properties`
- `add_blueprint_event_node`, `add_blueprint_function_node`
- `create_gameplay_effect`, `create_gameplay_ability`

```python
create_blueprint(
    name="BP_Enemy",
    parent_class="Character",
    rationale="敵キャラ用。AIControllerで制御するためCharacterベース"
)
```
