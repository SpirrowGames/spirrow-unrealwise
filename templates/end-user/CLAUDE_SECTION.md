<!--
This is a drop-in section for your project's CLAUDE.md.
Copy/append the content below the marker into your CLAUDE.md.

After installing the 4 skills under `.claude/skills/ue-*-bootstrap/` and
the cheatsheet at `Docs/SPIRROW_CHEATSHEET.md`, paste this section so
Claude (Code/Desktop) follows the rules.
-->

## Spirrow-UnrealWise (MCP) の使い方

このプロジェクトは Spirrow-UnrealWise MCP サーバ経由で UE エディタを操作する。25 メタツール × 157 コマンド (v0.9.5 時点) が使えるが、**呼び方を間違えるとハマる**ので以下の規約を守る。

### メタツールパターン

すべて以下の形式:
```python
<category>(command="<command_name>", params={"key": value, ...})
```

例:
```python
editor(command="spawn_actor", params={"name": "MyLight", "type": "PointLight", "location": [0, 0, 200]})
blueprint(command="create_blueprint", params={"name": "BP_Enemy", "parent_class": "Character"})
```

### 規約: help を先に呼ぶ

知らない / 自信のないコマンドは **必ず先に `help` で params 仕様を確認** してから実行する。Claude が憶測で params を埋めて呼ぶのは禁止 (失敗のループ → ユーザの時間浪費)。

```python
help(category="blueprint")                          # カテゴリ内コマンド一覧
help(category="blueprint", command="create_blueprint")  # 特定コマンドの params 詳細
```

例外: 過去に同じセッション内で同じコマンドを成功させていれば re-call で help 省略可。

### 規約: 標準ワークフローは Skill を起動

繰り返し発生するタスクは Skill 化済。該当する場合は手書きで MCP 呼ぶ前にまず Skill を起動:

| やりたいこと | Skill |
|---|---|
| 新規マップ + GameMode 設定 | `/ue-level-bootstrap` |
| 新規 BP 作成 + コンポーネント + コンパイル | `/ue-blueprint-scaffold` |
| UMG HUD (Widget BP + 要素 + viewport 表示) | `/ue-hud-bootstrap` |
| Enhanced Input (IA + IMC + bind + default) | `/ue-input-bootstrap` |

Skill ファイル: `.claude/skills/ue-*-bootstrap/SKILL.md`

### 規約: コンパイル必須

BP 系コマンド (component 追加 / 変数追加 / ノード追加 / プロパティ変更) は **dirty にするだけで自動コンパイルしない**。一連の変更後に必ず最後に:
```python
blueprint(command="compile_blueprint", params={"blueprint_name": "<name>", "path": "<path>"})
```
を呼ぶ。Widget BP も同じ (`compile_blueprint` を共用)。

### 規約: WorldSettings 変更後は save

`editor(command="set_world_properties", ...)` は level を dirty マークするだけで保存しない。続けて `editor(command="save_current_level")` を呼ばないと再起動で消える。

### 規約: Level Script Blueprint (LSB) 編集は target_type で切替

通常 BP 用の `blueprint` / `blueprint_node` コマンド多数は、`target_type="level_blueprint"` パラメータで現在開いているレベルの LSB を対象にできる:
```python
blueprint_node(command="add_blueprint_event_node", params={
    "target_type": "level_blueprint",
    "level_path": "/Game/Maps/MyMap",  # 省略時は現在開いているレベル
    "event_name": "BeginPlay"
})
```
対応コマンド一覧は `Docs/SPIRROW_CHEATSHEET.md` § F.

### エラー時のリトライ戦略

| エラー | 対処 |
|---|---|
| `Unknown command: xxx` | スペル (typo) → `help(category=...)` で正規名確認。それでも無ければ Spirrow-UnrealWise が古い可能性 |
| `<param> is required` | 必須 param 不足 → `help(category, command)` で仕様再確認 |
| `Blueprint not found: <name>` | 名前 / path のミス → `blueprint(command="scan_project_classes", params={"class_type": "blueprint"})` で実在確認 |
| `Compilation failed` | 直前の add_component / add_variable / ノード追加が原因。`get_blueprint_graph` でグラフ状態確認 |
| Class picker 系で `Class not found` | パスフォーマット: C++ は `/Script/Engine.Character` (U/A プレフィックス無し)、BP は `/Game/.../BP_Foo.BP_Foo_C` (末尾 `_C` 必須) |
| 部分成功 (`set_world_properties` 等) | レスポンスの `applied` と `failed: [{property, error}]` を**両方**見る。success=true でも失敗項目あり得る |

**同じエラーで2回連続失敗したら Claude 側でループしない**。ユーザに状況報告し、判断を仰ぐ。

### 早見表

詳細リファレンスは `Docs/SPIRROW_CHEATSHEET.md` を参照。25 メタツール一覧、典型ワークフロー、よく使うコマンド トップ 30、未対応機能リスト等を掲載。
