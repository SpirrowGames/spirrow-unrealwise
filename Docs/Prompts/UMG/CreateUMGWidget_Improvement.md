# create_umg_widget_blueprint 改善

## 概要

`create_umg_widget_blueprint` コマンドの柔軟性と機能を向上させる改善。

## 変更内容

### 1. パラメータの互換性向上

**Before:**
- `name` のみサポート

**After:**
- `widget_name` を推奨パラメータとして追加
- `name` も後方互換性のためサポート継続

### 2. パス指定の柔軟化

**Before:**
- 固定パス: `/Game/Widgets/`

**After:**
- デフォルトパス: `/Game/UI/` (より一般的な命名規則)
- `path` パラメータで任意のパス指定可能

### 3. 親クラスの動的指定

**Before:**
- 常に `UUserWidget` 固定

**After:**
- `parent_class` パラメータで親クラス指定可能
- カスタムウィジェットクラスの継承をサポート
- 指定されたクラスが見つからない場合は `UUserWidget` にフォールバック

### 4. Blueprint クラス指定の修正

**Before:**
```cpp
UBlueprint::StaticClass()
```

**After:**
```cpp
UWidgetBlueprint::StaticClass()
```

ウィジェット専用のBlueprintクラスを使用することで、UMG固有の機能を正しくサポート。

### 5. パッケージ保存の改善

UE 5.0+ の新しい API を使用してパッケージを保存:

```cpp
FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
FSavePackageArgs SaveArgs;
SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
SaveArgs.Error = GError;
SaveArgs.SaveFlags = SAVE_NoError;
UPackage::SavePackage(Package, WidgetBlueprint, *PackageFileName, SaveArgs);
```

### 6. レスポンスの改善

**追加された情報:**
- `success`: bool フラグ
- `parent_class`: 使用された親クラス名
- より詳細なエラーメッセージ（既存アセットの場合はフルパスを表示）

## 使用例

### 基本的な使用

```python
create_umg_widget_blueprint(
    widget_name="WBP_MainMenu"
)
# → /Game/UI/WBP_MainMenu に作成
```

### カスタムパス指定

```python
create_umg_widget_blueprint(
    widget_name="WBP_HUD",
    path="/Game/TrapxTrap/UI/HUD"
)
```

### カスタム親クラス指定

```python
create_umg_widget_blueprint(
    widget_name="WBP_CustomDialog",
    parent_class="MyCustomWidget",
    path="/Game/UI/Dialogs"
)
```

## 技術的詳細

### 親クラスの解決順序

1. `parent_class` パラメータが "UserWidget" の場合
   - `UUserWidget::StaticClass()` を直接使用

2. カスタムクラス名の場合
   - `FindFirstObject<UClass>()` で検索
   - 見つからない場合は `/Script/UMG.{ClassName}` のフルパスで検索
   - それでも見つからない場合は `UUserWidget` にフォールバック

### Include 追加

```cpp
#include "Misc/PackageName.h"
```

UE 5.0+ のパッケージ名処理に必要。

## 後方互換性

既存のコードは引き続き動作します:

```python
# 古い形式（依然として動作）
create_umg_widget_blueprint(name="WBP_OldStyle")

# 新しい形式（推奨）
create_umg_widget_blueprint(widget_name="WBP_NewStyle")
```

## 今後の拡張可能性

将来的に追加可能な機能:
- テンプレート指定（事前定義されたレイアウト）
- 初期ウィジェット追加（Canvas Panel、Vertical Box など）
- 親ウィジェットからの継承時のオーバーライド設定
