# AGENTS.md - spirrow-unrealwise MCP

このドキュメントは、AIエージェントがspirrow-unrealwise MCPツールを使用する際のガイドラインです。

---

## 概要

spirrow-unrealwiseは、Unreal Engine 5とMCP（Model Context Protocol）を接続し、LLMからUEエディタを操作するためのツール群です。

**主な用途**:
- Blueprint作成・編集
- アクター操作
- ノードグラフ構築
- UMG Widget作成

---

## ノード配置ルール

### 基本設定

```
水平間隔: 300px
垂直間隔: 150px
起点: [0, 0]
```

### レイアウトパターン

#### 1. 直列（Linear）

```
[Event] → [Node A] → [Node B] → [Node C]
[0, 0]    [300, 0]   [600, 0]   [900, 0]
```

```python
# 計算式
x = index * 300
y = 0
```

#### 2. 分岐（Branch）

```
              → [Node B] [300, 0]
[Event] → [Branch]
              → [Node C] [300, 150]
[0, 0]    [300, 0]
```

分岐後のノードは下方向（+Y）に展開:
```python
# 分岐先の計算
x = parent_x + 300
y = branch_index * 150
```

#### 3. 合流（Merge）

```
[Node A] →
              → [Node C]
[Node B] →
```

合流ノードは最も下の入力ノードのY座標 + 75（中央揃え）に配置。

#### 4. 複雑なグラフ

複数の分岐・合流がある場合:
1. 左から右へ処理順に配置
2. 分岐は下方向に展開
3. 合流点は分岐の中央Y座標に配置
4. 重なりそうな場合は垂直間隔を広げる（150 → 200）

---

## Blueprint作成のベストプラクティス

### 命名規則

| 種類 | プレフィックス | 例 |
|------|---------------|-----|
| Actor Blueprint | `BP_` | `BP_Enemy`, `BP_Projectile` |
| Widget Blueprint | `WBP_` | `WBP_MainMenu`, `WBP_HUD` |
| Component | なし（説明的な名前） | `CubeMesh`, `RootCollision` |

### 作成フロー

1. **Blueprint作成**: `create_blueprint`
2. **コンポーネント追加**: `add_component_to_blueprint`
3. **メッシュ/プロパティ設定**: `set_static_mesh_properties`, `set_component_property`
4. **イベントノード追加**: `add_blueprint_event_node`
5. **関数ノード追加**: `add_blueprint_function_node`
6. **ノード接続**: `connect_blueprint_nodes`
7. **コンパイル**: `compile_blueprint`

### 関数ノードのtarget指定

| 関数の種類 | target | 例 |
|-----------|--------|-----|
| アクター自身のメソッド | `self` | `SetActorLocation` |
| Kismetライブラリ | `KismetSystemLibrary` | `PrintString`, `Delay` |
| Math系 | `KismetMathLibrary` | `Sin`, `Lerp` |
| コンポーネントのメソッド | コンポーネント名 | `MeshComponent` |

---

## ピン名リファレンス

### 実行ピン

| ノード種類 | 出力ピン | 入力ピン |
|-----------|---------|---------|
| Event | `then` | - |
| Function | `then` | `execute` |
| Branch | `True`, `False` | `execute` |

### 一般的なデータピン

| 型 | ピン名例 |
|----|---------|
| Boolean | `Condition`, `ReturnValue` |
| Float | `Value`, `DeltaTime` |
| Vector | `Location`, `Direction` |
| Actor | `Target`, `OtherActor` |

---

## エラーハンドリング

### よくあるエラーと対処

| エラー | 原因 | 対処 |
|--------|------|------|
| `Function not found in target self` | グローバル関数をselfで呼んだ | 適切なライブラリ名を指定 |
| `Timeout receiving Unreal response` | UE側の処理遅延/通信エラー | 再試行、またはUEエディタ確認 |
| `Property not found` | プロパティ名が間違っている | UEでプロパティ名を確認 |

### タイムアウト時の対応

1. UEエディタがフリーズしていないか確認
2. 操作自体は成功している可能性があるので、エディタで結果確認
3. 必要に応じて再試行

---

## 制限事項

### 現在対応していない操作

- 既存アクターへのStaticMesh直接設定（Blueprint経由で対応）
- `spawn_blueprint_actor` は不安定（タイムアウトの可能性）
- ノードのパラメータ設定（一部のみ対応）

### 推奨ワークフロー

複雑なBlueprintは:
1. MCPで骨格（ノード構成）を作成
2. UEエディタで詳細パラメータを調整

---

## 使用例

### 例1: BeginPlayでPrintString

```
1. create_blueprint("BP_Example", "Actor")
2. add_blueprint_event_node("BP_Example", "ReceiveBeginPlay", [0, 0])
3. add_blueprint_function_node("BP_Example", "PrintString", "KismetSystemLibrary", [300, 0])
4. connect_blueprint_nodes(source=BeginPlay, target=PrintString, "then" → "execute")
5. compile_blueprint("BP_Example")
```

### 例2: 物理キューブActor

```
1. create_blueprint("BP_PhysicsCube", "Actor")
2. add_component_to_blueprint("BP_PhysicsCube", "CubeMesh", "StaticMeshComponent")
3. set_static_mesh_properties("BP_PhysicsCube", "CubeMesh", "/Engine/BasicShapes/Cube.Cube")
4. set_physics_properties("BP_PhysicsCube", "CubeMesh", simulate_physics=true)
5. compile_blueprint("BP_PhysicsCube")
```

---

## 更新履歴

- 2024-12-03: 初版作成
