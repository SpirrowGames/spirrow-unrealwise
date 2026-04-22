# 更新履歴 (Changelog)

このファイルはspirrow-unrealwiseの詳細な更新履歴をアーカイブしています。

---

## 2026-04-22: Layout Polish — VBox/HBox Slot + get_widget_elements dedupe + IPC UTF-8 修正 (v0.9.9)

**概要**: バックログ残タスク (FR-2 / FR-3 / BUG-3 / BUG-4) をまとめて解消。

### FR-2: set_widget_slot_property の slot 型ディスパッチ

`set_widget_slot_property` が `UCanvasPanelSlot` 専用だったため、VBox/HBox/Overlay/Border の子に対しては slot 設定が全くできなかった。v0.9.9 で slot 型検出 → 型別適用に変更:

- **UVerticalBoxSlot / UHorizontalBoxSlot**: `padding` [L,T,R,B] (FMargin) / `horizontal_alignment` / `vertical_alignment` / `size_rule` ("Auto"/"Fill") / `size_value` (Fill 重み)
- **UOverlaySlot / UBorderSlot**: `padding` / `horizontal_alignment` / `vertical_alignment`
- **UWidgetSwitcherSlot**: レイアウトフィールドなし (dispatch のみ)
- **UCanvasPanelSlot**: 既存の全機能 (v0.9.6/7 と互換) — position/size/anchor/anchor_min/anchor_max/offset_*/alignment/z_order/auto_size

共通ヘルパ:
- `ParseHAlign(str)` / `ParseVAlign(str)` — "Left/Center/Right/Fill" → `EHorizontalAlignment`
- `TryParseFMarginFromParams(params, key, out)` — `[L,T,R,B]` 配列 → FMargin
- `ApplyBoxLikeSlotFields<SlotT>(slot, params)` — Padding/H-V Align を SFINAE-free テンプレで共通適用
- `ApplyBoxChildSize<SlotT>(slot, params)` — FSlateChildSize (ESlateSizeRule: Automatic/Fill + Value) を VBox/HBox 限定で適用

Response に `slot_type` フィールド追加 (`"VerticalBoxSlot"` / `"HorizontalBoxSlot"` / `"OverlaySlot"` / `"BorderSlot"` / `"WidgetSwitcherSlot"` / `"CanvasPanelSlot"`)。

未サポート slot 型は `NotSupported` エラーに (slot_type を details に含める) — 旧 `CanvasPanelNotFound` より意図が明確。

### FR-3: get_widget_elements dedupe + duplicate_names

`WidgetTree->GetAllWidgets` が内部配列で dedupe しているはずだが、BUG-1 クラスの corrupt tree state では同一 pointer が複数回現れうる。v0.9.9 で明示的に `TSet<UWidget*>` で pointer dedupe。また同名 widget (異なる pointer) を `TMap<FName, int32>` で検出し、Response に `duplicate_names` 配列として露出。caller は重複 parent 問題や意図的な同名設計を検出可能に。

### BUG-3 / BUG-4: IPC UTF-8 境界 + timeout 修正 (3 箇所の根本原因)

**再現条件**: 日本語テキスト入り `add_button_to_widget` の並列送信で "utf-8 codec can't decode byte 0xe3 in position 153" エラー、または "Timeout receiving Unreal response"。

**調査結果と修正**:

1. **UE 送信側 (最も critical)** — `MCPServerRunnable.cpp` の `Run()` ループと `ProcessMessage`:
   ```cpp
   // BEFORE (buggy)
   ClientSocket->Send((uint8*)TCHAR_TO_UTF8(*Response), Response.Len(), BytesSent);
   //                                                   ^^^^^^^^^^^^^^
   // Response.Len() は TCHAR 文字数。日本語や emoji 入りの Response では
   // UTF-8 バイト数が TCHAR 数の 2-3 倍になるため、末尾が切れて送信される。
   // Python 側で partial UTF-8 を受信 → json.loads 失敗 → timeout

   // AFTER (fixed)
   FTCHARToUTF8 ResponseUtf8(*Response);
   ClientSocket->Send(reinterpret_cast<const uint8*>(ResponseUtf8.Get()),
                      ResponseUtf8.Length(), BytesSent);
   //                 ^^^^^^^^^^^^^^^^^^^^^^^^
   //                 正確な UTF-8 バイト数
   ```

2. **UE 受信側** — Buffer 8192 → 65536 (socket `SO_RCVBUF` 設定と揃える)。加えて null-terminator 依存を排除:
   ```cpp
   // BEFORE
   uint8 Buffer[8192];
   Buffer[BytesRead] = '\0';  // Recv が full で埋めると OOB 書込み!
   FString ReceivedText = UTF8_TO_TCHAR(Buffer);  // 暗黙 null 終端依存

   // AFTER
   constexpr int32 RecvBufferCapacity = 65536;
   uint8 Buffer[RecvBufferCapacity + 1];  // +1 は null 終端の headroom
   Buffer[BytesRead] = '\0';  // 今は常に inside bounds
   FString ReceivedText = FString(FUTF8ToTCHAR(
       reinterpret_cast<const ANSICHAR*>(Buffer), BytesRead));
   //                                   ^^^^^^^^^^^^^^^^^^^^ 明示バイト長
   ```

3. **Python 受信側** — `unreal_mcp_server.py::receive_full_response`:
   ```python
   # BEFORE (buggy)
   data = b''.join(chunks)
   decoded_data = data.decode('utf-8')   # partial UTF-8 で UnicodeDecodeError
   try:
       json.loads(decoded_data)
       return data
   except json.JSONDecodeError:
       continue
   # 問題: decode() が先に raise すると外側 except で timeout 扱い

   # AFTER (fixed)
   data = b''.join(chunks)
   try:
       decoded_data = data.decode('utf-8')
   except UnicodeDecodeError:
       # マルチバイト境界で切れた → 次の recv を待つ
       continue
   try:
       json.loads(decoded_data)
       return data
   except json.JSONDecodeError:
       continue
   ```

これら 3 箇所の複合的な不整合が BUG-3/4 を生んでいた。

### 変更ファイル

**C++** (2 files):
- `SpirrowBridgeUMGLayoutCommands.cpp` — FR-2 slot dispatch + helper templates + FR-3 dedupe/duplicate_names + slot header includes
- `MCPServerRunnable.cpp` — BUG-3 UE 送受信 UTF-8 修正

**Python** (2 files):
- `command_schemas.py` — set_widget_slot_property schema 拡張 (padding / h-v align / size_rule / size_value)
- `unreal_mcp_server.py` — BUG-3 受信側 partial UTF-8 許容
- `test_umg_widgets.py` — `TestUMGV099LayoutPolish` 追加 (5 テスト)

**ドキュメント** (3 files):
- `FEATURE_STATUS.md` / `Docs/DEV_CHEATSHEET.md` / `templates/end-user/SPIRROW_CHEATSHEET.md` — version bump + FR-2 usage example

**コマンド数**: 変更なし (**158**)。既存コマンドの機能拡張と IPC 層のバグ修正のみ。

---

## 2026-04-22: JSON Property Value — set_widget_element_property 型拡張 (v0.9.8)

**概要**: BUG-6 修正。`set_widget_element_property.property_value` が string のみ受理だった制限を撤廃し、UE struct (FLinearColor / FMargin / FVector 等) を JSON array で、primitive を JSON number/bool で直接渡せるようにする。色・padding・スタイル系プロパティが MCP 経由で初めて扱えるようになる。

### BUG-6 症状

- help は `type: any` と記載
- 実装は string 固定で list/dict を送ると `Parameter 'property_value' must be a string` エラー
- 結果として UE 構造体 (FLinearColor, FMargin, FVector 等) を MCP 経由で設定できず、色・padding・スタイル系 UPROPERTY が全て手詰まりだった

### 修正

**SpirrowBridgeUMGLayoutCommands.cpp — `HandleSetWidgetElementProperty`**: `property_value` を `TSharedPtr<FJsonValue>` として取得、文字列化試行 + 3 分岐:

- **nested path** (`"Foo.Bar"`): `ImportText_Direct` が string 必須なので非 string は `InvalidParamType` エラー返却
- **string value**: 既存の全 widget-specific alias (`Visibility` / `Text` / `ColorAndOpacity` string / `Justification` / `Percent`) + `ImportText_Direct` fallback を**完全後方互換で保持**
- **non-string value** (array / object / number / bool): 新パス — `ColorAndOpacity` + array は明示的な FLinearColor 変換、それ以外は `FSpirrowBridgeCommonUtils::SetObjectProperty` にデリゲート (既存の FLinearColor / FMargin / FVector / FVector2D / FRotator / FColor / FTransform struct ハンドリング再利用、primitive 自動変換、TSubclassOf 対応)

### 使用例

```python
# v0.9.7 まで (string 強制) → v0.9.8 (直接)
"property_value": "1"            → "property_value": 1
"property_value": "[1,0.5,0.2,1]"   → "property_value": [1.0, 0.5, 0.2, 1.0]

# 色の設定 (v0.9.7 以前は不可能)
set_widget_element_property(element_name="HealthBar", property_name="ColorAndOpacity",
    property_value=[1.0, 0.2, 0.2, 1.0])

# Padding の設定 (v0.9.7 以前は不可能)
set_widget_element_property(element_name="BgFrame", property_name="Padding",
    property_value=[16, 8, 16, 8])  # FMargin [L, T, R, B]
```

### 変更ファイル

- `SpirrowBridgeUMGLayoutCommands.cpp` — `HandleSetWidgetElementProperty` の validate/dispatch 再構築
- `command_schemas.py` — `set_widget_element_property.property_value` の desc に受理型リストと例を追加
- `test_umg_widgets.py` — `TestUMGV098PropertyValueTypes` 追加 (4 テスト)
- `FEATURE_STATUS.md` / `Docs/DEV_CHEATSHEET.md` / `templates/end-user/SPIRROW_CHEATSHEET.md` — version bump

**コマンド数**: 変更なし (**158**)。既存コマンドの入力型拡張のみ。

---

## 2026-04-22: Reparent Safety + parent_name on all leaf adds (v0.9.7)

**概要**: WBP_MainMenu 実装検証で判明した致命的バグ 3 件 + 重要バグ 1 件 + 機能要望 2 件をまとめて解決。`ue-investigator` で UE 5.7 の UMG 内部を 5 ラウンド解析した結果に基づく。

### BUG-1 修正: `reparent_widget_element` の double-parent corruption

**症状**: `reparent_widget_element(Button, new_parent=VBox)` 実行後、旧親 (CanvasPanel_0) の `children` 配列に widget 参照が残存。UE Hierarchy で実際に 2 箇所に表示され、BindWidget 付き widget は UE が削除を拒否するため MCP からの復旧が不可能になる致命的バグ。

**根本原因** (ue-investigator Q1/Q2/Q5 で確定):
- `UPanelWidget::RemoveChildAt` 自体は完全に動作 (PanelWidget.cpp:93-125 — Slots 配列除去 + Slot/Content/Parent 参照全クリア + OnSlotRemoved で Slate 側も同期)
- 旧実装は `Modify()` を関連 UObject (WidgetTree / OldParent / NewParent / Element) で呼んでいなかった → transaction system が pre-state を記録できず、serialization に反映されない
- 旧実装は `FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified` を呼んでいなかった → UMG Designer canonical pattern (SDesignerView.cpp:3237-3238) と乖離
- `FKismetEditorUtilities::CompileBlueprint` は **WidgetTree 整合性チェックをしない** (`UWidgetTree::RebuildTree` を呼ばない)。壊れた状態がそのまま焼き付く

**修正** (SpirrowBridgeUMGLayoutCommands.cpp:1178-1255 前後):
1. 全関連 UObject で `Modify()` 呼出 (WidgetBP / WidgetTree / Element / OldParent / NewParent)
2. `RemoveChild` → `AddChild`
3. **Defensive post-check**: `OldParent->GetChildIndex(Element) != INDEX_NONE` なら integrity エラー、`Element->GetParent() != NewParent` でも integrity エラー
4. `FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP)` 追加
5. `OldParent == NewParent` の no-op ケースを早期 return で検出
6. 無言 fallback を廃止 — 整合性違反は `ESpirrowErrorCode::Unknown` で返却

### BUG-5 修正: element_name ambiguity in lookup commands

**症状**: 同名 widget が複数存在すると `set_widget_element_property(element_name="Label")` などがどの widget を操作するか不定。特に BUG-1 で生じた重複状態では「VerticalBoxSlot 側が先にヒット → Canvas 側にアクセスできない」という挙動で詰む。

**根本原因** (ue-investigator Q3 で確定): `UWidgetTree::FindWidget(FName)` は早期 `break` せず **最後に一致した widget を返す** (`WidgetTree.cpp:32-44`)。走査順依存で決定論的だが、caller 側からは制御不能。

**修正**:
- 新ヘルパー `FindWidgetInPanelRecursive(UPanelWidget*, FName)` を SpirrowBridgeUMGLayoutCommands.cpp 先頭に追加 — 指定パネル配下を走査して**最初に一致した widget**を返す (決定論的)
- 新ヘルパー `ResolveElementScoped(WidgetTree, Params, ElementName, OutError)` — Params に `parent_name` があればそのパネル配下を走査、なければ既存の WidgetTree->FindWidget (last-match) に fallback
- 適用先: `set_widget_slot_property` / `set_widget_element_property` / `get_widget_element_property` / `reparent_widget_element` / `remove_widget_element` (5 コマンド)

### BUG-2 修正: `add_text_block_to_widget` 廃止

**症状**: help 出力は `widget_name` を要求するが実装は `blueprint_name` 必須 + `/Game/Widgets/` ハードコードパスを強制する壊れた legacy コマンド。

**修正**: 完全削除。`add_text_to_widget` への移行を推奨 (feature 的に完全な上位互換)。削除対象は:
- Python `umg_meta.py` WIDGET_COMMANDS dict
- Python `command_schemas.py` スキーマ
- C++ `SpirrowBridgeUMGWidgetBasicCommands` header + implementation
- C++ `SpirrowBridge.cpp` routing
- end-user docs (SPIRROW_CHEATSHEET.md / ue-hud-bootstrap/SKILL.md) を `add_text_to_widget` に更新

### FR-1: 全 `add_*_to_widget` に `parent_name` 追加

**動機**: BUG-1 の発生機会を根本的に減らすため、「Canvas に作成 → reparent」パターンを排除し「任意パネルに直接追加」できる API に。

**実装**:
- 共通ヘルパー `FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, OutError)` 新設
  - `parent_name` が Params にあれば UPanelWidget として解決 (見つからない/パネルでない場合はエラー返却)
  - なければ root CanvasPanel (欠損時は自動構築、legacy 動作維持)
- 9 コマンドの handler を `RootCanvas->AddChildToCanvas(X)` → `Parent->AddChild(X)` + `Cast<UCanvasPanelSlot>` パターンに変換
- 対象: button / text / image / progressbar / slider / checkbox / combobox / editabletext / spinbox / scrollbox (+ border は v0.9.6 で既対応)
- `UCanvasPanelSlot` 固有プロパティ (anchor / alignment / position / size) は親がキャンバスの場合のみ適用 (VBox/HBox 配下では無視される = UE Designer の挙動と一致)

### FR-4: `bind_widget_to_variable` の stale 参照削除

`umg_widget` docstring と WIDGET_COMMANDS dict に残っていたが C++ handler が存在せず `Unknown command` エラーを返す状態だった。削除。

### 変更ファイル

**C++** (4 files):
- `SpirrowBridgeUMGLayoutCommands.cpp` — reparent canonical pattern + ResolveElementScoped + 5 コマンドの lookup scope 対応
- `SpirrowBridgeUMGWidgetBasicCommands.{h,cpp}` — add_text/image/progressbar を ResolveAddTarget 経由に + HandleAddTextBlockToWidget 削除
- `SpirrowBridgeUMGWidgetInteractiveCommands.cpp` — 7 コマンド (button/slider/checkbox/combobox/editabletext/spinbox/scrollbox) を ResolveAddTarget 経由に
- `SpirrowBridgeUMGWidgetCoreCommands.{h,cpp}` — `ResolveAddTarget` 静的ヘルパー追加
- `SpirrowBridge.cpp` — add_text_block_to_widget routing 削除

**Python** (3 files):
- `umg_meta.py` — add_text_block_to_widget / bind_widget_to_variable を WIDGET_COMMANDS / docstring から削除
- `command_schemas.py` — add_text_block_to_widget schema 削除 / 9 leaf adds + 5 lookup コマンドに parent_name 追加 / reparent brief 更新
- `test_umg_widgets.py` — `TestUMGV097ReparentSafety` クラス追加 (5 テスト: reparent duplicate 検証 / parent_name on add / lookup scope / unresolvable parent)

**ドキュメント** (3 files):
- `FEATURE_STATUS.md` — v0.9.7 section + コマンド数 159→158
- `templates/end-user/SPIRROW_CHEATSHEET.md` — add_text_block → add_text 置換
- `templates/end-user/skills/ue-hud-bootstrap/SKILL.md` — 同上

---

## 2026-04-21: UMG Extensions (v0.9.6)

**概要**: UMG の WBP レイアウト表現力を拡張。WidgetSwitcher / Border コマンドを追加、CanvasPanelSlot で任意の FAnchors と LTRB オフセットを指定可能に、`create_umg_widget_blueprint` の parent_class 解決を汎用化。

### 新コマンド (2)

#### `add_widget_switcher_to_widget` (umg_layout)

UWidgetSwitcher をウィジェットツリーに追加。ページ切替 UI の中核。

**パラメータ**:
- `widget_name` (required): Widget Blueprint 名
- `switcher_name` (required): Switcher 要素名
- `parent_name` (optional): 親パネル名 (省略時 root canvas)
- `active_widget_index` (int, default 0): 初期 ActiveWidgetIndex
- `anchor` / `alignment` / `position` / `size`: CanvasPanelSlot プロパティ
- `path` (default `/Game/UI`): Content path

**ランタイム切替**: `umg_widget(command="set_widget_element_property", params={"element_name": "Switcher1", "property_name": "ActiveWidgetIndex", "property_value": "1"})` で切替可能 (リフレクション fallback 経由で int32 UPROPERTY 全般に適用される)。

#### `add_border_to_widget` (umg_widget)

UBorder (単一子コンテナ + 背景ブラシ) を追加。HUD やメニューの半透明背景レイヤ、パディング付きパネルに有用。

**パラメータ**:
- `widget_name` / `border_name` (required)
- `parent_name` (optional): 任意パネルへのネスト
- `brush_color` [r,g,b,a]: 背景ブラシ色
- `content_color_and_opacity` [r,g,b,a]: 子要素の tint
- `padding` [L,T,R,B]: FMargin (コンテンツの内側余白)
- `horizontal_alignment` / `vertical_alignment`: Left/Center/Right/Fill, Top/Center/Bottom/Fill
- CanvasPanelSlot プロパティ: `anchor` / `alignment` / `position` / `size`

UBorder は `UPanelWidget` (子 1 つ) のため、`reparent_widget_element` で既存要素を中に入れるか、`parent_name=BorderName` で直接子コマンドを追加できる。

### API 拡張 (2)

#### `set_widget_slot_property`: 任意の FAnchors + LTRB オフセット

従来は `anchor` 文字列プリセット (TopLeft/Center/BottomRight 等 9 種) のみ。新規:
- `anchor_min: [x, y]` / `anchor_max: [x, y]` — 0-1 UV 空間で任意の FAnchors (例: 全画面ストレッチ `[0,0]`-`[1,1]`)
- `offset_left / offset_top / offset_right / offset_bottom` — 個別の FMargin LTRB 差分更新

**優先順位**: `anchor` プリセットが指定されると `anchor_min/max` は無視される (後方互換)。`anchor_min/max` は片方だけの更新も可能 (もう一方は `GetAnchors()` の既存値を保持)。offsets は `GetOffsets()` 基点の差分更新。

**用途例**:
```
# 全画面ストレッチ + 16px inset
set_widget_slot_property(element_name="Panel",
    anchor_min=[0,0], anchor_max=[1,1],
    offset_left=16, offset_top=16, offset_right=16, offset_bottom=16)
```

#### `create_umg_widget_blueprint.parent_class` 汎用化

**従来の問題**:
- ハードコードで `"UserWidget"` だけ特別扱い
- `FindFirstObject` による不安全な検索 (複数マッチ時に不定)
- `/Script/UMG.<Name>` にしか対応せず C++ 派生クラスや BP 派生クラスを親にできない
- 解決失敗時に無音で `UUserWidget` にフォールバック (caller は気づかない)

**v0.9.6 修正**: `FSpirrowBridgeCommonUtils::SetObjectProperty` の FClassProperty 解決ロジックに揃えた順序で解決:
1. `"UserWidget"` shortcut → `UUserWidget::StaticClass()`
2. `LoadClass<UObject>(nullptr, *Path)` — `/Script/Module.Class` や `/Game/Path.Asset_C` 直接
3. `UEditorAssetLibrary::LoadAsset(Path)` → `UBlueprint::GeneratedClass` 抽出 (`/Game/Path.Asset` 形式)
4. サフィックス補完 (`/Game/UI/BP_Foo` → `/Game/UI/BP_Foo.BP_Foo`)
5. `/Script/UMG.<Name>` 後方互換 fallback
6. 失敗 → **ハードエラー** `ESpirrowErrorCode::ClassNotFound` (1211)

さらに解決後に `IsChildOf(UUserWidget::StaticClass())` を検証。失敗 → `InvalidParamValue` (1005) をハードエラー返却。

成功レスポンスの `parent_class` フィールドは `GetPathName()` (フルパス) を返すようになった。caller は実際に採用された解決済みクラスパスを確認できる。

### 変更ファイル

**C++**:
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeUMGLayoutCommands.h` — `HandleAddWidgetSwitcherToWidget` 宣言追加
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeUMGWidgetBasicCommands.h` — `HandleAddBorderToWidget` 宣言追加
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGLayoutCommands.cpp` — WidgetSwitcher 実装 + set_widget_slot_property 拡張 (anchor_min/max + LTRB offsets)
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGWidgetBasicCommands.cpp` — Border 実装
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGWidgetCoreCommands.cpp` — parent_class 汎用化
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/SpirrowBridge.cpp` — routing 2 コマンド追加

**Python**:
- `Python/tools/umg_meta.py` — LAYOUT_COMMANDS / WIDGET_COMMANDS に追加
- `Python/tools/command_schemas.py` — 4 コマンドの schema 更新 (2 新規 + 2 拡張)
- `Python/tests/test_umg_widgets.py` — `TestUMGV096Extensions` クラス追加 (7 テスト: switcher / ActiveWidgetIndex / border / border nested / 明示的 anchors / parent_class /Script path / parent_class 不正形式)

**ドキュメント**:
- `FEATURE_STATUS.md` — バージョン v0.9.6、コマンド数 157 → 159
- `Docs/DEV_CHEATSHEET.md` — v0.9.6 新機能を追記
- `templates/end-user/SPIRROW_CHEATSHEET.md` — end-user 向けに新 UI 作成例を追加

---

## 2026-01-07: Feature - Volume Actor Support in spawn_actor (v0.8.2)

**概要**: `spawn_actor`コマンドで8種類のVolumeアクターを生成可能に

**問題**:
- Volumeアクターは単純な`SpawnActor`だけではBrush Geometryが生成されない
- 結果としてDetailsパネルに「Brush Settings」が表示されず、機能しない

**解決策**:
- `UActorFactory::CreateBrushForVolumeActor`を使用して正しいBrush Geometryを生成
- `UCubeBuilder`でサイズを設定してからVolumeに適用

**対応したVolumeタイプ** (8種類):
- `NavMeshBoundsVolume` - NavMeshのビルド範囲
- `TriggerVolume` - トリガーイベント用
- `BlockingVolume` - コリジョンブロック用
- `KillZVolume` - 落下死亡用
- `PhysicsVolume` - 物理設定用
- `PostProcessVolume` - ポストプロセス用
- `AudioVolume` - オーディオ設定用
- `LightmassImportanceVolume` - ライトマス用

**新パラメータ**:
- `brush_size`: Volumeのサイズを明示的に指定 `[X, Y, Z]`
- 未指定時は `200 * scale` がデフォルト

**使用例**:
```python
# NavMeshボリュームを作成
spawn_actor(
    name="NavMesh_Arena",
    type="NavMeshBoundsVolume",
    location=[0, 0, 100],
    brush_size=[5000, 5000, 1000]
)

# トリガーボリュームを作成
spawn_actor(
    name="TrapZone",
    type="TriggerVolume",
    location=[500, 0, 50],
    brush_size=[300, 300, 200]
)
```

**変更ファイル**:
- `SpirrowBridgeEditorCommands.cpp` - Volumeアクター対応追加
- `SpirrowBridgeCommonUtils.h` - `InvalidActorType`エラーコード追加
- `editor_tools.py` - docstring更新、brush_size/scaleパラメータ追加

**追加include**:
- `NavMesh/NavMeshBoundsVolume.h`
- `GameFramework/KillZVolume.h`
- `Engine/TriggerVolume.h`
- `Engine/BlockingVolume.h`
- `GameFramework/PhysicsVolume.h`
- `Engine/PostProcessVolume.h`
- `Sound/AudioVolume.h`
- `Lightmass/LightmassImportanceVolume.h`
- `GameFramework/Volume.h`
- `ActorFactories/ActorFactory.h`
- `Builders/CubeBuilder.h`

---

## 2026-01-07: BugFix - Blackboard BaseClass Not Set (v0.8.1)

**概要**: `add_blackboard_key`の`base_class`パラメータが反映されない問題を修正

**問題**:
- `base_class="Actor"`を指定しても、Blackboardキーの BaseClass が `Object` のままだった
- 結果として MoveTo タスクで Object型キー（TargetActor等）が選択肢に表示されなかった

**原因**:
- `FindObject<UClass>(nullptr, *BaseClass)`で`"Actor"`という短い名前では検索できなかった
- 正しいパスは`/Script/Engine.Actor`

**修正内容**:
複数の検索方法を試行するように改善:
```cpp
// Method 1: 直接検索（フルパス用）
FoundClass = FindObject<UClass>(nullptr, *BaseClass);

// Method 2: /Script/Engine プレフィックス
FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *BaseClass);
FoundClass = FindObject<UClass>(nullptr, *EnginePath);

// Method 3: /Script/CoreUObject プレフィックス
FString CorePath = FString::Printf(TEXT("/Script/CoreUObject.%s"), *BaseClass);
FoundClass = FindObject<UClass>(nullptr, *CorePath);

// Method 4: StaticLoadClass フォールバック
FoundClass = StaticLoadClass(UObject::StaticClass(), nullptr, *ClassPath);
```

**変更ファイル**:
- `SpirrowBridgeAICommands_Blackboard.cpp` - BaseClass検索ロジック改善

**検証**:
- `add_blackboard_key(base_class="Actor")` → KeyType=Object, BaseClass=Actor ✅
- MoveTo タスクで TargetActor キーが選択可能に ✅
- `set_bt_node_property(property_name="BlackboardKey")` で設定反映 ✅

---

## 2026-01-07: Feature - Struct Property Support in SetObjectProperty

**概要**: `SetObjectProperty`に構造体プロパティ対応を追加

**対応構造体**:
- `FBlackboardKeySelector` - BTノードのBlackboardKey設定（文字列 or オブジェクト入力）
- `FVector` / `FVector2D` / `FRotator` - 座標・回転（配列 or オブジェクト入力）
- `FLinearColor` / `FColor` - 色（配列 or オブジェクト入力）
- `FAIDataProviderFloatValue` / `FAIDataProviderIntValue` / `FAIDataProviderBoolValue` - EQS用
- `FTransform` - トランスフォーム（オブジェクト入力）
- 汎用構造体 - リフレクションによるフィールド単位の設定

**追加プロパティタイプ**:
- `FNameProperty` - FName型
- `FDoubleProperty` - double型（UE5）

**使用例**:
```python
# BlackboardKey設定（文字列）
set_bt_node_property(node_id="...", property_name="BlackboardKey", property_value="TargetLocation")

# BlackboardKey設定（オブジェクト）
set_bt_node_property(node_id="...", property_name="BlackboardKey", property_value={"SelectedKeyName": "TargetActor"})

# Vector設定（配列）
set_actor_property(name="...", property_name="Location", property_value=[100, 200, 300])
```

**変更ファイル**:
- `SpirrowBridgeCommonUtils.h` - `SetStructPropertyValue`, `SetStructFieldValue` 宣言追加
- `SpirrowBridgeCommonUtils.cpp` - 構造体対応実装（約480行追加）

**既知の制限解消**:
- `set_eqs_test_property`でStruct型（FAIDataProviderFloatValue）が対応可能に

---

## 2026-01-07: BugFix - connect_bt_nodes Child Not Found

**概要**: `connect_bt_nodes`でRoot未接続ノードを検索できない問題を修正

**問題**:
- `add_bt_composite_node`で作成したノードを`connect_bt_nodes`でRootに接続しようとすると`Child node not found`エラー
- 原因: `FindBTNodeById`がRootNode以下のツリーのみを検索していた

**修正内容**:
- `PendingBTNodes`キャッシュを導入（作成済み・未接続ノードを一時保持）
- `HandleAddBTCompositeNode` / `HandleAddBTTaskNode`: ノード作成後にキャッシュに登録
- `FindBTNodeById`: まずキャッシュを検索、なければツリーを検索
- `HandleConnectBTNodes`: 接続成功後にキャッシュから削除

**変更ファイル (5ファイル)**:
- `SpirrowBridgeAICommands.h` - PendingBTNodes宣言追加
- `SpirrowBridgeAICommands.cpp` - static変数定義追加
- `AICommands_BTNodeCreation.cpp` - キャッシュ登録処理
- `AICommands_BTNodeHelpers.cpp` - キャッシュ検索処理
- `AICommands_BTNodeOperations.cpp` - キャッシュ削除処理

---

## 2026-01-06: Phase H - AIPerception & EQS

**概要**: AI感知システムとEnvironment Query System操作11ツール追加

**AIPerception (6ツール)**:
- `add_ai_perception_component` - AIControllerにPerceptionComponent追加
- `configure_sight_sense` - 視覚設定（距離/角度/アフィリエーション）
- `configure_hearing_sense` - 聴覚設定
- `configure_damage_sense` - ダメージ感知設定
- `set_perception_dominant_sense` - 優先センス設定
- `add_perception_stimuli_source` - 被検知側コンポーネント追加

**EQS (5ツール)**:
- `create_eqs_query` - EQS Query Asset作成
- `add_eqs_generator` - Generator追加（SimpleGrid/Donut/OnCircle/ActorsOfClass等）
- `add_eqs_test` - Test追加（Distance/Trace/Dot等）+ scoring_factor対応
- `set_eqs_test_property` - Testプロパティ設定（基本型のみ）
- `list_eqs_assets` - EQSアセット一覧

**技術詳細**:
- C++: AIPerceptionCommands (18KB) + EQSCommands (16KB)
- Python: perception_tools.py + eqs_tools.py
- テスト: test_phase_h.py (13テスト)

**既知の制限**:
- `set_eqs_test_property`でStruct型（FAIDataProviderFloatValue）は未対応
- → `add_eqs_test`の`scoring_factor`パラメータで代替可能

---

## 2026-01-06: Phase G - BehaviorTree Node Operations

**概要**: BTノードグラフをプログラマティックに構築する8ツール追加

**新規ツール**:
- `add_bt_composite_node` - Selector/Sequence/SimpleParallel追加
- `add_bt_task_node` - MoveTo/Wait等9タスク + カスタムBP対応
- `add_bt_decorator_node` - Blackboard/Cooldown等9デコレータ
- `add_bt_service_node` - DefaultFocus/RunEQS等サービス
- `connect_bt_nodes` - 親子接続、Root設定
- `set_bt_node_property` - リフレクション経由プロパティ設定
- `delete_bt_node` - ノード削除
- `list_bt_node_types` - 利用可能ノードタイプ一覧

**技術詳細**:
- C++ AICommands 6ファイル分割構成（1,805行）
- UE 5.6+ API互換性対応（Decorator格納方式、TryGetField変更）
- Python `tools/` フラット構造に統一

---

## 2026-01-05: Phase F - AI (BehaviorTree / Blackboard)

**概要**: AI開発に必須のBehaviorTree/Blackboard操作8ツール

**新規ツール**:
- `create_blackboard` - Blackboard Data Asset作成
- `add_blackboard_key` - キー追加（10タイプ対応）
- `remove_blackboard_key` / `list_blackboard_keys`
- `create_behavior_tree` - BehaviorTree Asset作成
- `set_behavior_tree_blackboard` / `get_behavior_tree_structure`
- `list_ai_assets` - AI関連アセット一覧

**技術詳細**:
- C++: SpirrowBridgeAICommands (674行)
- Python: ai_tools.py (455行)
- テスト: test_ai_tools.py (16テスト)

---

## 2026-01-03: Phase E - エラーハンドリング統一

**概要**: 全18 CommandsファイルにESpirrowErrorCode使用を統一

**追加エラーコード (12個)**:
- General: UnknownCommand, InvalidParameter, OperationFailed, SystemError
- Blueprint: GraphNotFound, NodeNotFound, ClassNotFound, InvalidOperation
- Actor: ComponentCreationFailed
- Config: ConfigKeyNotFound, FileWriteFailed, FileReadFailed

---

## 2026-01-03: Phase 0.6.6 - UMGWidgetCommands分割

**概要**: SpirrowBridgeUMGWidgetCommands.cpp (64KB) を3ファイルに分割

**分割構成**:
- UMGWidgetCoreCommands.cpp (7KB) - 3関数
- UMGWidgetBasicCommands.cpp (17KB) - 4関数
- UMGWidgetInteractiveCommands.cpp (30KB) - 7関数

**削減効果**: 最大64KB → 30KB (53%削減)

---

## 2026-01-03: Phase 0.6.5 - BlueprintCommands分割

**概要**: Blueprint系2ファイル（計163KB）を6ファイルに分割

**Blueprint系分割**:
- BlueprintCoreCommands.cpp (23KB)
- BlueprintComponentCommands.cpp (26KB)
- BlueprintPropertyCommands.cpp (21KB)

**BlueprintNode系分割**:
- BlueprintNodeCoreCommands.cpp (24KB)
- BlueprintNodeVariableCommands.cpp (14KB)
- BlueprintNodeControlFlowCommands.cpp (21KB)

**削減効果**: 最大95KB → 26KB (73%削減)

---

## それ以前の履歴

### Phase 0.6.0 - GAS対応
- GameplayTags / GameplayEffect / GameplayAbility ツール

### Phase 0.5.0 - UMG Widget
- Widget Blueprint操作（29ツール）
- アニメーション、変数・関数バインディング

### Phase 0.4.0 - Enhanced Input
- Input Action / Mapping Context
- Config操作ツール

### Phase 0.3.0 - Blueprint Node
- ノードグラフ操作
- RAG知識ベース統合
