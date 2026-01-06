# SpirrowUnrealWise テストスイート

MCPサーバーの自動テストフレームワーク

## 📁 ファイル構成

```
tests/
├── __init__.py
├── conftest.py          # pytest fixtures
├── test_framework.py    # テストフレームワーク
├── test_umg_widgets.py  # UMG Widgetテスト
├── test_blueprints.py   # Blueprintテスト
├── test_ai_tools.py     # AI (BehaviorTree/Blackboard) テスト
├── run_tests.py         # テストランナー
├── smoke_test.py        # クイックスモークテスト
└── README.md            # このファイル
```

## 🚀 クイックスタート

### 前提条件

1. Unreal Editorが起動していること
2. SpirrowBridgeプラグインが有効でMCPサーバーが稼働中
3. Python 3.10+

### 依存関係インストール

```bash
cd Python
pip install -e ".[test]"
```

### スモークテスト実行

```bash
cd Python/tests
python smoke_test.py
```

### 全テスト実行

```bash
cd Python/tests
python run_tests.py
```

## 🧪 テストコマンド

### マーカー別実行

```bash
# UMGテストのみ
python run_tests.py -m umg

# Blueprintテストのみ
python run_tests.py -m blueprint

# AIテストのみ (BehaviorTree/Blackboard)
python run_tests.py -m ai

# ノードテストのみ
python run_tests.py -m node

# 統合テストのみ
python run_tests.py -m integration
```

### その他のオプション

```bash
# 詳細出力
python run_tests.py -v

# キーワードフィルタ
python run_tests.py -k "button"

# HTMLレポート出力
python run_tests.py --html=report.html

# 特定ファイルのみ
python run_tests.py test_umg_widgets.py
```

## 📊 テスト構成

### UMG Widgetテスト (`test_umg_widgets.py`)

| クラス | テスト数 | 内容 |
|--------|---------|------|
| `TestUMGWidgetCore` | 2 | Widget作成、Viewport追加 |
| `TestUMGWidgetBasic` | 3 | Text, Image, ProgressBar |
| `TestUMGWidgetInteractive` | 7 | Button, Slider, CheckBox, ComboBox, EditableText, SpinBox, ScrollBox |
| `TestUMGWidgetIntegration` | 1 | 複数コマンド連携 |

### Blueprintテスト (`test_blueprints.py`)

| クラス | テスト数 | 内容 |
|--------|---------|------|
| `TestBlueprintCore` | 3 | 作成、コンパイル、複製 |
| `TestBlueprintComponent` | 2 | コンポーネント追加、メッシュ設定 |
| `TestBlueprintNode` | 5 | Event, PrintString, Delay, Branch, Sequence |
| `TestBlueprintIntegration` | 1 | 完全なBlueprint作成 |

### AIテスト (`test_ai_tools.py`)

| クラス | テスト数 | 内容 |
|--------|---------|------|
| `TestBlackboard` | 8 | Blackboard作成、キー追加（Bool/Int/Float/Vector/Object）、キー削除、一覧取得 |
| `TestBehaviorTree` | 4 | BehaviorTree作成、Blackboard連携、Blackboard設定、構造取得 |
| `TestAIUtility` | 3 | AIアセット一覧（全て/Blackboardのみ/BehaviorTreeのみ） |
| `TestAIIntegration` | 1 | 完全なAIシステム作成（Blackboard+BehaviorTree） |

## 🛠️ テストフレームワーク

### UnrealMCPClient

MCPサーバーとの通信をラップ:

```python
from test_framework import UnrealMCPClient

client = UnrealMCPClient(host="127.0.0.1", port=55557)
result = client.send_command("create_blueprint", {"name": "TestBP", ...})
```

### TestSuite

テスト実行とクリーンアップ管理:

```python
from test_framework import TestSuite

suite = TestSuite(client)
result = suite.run_command("create_blueprint", {...})
suite.add_cleanup("delete_asset", {"asset_path": "/Game/Test/TestBP"})
suite.cleanup()  # 登録順の逆順でクリーンアップ
```

### アサーション

```python
from test_framework import assert_success, assert_response_has

assert_success(result, "エラーメッセージ")
assert_response_has(result, "success", True)
```

## 📝 テスト追加方法

1. `test_*.py` ファイルを作成
2. クラス名は `Test*` で開始
3. メソッド名は `test_*` で開始
4. 適切なマーカーを付与 (`@pytest.mark.umg` など)

```python
import pytest
from test_framework import assert_success

@pytest.mark.umg
class TestMyFeature:
    def test_something(self, test_suite, unique_name):
        result = test_suite.run_command("my_command", {...})
        assert_success(result)
        test_suite.add_cleanup("delete_asset", {...})
```

## ⚠️ 注意事項

- テストは `/Game/Test/` フォルダ内にアセットを作成
- 各テスト後に自動クリーンアップ実行
- Unreal Editorが起動していないとテスト失敗
- PIE実行中のテストは一部制限あり
