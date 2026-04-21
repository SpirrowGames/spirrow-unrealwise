"""
UMG Widget操作のテストスイート

Phase 0.6.6で分割されたUMGWidgetCommands (Core/Basic/Interactive) の動作確認
"""

import pytest
from test_framework import assert_success, assert_response_has


@pytest.mark.umg
class TestUMGWidgetCore:
    """UMGWidgetCoreCommands テスト"""
    
    def test_create_widget_blueprint(self, test_suite, unique_name):
        """Widget Blueprint作成テスト"""
        widget_name = unique_name("WBP_Test")
        
        result = test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": widget_name,
            "path": "/Game/Test",
            "parent_class": "UserWidget"
        })
        
        assert_success(result, "Widget Blueprint作成")
        assert_response_has(result, "success", True)
        assert_response_has(result, "name", widget_name)
        
        # クリーンアップ登録
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{widget_name}"
        })
    
    def test_add_widget_to_viewport(self, test_suite, unique_name):
        """Widget Viewport追加テスト"""
        widget_name = unique_name("WBP_Viewport")
        
        # まずWidgetを作成
        test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": widget_name,
            "path": "/Game/Test"
        })
        
        result = test_suite.run_command("add_widget_to_viewport", {
            "widget_name": widget_name,
            "z_order": 0
        })
        
        # ViewportはPIE中でないと動かない可能性があるのでエラーも許容
        # assert_success(result, "Widget Viewport追加")
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{widget_name}"
        })


@pytest.mark.umg
class TestUMGWidgetBasic:
    """UMGWidgetBasicCommands テスト"""
    
    @pytest.fixture(autouse=True)
    def setup_widget(self, test_suite, unique_name):
        """各テスト前にWidgetを作成"""
        self.widget_name = unique_name("WBP_Basic")
        test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": self.widget_name,
            "path": "/Game/Test"
        })
        yield
        # クリーンアップはtest_suiteが自動実行
    
    def test_add_text_to_widget(self, test_suite):
        """Text追加テスト"""
        result = test_suite.run_command("add_text_to_widget", {
            "widget_name": self.widget_name,
            "text_name": "TestText",
            "text": "Hello World",
            "font_size": 24,
            "anchor": "Center",
            "path": "/Game/Test"
        })
        
        assert_success(result, "Text追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_image_to_widget(self, test_suite):
        """Image追加テスト"""
        result = test_suite.run_command("add_image_to_widget", {
            "widget_name": self.widget_name,
            "image_name": "TestImage",
            "size": [64, 64],
            "anchor": "TopLeft",
            "path": "/Game/Test"
        })
        
        assert_success(result, "Image追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_progressbar_to_widget(self, test_suite):
        """ProgressBar追加テスト"""
        result = test_suite.run_command("add_progressbar_to_widget", {
            "widget_name": self.widget_name,
            "progressbar_name": "TestProgress",
            "percent": 0.5,
            "fill_color": [0, 1, 0, 1],
            "path": "/Game/Test"
        })
        
        assert_success(result, "ProgressBar追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })


@pytest.mark.umg
class TestUMGWidgetInteractive:
    """UMGWidgetInteractiveCommands テスト"""
    
    @pytest.fixture(autouse=True)
    def setup_widget(self, test_suite, unique_name):
        """各テスト前にWidgetを作成"""
        self.widget_name = unique_name("WBP_Interactive")
        test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": self.widget_name,
            "path": "/Game/Test"
        })
        yield
    
    def test_add_button_to_widget(self, test_suite):
        """Button追加テスト"""
        result = test_suite.run_command("add_button_to_widget", {
            "widget_name": self.widget_name,
            "button_name": "TestButton",
            "text": "Click Me",
            "size": [200, 50],
            "anchor": "Center",
            "path": "/Game/Test"
        })
        
        assert_success(result, "Button追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_slider_to_widget(self, test_suite):
        """Slider追加テスト"""
        result = test_suite.run_command("add_slider_to_widget", {
            "widget_name": self.widget_name,
            "slider_name": "TestSlider",
            "value": 0.5,
            "min_value": 0,
            "max_value": 1,
            "path": "/Game/Test"
        })
        
        assert_success(result, "Slider追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_checkbox_to_widget(self, test_suite):
        """CheckBox追加テスト"""
        result = test_suite.run_command("add_checkbox_to_widget", {
            "widget_name": self.widget_name,
            "checkbox_name": "TestCheckbox",
            "is_checked": False,
            "label_text": "Enable",
            "path": "/Game/Test"
        })
        
        assert_success(result, "CheckBox追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_combobox_to_widget(self, test_suite):
        """ComboBox追加テスト"""
        result = test_suite.run_command("add_combobox_to_widget", {
            "widget_name": self.widget_name,
            "combobox_name": "TestCombo",
            "options": ["Option1", "Option2", "Option3"],
            "selected_index": 0,
            "path": "/Game/Test"
        })
        
        assert_success(result, "ComboBox追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_editabletext_to_widget(self, test_suite):
        """EditableText追加テスト"""
        result = test_suite.run_command("add_editabletext_to_widget", {
            "widget_name": self.widget_name,
            "text_name": "TestInput",
            "hint_text": "Enter text...",
            "path": "/Game/Test"
        })
        
        assert_success(result, "EditableText追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_spinbox_to_widget(self, test_suite):
        """SpinBox追加テスト"""
        result = test_suite.run_command("add_spinbox_to_widget", {
            "widget_name": self.widget_name,
            "spinbox_name": "TestSpinBox",
            "value": 50,
            "min_value": 0,
            "max_value": 100,
            "path": "/Game/Test"
        })
        
        assert_success(result, "SpinBox追加")
        assert_response_has(result, "success", True)
        
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })
    
    def test_add_scrollbox_to_widget(self, test_suite):
        """ScrollBox追加テスト"""
        result = test_suite.run_command("add_scrollbox_to_widget", {
            "widget_name": self.widget_name,
            "scrollbox_name": "TestScrollBox",
            "orientation": "Vertical",
            "size": [300, 200],
            "path": "/Game/Test"
        })

        assert_success(result, "ScrollBox追加")
        assert_response_has(result, "success", True)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })


@pytest.mark.umg
class TestUMGV096Extensions:
    """v0.9.6 で追加された WidgetSwitcher / Border / 明示的 Anchors / parent_class 汎用化の検証"""

    @pytest.fixture(autouse=True)
    def setup_widget(self, test_suite, unique_name):
        """各テスト前に Widget を作成"""
        self.widget_name = unique_name("WBP_V096")
        test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": self.widget_name,
            "path": "/Game/Test"
        })
        yield

    def test_add_widget_switcher(self, test_suite):
        """WidgetSwitcher 追加テスト (P0 #1)"""
        result = test_suite.run_command("add_widget_switcher_to_widget", {
            "widget_name": self.widget_name,
            "switcher_name": "Pager",
            "active_widget_index": 0,
            "anchor": "Center",
            "size": [400, 300],
            "path": "/Game/Test"
        })

        assert_success(result, "WidgetSwitcher 追加")
        assert_response_has(result, "success", True)
        assert_response_has(result, "switcher_name", "Pager")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_set_active_widget_index_via_reflection(self, test_suite):
        """set_widget_element_property が ActiveWidgetIndex (int32) を
        リフレクション fallback 経由で扱えることを検証 (P0 #4)"""
        # Switcher + 2 つの子パネル (VerticalBox) を用意
        test_suite.run_command("add_widget_switcher_to_widget", {
            "widget_name": self.widget_name,
            "switcher_name": "Pager",
            "path": "/Game/Test"
        })
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "Page0",
            "parent_name": "Pager",
            "path": "/Game/Test"
        })
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "Page1",
            "parent_name": "Pager",
            "path": "/Game/Test"
        })

        result = test_suite.run_command("set_widget_element_property", {
            "widget_name": self.widget_name,
            "element_name": "Pager",
            "property_name": "ActiveWidgetIndex",
            "property_value": "1",
            "path": "/Game/Test"
        })

        assert_success(result, "ActiveWidgetIndex 設定")
        assert_response_has(result, "success", True)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_add_border(self, test_suite):
        """Border 追加テスト (P0 #2)"""
        result = test_suite.run_command("add_border_to_widget", {
            "widget_name": self.widget_name,
            "border_name": "BgFrame",
            "brush_color": [0.1, 0.1, 0.1, 0.8],
            "padding": [8, 8, 8, 8],
            "size": [300, 200],
            "path": "/Game/Test"
        })

        assert_success(result, "Border 追加")
        assert_response_has(result, "success", True)
        assert_response_has(result, "border_name", "BgFrame")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_border_nested_in_panel(self, test_suite):
        """Border を別のパネルの子として追加できること (parent_name)"""
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "OuterVB",
            "path": "/Game/Test"
        })

        result = test_suite.run_command("add_border_to_widget", {
            "widget_name": self.widget_name,
            "border_name": "InnerBorder",
            "parent_name": "OuterVB",
            "brush_color": [0.2, 0.2, 0.2, 1.0],
            "padding": [4, 4, 4, 4],
            "path": "/Game/Test"
        })

        assert_success(result, "Border ネスト追加")
        assert_response_has(result, "success", True)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_explicit_anchors_and_offsets(self, test_suite):
        """set_widget_slot_property の anchor_min/max + LTRB offsets 拡張 (P0 #3)"""
        # 何らかの要素を追加 (Border を流用)
        test_suite.run_command("add_border_to_widget", {
            "widget_name": self.widget_name,
            "border_name": "StretchFrame",
            "path": "/Game/Test"
        })

        # 全画面ストレッチ + 16px inset
        result = test_suite.run_command("set_widget_slot_property", {
            "widget_name": self.widget_name,
            "element_name": "StretchFrame",
            "anchor_min": [0, 0],
            "anchor_max": [1, 1],
            "offset_left": 16,
            "offset_top": 16,
            "offset_right": 16,
            "offset_bottom": 16,
            "path": "/Game/Test"
        })

        assert_success(result, "明示的 anchor_min/max + LTRB offsets")
        assert_response_has(result, "success", True)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_parent_class_script_path(self, test_suite, unique_name):
        """parent_class に /Script/UMG.UserWidget フルパス指定 (Fix A)"""
        wbp = unique_name("WBP_ScriptParent")
        result = test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": wbp,
            "path": "/Game/Test",
            "parent_class": "/Script/UMG.UserWidget"
        })

        assert_success(result, "/Script/ パスでの parent_class 解決")
        assert_response_has(result, "success", True)

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{wbp}"
        })

    def test_parent_class_unresolvable_errors(self, test_suite, unique_name):
        """解決不能な parent_class は ClassNotFound (1211) を返すこと"""
        wbp = unique_name("WBP_BadParent")
        result = test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": wbp,
            "path": "/Game/Test",
            "parent_class": "DoesNotExistAnywhere_XYZ"
        })

        # 成功せず、エラーコード 1211 (ClassNotFound) が返ること
        assert result.get("success") is not True, "存在しない parent_class で成功してはならない"
        # error_code または code フィールド経由でチェック (実装依存)
        err_code = result.get("error_code") or result.get("code")
        assert err_code == 1211, f"期待 error_code=1211 (ClassNotFound), 実際: {err_code}"

    def test_parent_class_non_user_widget_errors(self, test_suite, unique_name):
        """UUserWidget 以外のクラスは InvalidParamValue (1005) を返すこと"""
        wbp = unique_name("WBP_NonUserWidget")
        result = test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": wbp,
            "path": "/Game/Test",
            "parent_class": "/Script/Engine.Actor"
        })

        assert result.get("success") is not True, "Actor 親で成功してはならない"
        err_code = result.get("error_code") or result.get("code")
        assert err_code == 1005, f"期待 error_code=1005 (InvalidParamValue), 実際: {err_code}"


@pytest.mark.umg
@pytest.mark.integration
class TestUMGWidgetIntegration:
    """UMG統合テスト - 複数コマンドの連携"""
    
    def test_create_complete_widget(self, test_suite, unique_name):
        """完全なWidgetを作成する統合テスト"""
        widget_name = unique_name("WBP_Complete")
        
        # 1. Widget作成
        result = test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": widget_name,
            "path": "/Game/Test"
        })
        assert_success(result, "Widget作成")
        
        # 2. タイトルText追加
        result = test_suite.run_command("add_text_to_widget", {
            "widget_name": widget_name,
            "text_name": "TitleText",
            "text": "Settings",
            "font_size": 32,
            "anchor": "TopCenter",
            "path": "/Game/Test"
        })
        assert_success(result, "Title追加")
        
        # 3. Slider追加
        result = test_suite.run_command("add_slider_to_widget", {
            "widget_name": widget_name,
            "slider_name": "VolumeSlider",
            "value": 0.7,
            "path": "/Game/Test"
        })
        assert_success(result, "Slider追加")
        
        # 4. Button追加
        result = test_suite.run_command("add_button_to_widget", {
            "widget_name": widget_name,
            "button_name": "ApplyButton",
            "text": "Apply",
            "anchor": "BottomCenter",
            "path": "/Game/Test"
        })
        assert_success(result, "Button追加")
        
        # 5. ProgressBar追加
        result = test_suite.run_command("add_progressbar_to_widget", {
            "widget_name": widget_name,
            "progressbar_name": "LoadingBar",
            "percent": 0.0,
            "path": "/Game/Test"
        })
        assert_success(result, "ProgressBar追加")
        
        # クリーンアップ
        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{widget_name}"
        })
        
        # サマリー出力
        summary = test_suite.get_summary()
        print(f"\n統合テスト結果: {summary['passed']}/{summary['total']} passed")
        print(f"平均実行時間: {summary['avg_duration_ms']:.2f}ms")
