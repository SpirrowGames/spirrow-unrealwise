"""
AI操作のテストスイート

BehaviorTree / Blackboard 作成・操作のテスト
"""

import pytest
from test_framework import assert_success, assert_response_has


@pytest.mark.ai
class TestBlackboard:
    """Blackboard操作テスト"""

    def test_create_blackboard(self, test_suite, unique_name):
        """Blackboard作成テスト"""
        bb_name = unique_name("BB_Test")

        result = test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "Blackboard作成")
        assert_response_has(result, "success", True)
        assert_response_has(result, "name", bb_name)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_add_blackboard_key_bool(self, test_suite, unique_name):
        """Blackboard Bool型キー追加テスト"""
        bb_name = unique_name("BB_BoolKey")

        # Blackboard作成
        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        # Bool型キー追加
        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "IsAlerted",
            "key_type": "Bool",
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "Bool型キー追加")
        assert_response_has(result, "key_name", "IsAlerted")
        assert_response_has(result, "total_keys", 1)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_add_blackboard_key_int(self, test_suite, unique_name):
        """Blackboard Int型キー追加テスト"""
        bb_name = unique_name("BB_IntKey")

        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "PatrolIndex",
            "key_type": "Int",
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "Int型キー追加")
        assert_response_has(result, "key_name", "PatrolIndex")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_add_blackboard_key_float(self, test_suite, unique_name):
        """Blackboard Float型キー追加テスト"""
        bb_name = unique_name("BB_FloatKey")

        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "Speed",
            "key_type": "Float",
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "Float型キー追加")
        assert_response_has(result, "key_name", "Speed")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_add_blackboard_key_vector(self, test_suite, unique_name):
        """Blackboard Vector型キー追加テスト"""
        bb_name = unique_name("BB_VectorKey")

        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "MoveToLocation",
            "key_type": "Vector",
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "Vector型キー追加")
        assert_response_has(result, "key_name", "MoveToLocation")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_add_blackboard_key_object(self, test_suite, unique_name):
        """Blackboard Object型キー追加テスト（BaseClass指定）"""
        bb_name = unique_name("BB_ObjectKey")

        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "TargetActor",
            "key_type": "Object",
            "base_class": "Actor",
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "Object型キー追加")
        assert_response_has(result, "key_name", "TargetActor")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_list_blackboard_keys(self, test_suite, unique_name):
        """Blackboardキー一覧取得テスト"""
        bb_name = unique_name("BB_ListKeys")

        # Blackboard作成
        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        # 複数キー追加
        test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "IsAlerted",
            "key_type": "Bool",
            "path": "/Game/Test/AI/Blackboards"
        })

        test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "PatrolIndex",
            "key_type": "Int",
            "path": "/Game/Test/AI/Blackboards"
        })

        test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "TargetActor",
            "key_type": "Object",
            "base_class": "Actor",
            "path": "/Game/Test/AI/Blackboards"
        })

        # キー一覧取得
        result = test_suite.run_command("list_blackboard_keys", {
            "blackboard_name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "キー一覧取得")
        assert_response_has(result, "count", 3)
        assert "keys" in result

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_remove_blackboard_key(self, test_suite, unique_name):
        """Blackboardキー削除テスト"""
        bb_name = unique_name("BB_RemoveKey")

        # Blackboard作成とキー追加
        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "TempKey",
            "key_type": "Bool",
            "path": "/Game/Test/AI/Blackboards"
        })

        # キー削除
        result = test_suite.run_command("remove_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "TempKey",
            "path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "キー削除")
        assert_response_has(result, "removed_key", "TempKey")
        assert_response_has(result, "remaining_keys", 0)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })


@pytest.mark.ai
class TestBehaviorTree:
    """BehaviorTree操作テスト"""

    def test_create_behavior_tree(self, test_suite, unique_name):
        """BehaviorTree作成テスト（Blackboardなし）"""
        bt_name = unique_name("BT_Test")

        result = test_suite.run_command("create_behavior_tree", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees"
        })

        assert_success(result, "BehaviorTree作成")
        assert_response_has(result, "success", True)
        assert_response_has(result, "name", bt_name)
        assert_response_has(result, "has_blackboard", False)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/BehaviorTrees/{bt_name}"
        })

    def test_create_behavior_tree_with_blackboard(self, test_suite, unique_name):
        """BehaviorTree作成テスト（Blackboard連携）"""
        bb_name = unique_name("BB_ForBT")
        bt_name = unique_name("BT_WithBB")

        # Blackboard作成
        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        # BehaviorTree作成（Blackboard連携）
        result = test_suite.run_command("create_behavior_tree", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees",
            "blackboard_name": bb_name,
            "blackboard_path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "BehaviorTree作成（BB連携）")
        assert_response_has(result, "has_blackboard", True)
        assert_response_has(result, "blackboard_name", bb_name)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/BehaviorTrees/{bt_name}"
        })
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_set_behavior_tree_blackboard(self, test_suite, unique_name):
        """BehaviorTreeにBlackboard設定テスト"""
        bb_name = unique_name("BB_ForSet")
        bt_name = unique_name("BT_SetBB")

        # Blackboard作成
        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        # BehaviorTree作成（Blackboardなし）
        test_suite.run_command("create_behavior_tree", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees"
        })

        # Blackboard設定
        result = test_suite.run_command("set_behavior_tree_blackboard", {
            "behavior_tree_name": bt_name,
            "blackboard_name": bb_name,
            "behavior_tree_path": "/Game/Test/AI/BehaviorTrees",
            "blackboard_path": "/Game/Test/AI/Blackboards"
        })

        assert_success(result, "Blackboard設定")
        assert_response_has(result, "behavior_tree_name", bt_name)
        assert_response_has(result, "blackboard_name", bb_name)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/BehaviorTrees/{bt_name}"
        })
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_get_behavior_tree_structure(self, test_suite, unique_name):
        """BehaviorTree構造取得テスト"""
        bb_name = unique_name("BB_Struct")
        bt_name = unique_name("BT_Struct")

        # Blackboard作成とキー追加
        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "TestKey1",
            "key_type": "Bool",
            "path": "/Game/Test/AI/Blackboards"
        })

        test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "TestKey2",
            "key_type": "Int",
            "path": "/Game/Test/AI/Blackboards"
        })

        # BehaviorTree作成（Blackboard連携）
        test_suite.run_command("create_behavior_tree", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees",
            "blackboard_name": bb_name,
            "blackboard_path": "/Game/Test/AI/Blackboards"
        })

        # 構造取得
        result = test_suite.run_command("get_behavior_tree_structure", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees"
        })

        assert_success(result, "BT構造取得")
        assert_response_has(result, "name", bt_name)
        assert_response_has(result, "blackboard_name", bb_name)
        assert_response_has(result, "blackboard_key_count", 2)
        assert_response_has(result, "has_root_node", False)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/BehaviorTrees/{bt_name}"
        })
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })


@pytest.mark.ai
class TestAIUtility:
    """AIユーティリティテスト"""

    def test_list_ai_assets_all(self, test_suite, unique_name):
        """AIアセット一覧取得テスト（全て）"""
        bb_name = unique_name("BB_List")
        bt_name = unique_name("BT_List")

        # アセット作成
        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        test_suite.run_command("create_behavior_tree", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees"
        })

        # 一覧取得
        result = test_suite.run_command("list_ai_assets", {
            "asset_type": "all",
            "path_filter": "/Game/Test/AI/"
        })

        assert_success(result, "AIアセット一覧取得")
        assert "behavior_trees" in result
        assert "blackboards" in result
        assert "total_behavior_trees" in result
        assert "total_blackboards" in result

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/BehaviorTrees/{bt_name}"
        })
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_list_ai_assets_blackboard_only(self, test_suite, unique_name):
        """AIアセット一覧取得テスト（Blackboardのみ）"""
        bb_name = unique_name("BB_ListOnly")

        test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })

        result = test_suite.run_command("list_ai_assets", {
            "asset_type": "blackboard",
            "path_filter": "/Game/Test/AI/"
        })

        assert_success(result, "Blackboard一覧取得")
        assert "blackboards" in result

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

    def test_list_ai_assets_behavior_tree_only(self, test_suite, unique_name):
        """AIアセット一覧取得テスト（BehaviorTreeのみ）"""
        bt_name = unique_name("BT_ListOnly")

        test_suite.run_command("create_behavior_tree", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees"
        })

        result = test_suite.run_command("list_ai_assets", {
            "asset_type": "behavior_tree",
            "path_filter": "/Game/Test/AI/"
        })

        assert_success(result, "BehaviorTree一覧取得")
        assert "behavior_trees" in result

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/BehaviorTrees/{bt_name}"
        })


@pytest.mark.ai
@pytest.mark.integration
class TestAIIntegration:
    """AI統合テスト"""

    def test_create_complete_ai_system(self, test_suite, unique_name):
        """完全なAIシステムを作成する統合テスト"""
        bb_name = unique_name("BB_Complete")
        bt_name = unique_name("BT_Complete")

        # 1. Blackboard作成
        result = test_suite.run_command("create_blackboard", {
            "name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "Blackboard作成")

        # 2. Blackboardキー追加（複数タイプ）
        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "IsAlerted",
            "key_type": "Bool",
            "path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "Bool型キー追加")

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "PatrolIndex",
            "key_type": "Int",
            "path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "Int型キー追加")

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "Speed",
            "key_type": "Float",
            "path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "Float型キー追加")

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "TargetActor",
            "key_type": "Object",
            "base_class": "Actor",
            "path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "Object型キー追加")

        result = test_suite.run_command("add_blackboard_key", {
            "blackboard_name": bb_name,
            "key_name": "MoveToLocation",
            "key_type": "Vector",
            "path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "Vector型キー追加")

        # 3. キー一覧確認
        result = test_suite.run_command("list_blackboard_keys", {
            "blackboard_name": bb_name,
            "path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "キー一覧取得")
        assert_response_has(result, "count", 5)

        # 4. BehaviorTree作成（Blackboard連携）
        result = test_suite.run_command("create_behavior_tree", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees",
            "blackboard_name": bb_name,
            "blackboard_path": "/Game/Test/AI/Blackboards"
        })
        assert_success(result, "BehaviorTree作成")
        assert_response_has(result, "has_blackboard", True)

        # 5. BehaviorTree構造確認
        result = test_suite.run_command("get_behavior_tree_structure", {
            "name": bt_name,
            "path": "/Game/Test/AI/BehaviorTrees"
        })
        assert_success(result, "BT構造取得")
        assert_response_has(result, "blackboard_name", bb_name)
        assert_response_has(result, "blackboard_key_count", 5)

        # 6. AIアセット一覧確認
        result = test_suite.run_command("list_ai_assets", {
            "asset_type": "all",
            "path_filter": "/Game/Test/AI/"
        })
        assert_success(result, "AIアセット一覧取得")

        # クリーンアップ
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/BehaviorTrees/{bt_name}"
        })
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/AI/Blackboards/{bb_name}"
        })

        # サマリー出力
        summary = test_suite.get_summary()
        print(f"\n統合テスト結果: {summary['passed']}/{summary['total']} passed")
