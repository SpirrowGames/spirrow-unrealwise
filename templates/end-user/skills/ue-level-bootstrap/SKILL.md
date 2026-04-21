---
name: ue-level-bootstrap
description: Bootstrap a new Unreal level via Spirrow-UnrealWise MCP — create the .umap, configure WorldSettings (GameMode Override, KillZ, etc.), and save. Use when the user asks to create a new map / level / world from scratch.
---

# Level bootstrap (Spirrow-UnrealWise)

End-to-end flow for creating a fresh `.umap` and getting it production-ready.

## When to use

User says things like:
- 「新しいマップを作って」/ "create a new map called X"
- 「新規レベル + ゲームモード設定して」
- 「Open World テンプレートで XXX を作って」

If the user only wants to edit an *existing* level, skip this — go straight to `editor(command="open_level", ...)`.

## Inputs to gather (ask if missing)

| Param | Required | Default | Notes |
|---|---|---|---|
| `level_name` | ✅ | — | snake_case or PascalCase. No slashes/spaces. |
| `path` | ❌ | `/Game/Maps` | Content folder. Must start with `/Game/` |
| `template` | ❌ | `default` | `default` (UE 5.7 = WP enabled) / `empty` (non-WP) / explicit `/Game/...` path |
| `game_mode` | ❌ (recommended) | — | Class path like `/Game/Blueprints/BP_GM_Default.BP_GM_Default_C`. **Sets `DefaultGameMode` on WorldSettings (UI shows as "GameMode Override")** |
| Other WorldSettings | ❌ | — | KillZ / WorldToMeters / TimeDilation / GlobalGravityZ / bEnableWorldBoundsChecks / bEnableWorldComposition / DefaultPhysicsVolumeClass / KillZDamageType |

If the user wants a GameMode but doesn't have one yet, surface that as a follow-up: "対応する GameMode BP は既に存在しますか? 無ければ先に `blueprint(command='create_blueprint', params={'parent_class': 'GameModeBase', ...})` で作成しましょう"

## Standard recipe

```python
# 1. Create the .umap (auto-saves and switches editor to it)
editor(command="create_level", params={
    "name": "<level_name>",
    "path": "<path>",         # default /Game/Maps
    "template": "<template>"  # default
})
# → response: { level_path: "/Game/Maps/<level_name>", switched_editor: true, ... }

# 2. Set WorldSettings (skip individual props the user didn't request)
editor(command="set_world_properties", params={"properties": {
    "DefaultGameMode": "<game_mode_class_path>",  # e.g. "/Game/Blueprints/BP_GM_Default.BP_GM_Default_C"
    "KillZ": -5000.0,
    # ... only include what was requested
}})
# → applied: [...], failed: [...]
# → if failed is non-empty, surface the per-property error to the user

# 3. Persist to disk (set_world_properties only marks dirty; doesn't save)
editor(command="save_current_level")
```

## After bootstrap — what to suggest next

Naturally chain into one of these depending on user intent:
- "次に default Pawn を spawn する?" → `editor(command="spawn_blueprint_actor", ...)`
- "Level Blueprint に BeginPlay を仕込む?" → `blueprint_node(command="add_blueprint_event_node", params={"target_type": "level_blueprint", "level_path": "<level_path>", "event_name": "BeginPlay"})`
- "ライトと NavMeshBoundsVolume を置く?" → `editor(command="spawn_actor", ...)` × 複数

## Pitfalls

1. **`overwrite=false` の既存マップ衝突** — `create_level` は同名既存で fail。意図したら `overwrite=true` を明示
2. **`template="default"` は World Partition 有効** — WP 環境を望まないなら `template="empty"`
3. **`set_world_properties` は自動保存しない** — `save_current_level` を必ず最後に呼ぶ
4. **GameMode のフィールド名は `DefaultGameMode`** — UE エディタの "GameMode Override" 表示に騙されない (内部名は `DefaultGameMode`、`DisplayName` メタで表示が変わるだけ)
5. **未保存ワーニング** — `create_level` は現在開いているレベルを閉じる。dirty なら UE の保存ダイアログが出ることがある

## Verification

ユーザに「UE エディタの Window → World Settings パネルで設定値を目視確認してください」と促すか、自動で:
```python
editor(command="get_world_settings")
# → curated preset 9 props を返却。ユーザに reflect 値を見せる
```

## Reference docs (in this repo)

- `docs/SPIRROW_CHEATSHEET.md` — メタツール早見
- `CLAUDE.md` § "Spirrow-UnrealWise の使い方" — メタツールパターン全般
