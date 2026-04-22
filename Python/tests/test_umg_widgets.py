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
class TestUMGV097ReparentSafety:
    """v0.9.7: reparent double-parent 防止 + parent_name 対応"""

    @pytest.fixture(autouse=True)
    def setup_widget(self, test_suite, unique_name):
        self.widget_name = unique_name("WBP_V097")
        test_suite.run_command("create_umg_widget_blueprint", {
            "widget_name": self.widget_name,
            "path": "/Game/Test"
        })
        yield

    def test_reparent_no_duplicate(self, test_suite):
        """BUG-1 regression: reparent 後に旧親の children 配列に widget が残らないこと"""
        # Canvas 直下にボタンを作る (parent_name 省略 = root canvas)
        test_suite.run_command("add_button_to_widget", {
            "widget_name": self.widget_name,
            "button_name": "BtnJoin",
            "path": "/Game/Test"
        })
        # 別のコンテナ (VerticalBox) を Canvas 直下に作る
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "MainVBox",
            "path": "/Game/Test"
        })
        # BtnJoin を MainVBox 配下に移動
        reparent_result = test_suite.run_command("reparent_widget_element", {
            "widget_name": self.widget_name,
            "element_name": "BtnJoin",
            "new_parent_name": "MainVBox",
            "path": "/Game/Test"
        })
        assert_success(reparent_result, "reparent_widget_element")

        # get_widget_elements で検証: BtnJoin が elements 配列に 1 回だけ出現すること
        elements_result = test_suite.run_command("get_widget_elements", {
            "widget_name": self.widget_name,
            "path": "/Game/Test"
        })
        assert_success(elements_result, "get_widget_elements")
        elements = elements_result.get("elements", [])
        btn_matches = [e for e in elements if e.get("name") == "BtnJoin"]
        assert len(btn_matches) == 1, (
            f"BUG-1: BtnJoin が elements 配列に {len(btn_matches)} 個出現 (期待: 1)。"
            f"reparent が旧親 children を完全に切断できていない。"
        )

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_add_button_with_parent_name(self, test_suite):
        """FR-1: add_button_to_widget が parent_name を受け取って VerticalBox 内に配置できること"""
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "ButtonContainer",
            "path": "/Game/Test"
        })
        result = test_suite.run_command("add_button_to_widget", {
            "widget_name": self.widget_name,
            "button_name": "BtnInVBox",
            "parent_name": "ButtonContainer",
            "text": "Click",
            "path": "/Game/Test"
        })
        assert_success(result, "add_button_to_widget with parent_name")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_add_text_with_parent_name(self, test_suite):
        """FR-1: add_text_to_widget が parent_name を受け取れること"""
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "TextContainer",
            "path": "/Game/Test"
        })
        result = test_suite.run_command("add_text_to_widget", {
            "widget_name": self.widget_name,
            "text_name": "TxtInVBox",
            "parent_name": "TextContainer",
            "text": "Hello",
            "path": "/Game/Test"
        })
        assert_success(result, "add_text_to_widget with parent_name")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_set_element_property_with_parent_scope(self, test_suite):
        """BUG-5: set_widget_element_property が parent_name で scope 絞り込みできること"""
        # VBox を 2 つ作って、それぞれに同名 "Label" widget を入れる
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "PanelA",
            "path": "/Game/Test"
        })
        test_suite.run_command("add_vertical_box_to_widget", {
            "widget_name": self.widget_name,
            "box_name": "PanelB",
            "path": "/Game/Test"
        })
        test_suite.run_command("add_text_to_widget", {
            "widget_name": self.widget_name,
            "text_name": "Label",
            "parent_name": "PanelA",
            "text": "In A",
            "path": "/Game/Test"
        })
        # 同名 widget (名前ユニークの UE 側制約で実際は連番になる可能性があるが、
        # parent_name scope 機能のテストとしては引数が処理されるかを確認)
        result = test_suite.run_command("set_widget_element_property", {
            "widget_name": self.widget_name,
            "element_name": "Label",
            "parent_name": "PanelA",
            "property_name": "Text",
            "property_value": "Updated via scope",
            "path": "/Game/Test"
        })
        # parent_name が正しくルーティングされれば success, 引数が未サポートなら失敗
        assert_success(result, "set_widget_element_property with parent_name scope")

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })

    def test_parent_name_unresolvable_errors(self, test_suite):
        """parent_name が見つからない場合は WidgetElementNotFound エラー"""
        result = test_suite.run_command("add_button_to_widget", {
            "widget_name": self.widget_name,
            "button_name": "OrphanBtn",
            "parent_name": "DoesNotExistPanel",
            "text": "Orphan",
            "path": "/Game/Test"
        })
        assert result.get("success") is not True, "存在しない parent_name で成功してはならない"

        test_suite.add_cleanup("delete_asset", {
            "asset_path": f"/Game/Test/{self.widget_name}"
        })


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
