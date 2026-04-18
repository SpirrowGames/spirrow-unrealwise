---
name: ue-blueprint-scaffold
description: Scaffold a new Blueprint via Spirrow-UnrealWise MCP — create the BP, add components, wire BeginPlay, and compile. Use when the user wants a new Character/Actor/Pawn/GameMode/etc. BP from scratch.
---

# Blueprint scaffold (Spirrow-UnrealWise)

Standard pattern for spinning up a new Blueprint asset with the most common boilerplate.

## When to use

- 「新しい敵キャラの BP 作って」
- 「Pickup Actor の BP テンプレート」
- 「GameMode BP 作って Pawn を割り当てて」

If the user only wants to *edit* an existing BP, skip — go directly to `blueprint(command="add_component_to_blueprint", ...)` etc.

## Inputs to gather

| Param | Required | Default | Notes |
|---|---|---|---|
| `name` | ✅ | — | BP class name. Conventional prefix: `BP_` (e.g. `BP_Enemy`) |
| `parent_class` | ✅ | — | UE class name without prefix: `Actor`, `Pawn`, `Character`, `GameModeBase`, `PlayerController`, `HUD`, `UserWidget` (for UMG, use `umg_widget` instead), or BP class path |
| `path` | ❌ | `/Game/Blueprints` | Content folder |
| Components | ❌ | — | List of (type, name) pairs. Common: `StaticMeshComponent`, `SkeletalMeshComponent`, `BoxComponent`, `SphereComponent`, `CapsuleComponent`, `CameraComponent`, `SpringArmComponent`, `WidgetComponent` |
| BeginPlay handler | ❌ | recommended for Actor BPs | Empty event node (extend later) or with PrintString debug |

If `parent_class` is unclear, **ask** rather than guess. Common confusions:
- 「敵」→ `Character` (need walk + collision) or `Pawn` (custom movement)
- 「アイテム」→ `Actor` + StaticMesh + BoxComponent (overlap)
- 「UI」→ DON'T use this skill, use `ue-hud-bootstrap` instead

## Standard recipe

```python
# 1. Create the BP
blueprint(command="create_blueprint", params={
    "name": "<name>",
    "parent_class": "<parent_class>",
    "path": "<path>",      # default /Game/Blueprints
    "rationale": "<why this BP exists, e.g. '敵 AI の基底クラス'>"
})
# → response: { blueprint_path, ... }

# 2. Add components (each call attaches to the BP's SCS)
blueprint(command="add_component_to_blueprint", params={
    "blueprint_name": "<name>",
    "component_type": "StaticMeshComponent",
    "component_name": "Mesh",
    "location": [0, 0, 0],
    "rotation": [0, 0, 0],
    "scale": [1, 1, 1],
    "path": "<path>"
})
# Repeat for each component

# 3. Wire BeginPlay (optional but recommended for Actor/Character BPs)
blueprint_node(command="add_blueprint_event_node", params={
    "blueprint_name": "<name>",
    "event_name": "BeginPlay",
    "path": "<path>"
})
# → returns node_id; you can chain function calls / variable sets from this

# 4. Compile (final step — without this, changes don't take effect)
blueprint(command="compile_blueprint", params={
    "blueprint_name": "<name>",
    "path": "<path>"
})
```

## Common variants

### Pickup Actor (StaticMesh + overlap volume)
```python
blueprint(command="create_blueprint", params={"name": "BP_HealthPickup", "parent_class": "Actor", ...})
blueprint(command="add_component_to_blueprint", params={"blueprint_name": "BP_HealthPickup", "component_type": "StaticMeshComponent", "component_name": "Mesh", ...})
blueprint(command="add_component_to_blueprint", params={"blueprint_name": "BP_HealthPickup", "component_type": "SphereComponent", "component_name": "OverlapTrigger", ...})
blueprint_node(command="add_blueprint_event_node", params={"blueprint_name": "BP_HealthPickup", "event_name": "ActorBeginOverlap", ...})
blueprint(command="compile_blueprint", params={"blueprint_name": "BP_HealthPickup", ...})
```

### Character with custom mesh
```python
blueprint(command="create_blueprint", params={"name": "BP_Enemy", "parent_class": "Character", ...})
# Character already has CapsuleComponent + Mesh + CharacterMovement, so usually just configure the Mesh's static mesh asset:
blueprint(command="set_static_mesh_properties", params={"blueprint_name": "BP_Enemy", "component_name": "Mesh", "static_mesh": "/Engine/EngineMeshes/Sphere.Sphere", ...})
blueprint(command="compile_blueprint", params={"blueprint_name": "BP_Enemy", ...})
```

### GameMode BP (assign default classes)
```python
blueprint(command="create_blueprint", params={"name": "BP_GM_Default", "parent_class": "GameModeBase", ...})
blueprint(command="set_class_property", params={
    "blueprint_name": "BP_GM_Default",
    "property_name": "DefaultPawnClass",
    "class_path": "/Game/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter_C",
    "path": "/Game/Blueprints"
})
blueprint(command="compile_blueprint", params={"blueprint_name": "BP_GM_Default", ...})
```

## Pitfalls

1. **コンパイル忘れ** — `add_component_to_blueprint` 等は dirty にするだけで自動コンパイルしない。最後に必ず `compile_blueprint`
2. **add_component の component_type は U プレフィックス無し** — `StaticMeshComponent` (○) / `UStaticMeshComponent` (×)
3. **Character は既存コンポーネント** — 親 Class 由来 (CapsuleComponent / Mesh / CharacterMovement / Arrow) は再追加せず、`set_static_mesh_properties` 等で設定
4. **path のデフォルト** — `/Game/Blueprints` 固定。プロジェクト規約と違うなら明示
5. **parent_class が見つからない** — bare name で見つからなければ完全パス (`/Script/Engine.Actor`) で再試行可
6. **同名BP が既存** — `create_blueprint` は overwrite せず error。命名衝突を確認
7. **rationale パラメータ** — オプションだが、後で「なぜこの BP を作ったか」追跡用に推奨 (RAG 蓄積に活用)

## After scaffold — what to suggest next

- 「ロジック組む?」→ `blueprint_node` の各種 (`add_blueprint_function_node`, `add_branch_node`, `add_variable_set_node` 等)
- 「物理 / ヒット判定?」→ `blueprint(command="set_physics_properties", ...)`
- 「Spawn してテスト?」→ `editor(command="spawn_blueprint_actor", ...)`
- 「DataAsset 紐付け?」→ `blueprint(command="create_data_asset", ...)` + `set_data_asset_property`

## Reference docs

- `docs/SPIRROW_CHEATSHEET.md` § blueprint / blueprint_node
- `CLAUDE.md` § "Spirrow-UnrealWise の使い方"
