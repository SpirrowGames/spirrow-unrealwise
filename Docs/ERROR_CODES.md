# エラーコード一覧

SpirrowBridge で使用されるエラーコード体系。

---

## General (1000-1099)

| コード | 値 | 説明 |
|--------|-----|------|
| `Success` | 0 | 成功 |
| `UnknownError` | 1000 | 不明なエラー |
| `UnknownCommand` | 1001 | 不明なコマンド |
| `InvalidParams` | 1002 | 無効なパラメータ |
| `MissingRequiredParam` | 1003 | 必須パラメータ不足 |
| `InvalidParamType` | 1004 | パラメータ型不正 |
| `InvalidParamValue` | 1005 | パラメータ値不正 |
| `InvalidParameter` | 1006 | 無効なパラメータ |
| `OperationFailed` | 1007 | 操作失敗 |
| `SystemError` | 1008 | システムエラー |

## Asset (1100-1199)

| コード | 値 | 説明 |
|--------|-----|------|
| `AssetNotFound` | 1100 | アセットが見つからない |
| `AssetLoadFailed` | 1101 | アセット読み込み失敗 |
| `AssetAlreadyExists` | 1102 | アセットが既に存在 |
| `AssetCreationFailed` | 1103 | アセット作成失敗 |
| `AssetDeleteFailed` | 1104 | アセット削除失敗 |
| `InvalidAssetPath` | 1105 | 無効なアセットパス |

## Blueprint (1200-1299)

| コード | 値 | 説明 |
|--------|-----|------|
| `BlueprintNotFound` | 1200 | Blueprintが見つからない |
| `BlueprintCompileFailed` | 1201 | コンパイル失敗 |
| `BlueprintInvalidClass` | 1202 | 無効なクラス |
| `EventGraphNotFound` | 1203 | EventGraphが見つからない |
| `NodeCreationFailed` | 1204 | ノード作成失敗 |
| `NodeConnectionFailed` | 1205 | ノード接続失敗 |
| `PinNotFound` | 1206 | ピンが見つからない |
| `VariableNotFound` | 1207 | 変数が見つからない |
| `FunctionNotFound` | 1208 | 関数が見つからない |
| `GraphNotFound` | 1209 | グラフが見つからない |
| `NodeNotFound` | 1210 | ノードが見つからない |
| `ClassNotFound` | 1211 | クラスが見つからない |
| `InvalidOperation` | 1212 | 無効な操作 |

## Widget (1300-1399)

| コード | 値 | 説明 |
|--------|-----|------|
| `WidgetNotFound` | 1300 | Widgetが見つからない |
| `WidgetElementNotFound` | 1301 | 要素が見つからない |
| `WidgetCreationFailed` | 1302 | Widget作成失敗 |
| `WidgetTreeNotFound` | 1303 | Widget Treeが見つからない |
| `CanvasPanelNotFound` | 1304 | CanvasPanelが見つからない |
| `AnimationNotFound` | 1305 | アニメーションが見つからない |

## Actor (1400-1499)

| コード | 値 | 説明 |
|--------|-----|------|
| `ActorNotFound` | 1400 | アクターが見つからない |
| `ActorSpawnFailed` | 1401 | アクター生成失敗 |
| `ComponentNotFound` | 1402 | コンポーネントが見つからない |
| `PropertyNotFound` | 1403 | プロパティが見つからない |
| `PropertySetFailed` | 1404 | プロパティ設定失敗 |
| `ComponentCreationFailed` | 1405 | コンポーネント作成失敗 |

## GAS (1500-1599)

| コード | 値 | 説明 |
|--------|-----|------|
| `GameplayTagInvalid` | 1500 | 無効なGameplayTag |
| `GameplayEffectFailed` | 1501 | GameplayEffect失敗 |
| `GameplayAbilityFailed` | 1502 | GameplayAbility失敗 |

## Config (1600-1699)

| コード | 値 | 説明 |
|--------|-----|------|
| `ConfigKeyNotFound` | 1600 | 設定キーが見つからない |
| `FileWriteFailed` | 1601 | ファイル書き込み失敗 |
| `FileReadFailed` | 1602 | ファイル読み取り失敗 |

## PIE / Runtime / Logs (1700-1799) — v0.10.0

| コード | 値 | 説明 |
|--------|-----|------|
| `PIENotRunning` | 1700 | PIE が起動していない (PlayWorld == nullptr) |
| `PIEAlreadyRunning` | 1701 | PIE が既に起動中 (start_pie 二重呼び出し) |
| `PIEStartFailed` | 1702 | PIE 起動失敗 |
| `PlayerControllerNotFound` | 1703 | 指定 player_index に PlayerController が無い |
| `PawnNotFound` | 1704 | PlayerController に Pawn が possessed されていない |
| `ConsoleCommandFailed` | 1705 | exec_console_command の対象 World が無い等 |
| `HighResScreenshotFailed` | 1706 | HighResShot 失敗 / World 不在 |
| `LogFileNotAccessible` | 1707 | ログファイルが存在しない / 読めない |
| `LogParseError` | 1708 | ログパース失敗 (search_ue_log の regex 不正等) |
| `LiveCodingUnavailable` | 1709 | Live Coding モジュール未読み込み / 無効 / コンパイル中 |
| `EditorViewportNotFound` | 1710 | active な perspective editor viewport が見つからない |
| `InputKeyInvalid` | 1711 | simulate_pie_input の key 名が FKey として invalid |
| `FrameSteppingNotSupported` | 1712 | step_pie_frames セッションが既に進行中 |
| `ShowFlagInvalid` | 1713 | set_showflag の flag 名が engine に拒否された |

---

## 使用例

### C++

```cpp
#include "Utils/SpirrowBridgeCommonUtils.h"

// エラーレスポンス作成
return FSpirrowBridgeCommonUtils::CreateErrorResponse(
    ESpirrowErrorCode::BlueprintNotFound,
    FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName)
);
```

### Python

```python
from tools.error_codes import ErrorCode, parse_error_response

result = await call_tool("create_blueprint", {...})
if not result.get("success"):
    error = parse_error_response(result)
    print(f"Error {error.code}: {error.message}")
```
