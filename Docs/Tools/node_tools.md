# Unreal MCP ノードツール

このドキュメントは、Unreal MCPで利用可能なBlueprintノードツールの詳細情報を提供します。

## 概要

ノードツールを使用すると、Blueprintグラフのノードと接続をプログラマティックに操作できます。イベントノード、関数ノード、変数の追加、ノード間の接続作成などが可能です。

## 基本ノードツール

### add_blueprint_event_node

Blueprintのイベントグラフにイベントノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `event_name` (string) - イベント名。標準イベントには 'Receive' プレフィックスを使用 (例: 'ReceiveBeginPlay', 'ReceiveTick')
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - Blueprintが配置されているコンテンツブラウザパス (デフォルト: "/Game/Blueprints")
- `rationale` (string, optional) - ナレッジベース用の設計根拠

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**例:**
```python
add_blueprint_event_node(
    blueprint_name="BP_MyActor",
    event_name="ReceiveBeginPlay",
    node_position=[100, 100],
    path="/Game/Blueprints"
)
```

### add_blueprint_input_action_node

Blueprintのイベントグラフにインプットアクションイベントノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `action_name` (string) - 応答するインプットアクション名
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**例:**
```python
add_blueprint_input_action_node(
    blueprint_name="BP_MyActor",
    action_name="IA_Jump",
    node_position=[200, 200]
)
```

### add_blueprint_function_node

Blueprintのイベントグラフに関数呼び出しノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `target` (string) - 関数のターゲットオブジェクト (コンポーネント名または self)
- `function_name` (string) - 呼び出す関数名
- `params` (object, optional) - 関数ノードに設定するパラメータ
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")
- `rationale` (string, optional) - ナレッジベース用の設計根拠

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**例:**
```python
add_blueprint_function_node(
    blueprint_name="BP_MyActor",
    target="StaticMeshComponent",
    function_name="SetRelativeLocation",
    params={"NewLocation": [0, 0, 100]},
    node_position=[300, 300]
)
```

### connect_blueprint_nodes

Blueprintのイベントグラフ内で2つのノードを接続します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `source_node_id` (string) - ソースノードのID
- `source_pin` (string) - ソースノードの出力ピン名
- `target_node_id` (string) - ターゲットノードのID
- `target_pin` (string) - ターゲットノードの入力ピン名
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- 成功または失敗を示すレスポンス

**例:**
```python
connect_blueprint_nodes(
    blueprint_name="BP_MyActor",
    source_node_id="ABC123...",
    source_pin="then",
    target_node_id="DEF456...",
    target_pin="execute"
)
```

### add_blueprint_variable

Blueprintに変数を追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `variable_name` (string) - 変数名
- `variable_type` (string) - 変数の型 (Boolean, Integer, Float, Vector, String, など)
- `is_exposed` (boolean, optional) - エディタに変数を公開するか (デフォルト: false)
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")
- `rationale` (string, optional) - ナレッジベース用の設計根拠

**戻り値:**
- 成功または失敗を示すレスポンス

**例:**
```python
add_blueprint_variable(
    blueprint_name="BP_MyActor",
    variable_name="Health",
    variable_type="Float",
    is_exposed=True
)
```

### add_blueprint_get_self_component_reference

現在のBlueprintが所有するコンポーネントへの参照を取得するノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `component_name` (string) - 参照を取得するコンポーネント名
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**例:**
```python
add_blueprint_get_self_component_reference(
    blueprint_name="BP_MyActor",
    component_name="StaticMeshComponent",
    node_position=[400, 400]
)
```

### add_blueprint_self_reference

Blueprintのイベントグラフに 'Get Self' ノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**例:**
```python
add_blueprint_self_reference(
    blueprint_name="BP_MyActor",
    node_position=[500, 500]
)
```

### find_blueprint_nodes

Blueprintのイベントグラフ内でノードを検索します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `node_type` (string, optional) - 検索するノードタイプ (Event, Function, Variable, など)
- `event_type` (string, optional) - 検索する特定のイベントタイプ (BeginPlay, Tick, など)
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- 見つかったノードIDの配列と成功ステータスを含むレスポンス

**例:**
```python
find_blueprint_nodes(
    blueprint_name="BP_MyActor",
    node_type="Event",
    event_type="ReceiveBeginPlay"
)
```

---

## ノード操作ツール

これらのツールは、Blueprintグラフの高度なノード操作機能を提供します。

### set_node_pin_value

ノードのピンにデフォルト値を設定します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `node_id` (string) - ノードのGUID (find_blueprint_nodesまたはノード作成から取得)
- `pin_name` (string) - 設定するピン名 (例: PrintStringの "InString")
- `pin_value` (string) - ピンに設定する値
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- 成功または失敗を示すレスポンス

**サポートされているピンタイプ:**
- String / Text
- Integer
- Float / Real
- Boolean
- Name

**例:**
```python
# PrintStringノードのメッセージを設定
set_node_pin_value(
    blueprint_name="BP_Test",
    node_id="ABC123...",
    pin_name="InString",
    pin_value="Hello World!"
)

# 整数値を設定
set_node_pin_value(
    blueprint_name="BP_Test",
    node_id="DEF456...",
    pin_name="Count",
    pin_value="42"
)
```

### add_variable_get_node

変数の値を取得するVariable Getノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `variable_name` (string) - 取得する変数名 (Blueprint内に存在する必要があります)
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**例:**
```python
# まず変数を作成
add_blueprint_variable(
    blueprint_name="BP_Test",
    variable_name="Health",
    variable_type="Float"
)

# 次にGetノードを追加
add_variable_get_node(
    blueprint_name="BP_Test",
    variable_name="Health",
    node_position=[200, 100]
)
```

### add_variable_set_node

変数に値を代入するVariable Setノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `variable_name` (string) - 設定する変数名 (Blueprint内に存在する必要があります)
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**例:**
```python
# まず変数を作成
add_blueprint_variable(
    blueprint_name="BP_Test",
    variable_name="Health",
    variable_type="Float"
)

# 次にSetノードを追加
add_variable_set_node(
    blueprint_name="BP_Test",
    variable_name="Health",
    node_position=[400, 100]
)
```

### add_branch_node

条件分岐用のBranch (if/else) ノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと成功ステータスを含むレスポンス

**ピン:**
- **入力:**
  - `execute` - 実行入力
  - `Condition` - 評価するBoolean条件
- **出力:**
  - `True` - 条件がtrueの場合はここで実行が続行
  - `False` - 条件がfalseの場合はここで実行が続行

**例:**
```python
add_branch_node(
    blueprint_name="BP_Test",
    node_position=[600, 100]
)
```

### delete_blueprint_node

Blueprintのイベントグラフからノードを削除します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `node_id` (string) - 削除するノードのGUID
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- 成功または失敗を示すレスポンス

**注意:** ノードへの/からのすべての接続は削除前に切断されます。

**例:**
```python
delete_blueprint_node(
    blueprint_name="BP_Test",
    node_id="ABC123...",
    path="/Game/Blueprints"
)
```

### move_blueprint_node

Blueprintグラフ内でノードを新しい位置に移動します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `node_id` (string) - 移動するノードのGUID
- `position` (array) - ノードの新しい [X, Y] 位置
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- 新しい位置と成功ステータスを含むレスポンス

**例:**
```python
move_blueprint_node(
    blueprint_name="BP_Test",
    node_id="ABC123...",
    position=[1000, 300]
)
```

---

## 制御フロー＆ユーティリティツール

これらのツールは、Blueprintグラフ用の制御フローノードとユーティリティ機能を提供します。

### add_sequence_node

複数のブランチを順番に実行するSequenceノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `num_outputs` (integer, optional) - 出力実行ピンの数 (2-10, デフォルト: 2)
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと出力数を含むレスポンス

**ピン:**
- **入力:**
  - `execute` - 実行入力
- **出力:**
  - `then_0`, `then_1`, `then_2`, ... - 順次実行される出力

**例:**
```python
add_sequence_node(
    blueprint_name="BP_Test",
    num_outputs=3,
    node_position=[200, 0]
)
```

### add_delay_node

時間遅延実行用のDelayノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `duration` (float, optional) - 遅延時間（秒） (デフォルト: 1.0)
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと遅延時間を含むレスポンス

**ピン:**
- **入力:**
  - `execute` - 実行入力
  - `Duration` - 遅延時間（秒）のFloat値
- **出力:**
  - `then` - 遅延後に発火 (エディタでは "Completed" とラベル表示)

**例:**
```python
add_delay_node(
    blueprint_name="BP_Test",
    duration=2.5,
    node_position=[400, 0]
)
```

### add_foreach_loop_node

**ステータス:** 🚫 **非推奨 (DEPRECATED)**

ForEachループはBlueprintマクロとして実装されており、プログラマティックに追加できません。

**代替手段:** イテレーション処理には `add_forloop_with_break_node` を使用してください。

**詳細:**
- ForEachLoopは `/Engine/EditorBlueprintResources/StandardMacros` に保存されているマクロです
- Unreal Engine 5.7では、マクロノードはプログラマティックな追加に必要なメタデータを持っていません
- 配列要素のイテレーションには、代わりにForLoopWithBreakノードを使用できます

### add_forloop_with_break_node

指定回数だけイテレーションするForLoopWithBreakノードを追加します。

**ステータス:** ✅ **実装済み** (UE 5.7対応)

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `first_index` (integer, optional) - 開始インデックス (デフォルト: 0)
- `last_index` (integer, optional) - 終了インデックス (デフォルト: 10)
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードID、first_index、last_indexを含むレスポンス

**ピン:**
- **入力:**
  - `execute` - 実行入力
  - `FirstIndex` - 開始インデックス
  - `LastIndex` - 終了インデックス
  - `Break` - ループから抜けるために接続
- **出力:**
  - `LoopBody` - 各イテレーションで発火
  - `Index` - 現在のループインデックス
  - `Completed` - ループ完了時に発火

**実装の詳細:**
- `UK2Node_MacroInstance` を使用してStandardMacrosからロード
- UE 5.7で完全にテスト済み
- ノードGUID生成とピン初期化を含む適切な初期化シーケンス実装済み

**例:**
```python
# 0から5まで（合計6回）ループ
add_forloop_with_break_node(
    blueprint_name="BP_Test",
    first_index=0,
    last_index=5,
    node_position=[200, 0]
)
```

**配列イテレーションの使用例:**
```python
# 1. Blueprint作成と配列変数追加
create_blueprint(name="BP_ArrayTest", parent_class="Actor")
add_blueprint_variable(
    blueprint_name="BP_ArrayTest",
    variable_name="MyArray",
    variable_type="Integer"  # 配列型として設定
)

# 2. BeginPlayイベント追加
event = add_blueprint_event_node(
    blueprint_name="BP_ArrayTest",
    event_name="ReceiveBeginPlay"
)

# 3. 配列の長さを取得（Length関数）
length_node = add_blueprint_function_node(
    blueprint_name="BP_ArrayTest",
    target="self",
    function_name="Length",  # 配列の長さを取得
    node_position=[200, 0]
)

# 4. ForLoopWithBreakノード追加
loop = add_forloop_with_break_node(
    blueprint_name="BP_ArrayTest",
    first_index=0,
    last_index=0,  # LastIndexピンに配列長-1を接続
    node_position=[400, 0]
)

# 5. LastIndexピンに配列長を接続
# (Length - 1をLastIndexに接続する減算ノードが必要)
```

### add_print_string_node

デバッグ出力用のPrintStringノードを追加します。

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `message` (string, optional) - 印刷するデフォルトメッセージ (デフォルト: "Hello")
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDとメッセージを含むレスポンス

**ピン:**
- **入力:**
  - `execute` - 実行入力
  - `InString` - 印刷する文字列
  - `bPrintToScreen` - 画面に印刷するか (デフォルト: true)
  - `bPrintToLog` - ログに印刷するか (デフォルト: true)
  - `TextColor` - 画面テキストの色
  - `Duration` - 画面に表示する時間
- **出力:**
  - `then` - 実行が続行

**例:**
```python
add_print_string_node(
    blueprint_name="BP_Test",
    message="Hello from MCP!",
    node_position=[600, 0]
)
```

### add_math_node

数学演算ノード (Add, Subtract, Multiply, Divide) を追加します。

**ステータス:** ✅ **実装済み** (UE 5.7対応 - DoubleDouble関数使用)

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `operation` (string) - 演算の種類 ("Add", "Subtract", "Multiply", "Divide")
- `value_type` (string) - 値の型 ("Float", "Integer")
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと演算情報を含むレスポンス

**実装の詳細:**
- UE 5.7では浮動小数点がfloatからdoubleに変更
- `Add_DoubleDouble`, `Subtract_DoubleDouble`, `Multiply_DoubleDouble`, `Divide_DoubleDouble` を使用
- 整数演算には `Add_IntInt`, `Subtract_IntInt`, `Multiply_IntInt`, `Divide_IntInt` を使用
- `UKismetMathLibrary::FindFunctionByName` で関数を検索
- `SetExternalMember` でK2Node関数参照を設定

**例:**
```python
# Float加算ノード追加
add_math_node(
    blueprint_name="BP_Test",
    operation="Add",
    value_type="Float",
    node_position=[200, 0]
)

# Integer乗算ノード追加
add_math_node(
    blueprint_name="BP_Test",
    operation="Multiply",
    value_type="Integer",
    node_position=[200, 100]
)
```

### add_comparison_node

比較ノード (Greater, Less, Equal, など) を追加します。

**ステータス:** ✅ **実装済み** (UE 5.7対応 - DoubleDouble関数使用)

**パラメータ:**
- `blueprint_name` (string) - 対象のBlueprint名
- `operation` (string) - 比較の種類 ("Greater", "Less", "Equal", "NotEqual", "GreaterEqual", "LessEqual")
- `value_type` (string) - 値の型 ("Float", "Integer")
- `node_position` (array, optional) - グラフ内の [X, Y] 位置 (デフォルト: [0, 0])
- `path` (string, optional) - コンテンツブラウザパス (デフォルト: "/Game/Blueprints")

**戻り値:**
- ノードIDと比較情報を含むレスポンス

**実装の詳細:**
- UE 5.7のdouble型に対応した関数名を使用
- Float比較: `Greater_DoubleDouble`, `Less_DoubleDouble`, `Equal_DoubleDouble`, など
- Integer比較: `Greater_IntInt`, `Less_IntInt`, `Equal_IntInt`, など
- `UKismetMathLibrary::FindFunctionByName` と `SetExternalMember` を使用

**例:**
```python
# Float比較ノード追加 (A > B)
add_comparison_node(
    blueprint_name="BP_Test",
    operation="Greater",
    value_type="Float",
    node_position=[300, 0]
)

# Integer等価比較ノード追加 (A == B)
add_comparison_node(
    blueprint_name="BP_Test",
    operation="Equal",
    value_type="Integer",
    node_position=[300, 100]
)
```

---

## 制御フローワークフロー例

SequenceノードとDelayノードを使用した例:

```python
# 1. Blueprint作成
create_blueprint(
    name="BP_SequenceDemo",
    parent_class="Actor",
    path="/Game/Blueprints"
)

# 2. BeginPlayイベント追加
event_result = add_blueprint_event_node(
    blueprint_name="BP_SequenceDemo",
    event_name="ReceiveBeginPlay",
    node_position=[0, 0]
)

# 3. 3つの出力を持つSequenceノード追加
sequence_result = add_sequence_node(
    blueprint_name="BP_SequenceDemo",
    num_outputs=3,
    node_position=[300, 0]
)

# 4. BeginPlayをSequenceに接続
connect_blueprint_nodes(
    blueprint_name="BP_SequenceDemo",
    source_node_id=event_result["node_id"],
    source_pin="then",
    target_node_id=sequence_result["node_id"],
    target_pin="execute"
)

# 5. 最初の出力用にPrintString追加
print1_result = add_print_string_node(
    blueprint_name="BP_SequenceDemo",
    message="ステップ1: 即座に実行",
    node_position=[600, 0]
)

# 6. 2番目の出力用にDelayノード追加
delay_result = add_delay_node(
    blueprint_name="BP_SequenceDemo",
    duration=2.0,
    node_position=[600, 150]
)

# 7. 遅延後のPrintString追加
print2_result = add_print_string_node(
    blueprint_name="BP_SequenceDemo",
    message="ステップ2: 2秒後に実行",
    node_position=[900, 150]
)

# 8. 3番目の出力用にPrintString追加
print3_result = add_print_string_node(
    blueprint_name="BP_SequenceDemo",
    message="ステップ3: これも即座に実行",
    node_position=[600, 300]
)

# 9. Sequence出力を接続
connect_blueprint_nodes(
    blueprint_name="BP_SequenceDemo",
    source_node_id=sequence_result["node_id"],
    source_pin="then_0",
    target_node_id=print1_result["node_id"],
    target_pin="execute"
)

connect_blueprint_nodes(
    blueprint_name="BP_SequenceDemo",
    source_node_id=sequence_result["node_id"],
    source_pin="then_1",
    target_node_id=delay_result["node_id"],
    target_pin="execute"
)

connect_blueprint_nodes(
    blueprint_name="BP_SequenceDemo",
    source_node_id=delay_result["node_id"],
    source_pin="then",
    target_node_id=print2_result["node_id"],
    target_pin="execute"
)

connect_blueprint_nodes(
    blueprint_name="BP_SequenceDemo",
    source_node_id=sequence_result["node_id"],
    source_pin="then_2",
    target_node_id=print3_result["node_id"],
    target_pin="execute"
)

# 10. コンパイル
compile_blueprint(
    blueprint_name="BP_SequenceDemo",
    path="/Game/Blueprints"
)
```

---

## 完全なワークフロー例

変数とロジックを使用したBlueprintの作成例:

```python
# 1. Blueprint作成
create_blueprint(
    name="BP_HealthSystem",
    parent_class="Actor",
    path="/Game/Blueprints"
)

# 2. 変数追加
add_blueprint_variable(
    blueprint_name="BP_HealthSystem",
    variable_name="CurrentHealth",
    variable_type="Float",
    path="/Game/Blueprints"
)

add_blueprint_variable(
    blueprint_name="BP_HealthSystem",
    variable_name="MaxHealth",
    variable_type="Float",
    path="/Game/Blueprints"
)

add_blueprint_variable(
    blueprint_name="BP_HealthSystem",
    variable_name="IsDead",
    variable_type="Boolean",
    path="/Game/Blueprints"
)

# 3. BeginPlayイベント追加
event_result = add_blueprint_event_node(
    blueprint_name="BP_HealthSystem",
    event_name="ReceiveBeginPlay",
    node_position=[0, 0],
    path="/Game/Blueprints"
)

# 4. ヘルス初期化用の変数Setノード追加
set_result = add_variable_set_node(
    blueprint_name="BP_HealthSystem",
    variable_name="CurrentHealth",
    node_position=[300, 0],
    path="/Game/Blueprints"
)

# 5. Setノードに初期値設定
set_node_pin_value(
    blueprint_name="BP_HealthSystem",
    node_id=set_result["node_id"],
    pin_name="CurrentHealth",
    pin_value="100.0",
    path="/Game/Blueprints"
)

# 6. BeginPlayをSetノードに接続
connect_blueprint_nodes(
    blueprint_name="BP_HealthSystem",
    source_node_id=event_result["node_id"],
    source_pin="then",
    target_node_id=set_result["node_id"],
    target_pin="execute",
    path="/Game/Blueprints"
)

# 7. 死亡チェック用のBranchノード追加
branch_result = add_branch_node(
    blueprint_name="BP_HealthSystem",
    node_position=[600, 0],
    path="/Game/Blueprints"
)

# 8. ヘルスチェック用の変数Getノード追加
get_result = add_variable_get_node(
    blueprint_name="BP_HealthSystem",
    variable_name="CurrentHealth",
    node_position=[400, 100],
    path="/Game/Blueprints"
)

# 9. Blueprintをコンパイル
compile_blueprint(
    blueprint_name="BP_HealthSystem",
    path="/Game/Blueprints"
)
```

---

## エラーハンドリング

すべてのコマンドレスポンスには、操作が成功したかどうかを示すstatusフィールドが含まれます:

**成功レスポンス:**
```json
{
  "status": "success",
  "result": {
    "node_id": "ABC123...",
    "variable_name": "Health"
  }
}
```

**エラーレスポンス:**
```json
{
  "status": "error",
  "error": "Variable not found in blueprint: Health"
}
```

---

## 型リファレンス

### ノードタイプ

`find_blueprint_nodes` コマンドで使用される一般的なノードタイプ:

- `Event` - イベントノード (BeginPlay, Tick, など)
- `Function` - 関数呼び出しノード
- `Variable` - 変数ノード
- `Component` - コンポーネント参照ノード
- `Self` - Self参照ノード

### 変数タイプ

`add_blueprint_variable` コマンドで使用される一般的な変数タイプ:

- `Boolean` - True/false値
- `Integer` / `Int` - 整数
- `Float` - 浮動小数点数
- `Vector` - 3Dベクトル値 (X, Y, Z)
- `String` - テキスト値

### ピン名

Blueprintノードで使用される一般的なピン名:

| ノードタイプ | 入力ピン | 出力ピン |
|------------|---------|---------|
| Event | - | `then`, 戻り値 |
| Branch | `execute`, `Condition` | `True`, `False` |
| Set Variable | `execute`, 変数値 | `then`, 変数値 |
| Get Variable | - | 変数値 |
| Function Call | `execute`, パラメータ | `then`, 戻り値 |
| Math (Add/Subtract/etc) | `execute`, `A`, `B` | `then`, `ReturnValue` |
| Comparison (Greater/Less/etc) | `A`, `B` | `ReturnValue` (Boolean) |
| ForLoopWithBreak | `execute`, `FirstIndex`, `LastIndex`, `Break` | `LoopBody`, `Index`, `Completed` |

---

## 実装ステータス

### ✅ 完全実装済み
- 基本ノード操作 (Event, Function, Variable, Connection)
- ノード操作ツール (Set Pin Value, Get/Set Variable, Branch, Delete, Move)
- 制御フローノード (Sequence, Delay, ForLoopWithBreak, PrintString)
- 数学・比較ノード (Math, Comparison) - UE 5.7対応済み

### 🚫 非推奨
- `add_foreach_loop_node` - Blueprintマクロのため実装不可。`add_forloop_with_break_node` を使用してください。

### 技術的な注意事項

**UE 5.7 互換性:**
- Math/Comparisonノードはfloat→double型変更に対応
- `FloatFloat` 関数から `DoubleDouble` 関数に変更
- `FindFunctionByName` による実行時関数検索を使用

**ノード初期化シーケンス:**
適切なノードGUID生成のため、以下の順序が必要:
1. `AddNode()` - グラフにノード追加
2. `CreateNewGuid()` - 固有のGUID生成
3. `PostPlacedNewNode()` - 配置後の初期化
4. `AllocateDefaultPins()` - ピン作成
