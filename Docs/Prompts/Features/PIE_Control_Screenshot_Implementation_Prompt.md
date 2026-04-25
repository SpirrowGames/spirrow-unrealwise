# PIE Control + Screenshot + Camera + Logs - 実装指示書

> AI-driven UE 開発を真に自動化するための「PIE 操作 + 画面取得 + 動的検証」基盤を SpirrowBridge に追加する。
> 起源: 2026-04-25 spirrow-voxelworld の SeamOctree LOD 修正 (commit c21b9e7) を PIE で目視確認したかったが、UnrealWise に PIE 起動・スクショ・カメラ操作系のコマンドが無く、人間が手で Play 押す必要があった。これが解決すれば、CLI 検証 → AI 自身が PIE 起動 → カメラ動かす → スクショ撮る → 視覚的 regression check まで完全自動化できる。

## コンテキスト

- **プロジェクト**: spirrow-unrealwise (`C:/Users/owner/Documents/Unreal Projects/spirrow-unrealwise/`)
- **branch**: `feat/v0.9.9-layout-polish` の上から新ブランチ切る想定
- **対象 UE バージョン**: 5.7
- **既存資産**: `HandleTakeScreenshot` は C++ 側で実装済 (`SpirrowBridgeEditorCommands.cpp:786`)、Python 側に登録漏れ
- **動作確認用 target project**: `C:/Users/owner/Documents/Unreal Projects/VoxelWorldHost/` (BakedVoxelWorld actor が存在、LOD 境界を視覚的に検証できる)

## ベストな状態 (実装目標)

**「人間が見ないと分からない PIE 中の挙動 (アニメーション、LOD 切り替わり、UI、物理) を AI が自律的に検証できる」** 状態。

### Phase 1: 必須 (これが揃えば voxel 側 SeamOctree 検証が即できる)

| 機能 | コマンド名 | C++ 実装 | Python expose |
|---|---|---|---|
| Editor viewport screenshot | `take_screenshot` | ✅ 既存 | ❌ 要追加 |
| PIE 起動 | `start_pie` | 新規 | 新規 |
| PIE 停止 | `stop_pie` | 新規 | 新規 |
| PIE 状態取得 | `get_pie_state` | 新規 | 新規 |
| PIE viewport screenshot | `take_pie_screenshot` | 新規 (active viewport reuse) | 新規 |
| PIE camera (player) 取得 | `get_pie_camera` | 新規 | 新規 |
| PIE camera (player) 設定 | `set_pie_camera` | 新規 | 新規 |
| Console command 実行 | `exec_console_command` | 新規 | 新規 |
| Editor viewport camera 取得/設定 | `get_editor_camera` / `set_editor_camera` | 新規 | 新規 |

### Phase 2: 高機能化 (Phase 1 完了後)

| 機能 | コマンド名 | 目的 |
|---|---|---|
| PIE pause/resume | `pause_pie` / `resume_pie` | 単一フレームで状態固定して観察 |
| Frame stepping | `step_pie_frames` | N フレーム進めて diff 取得 |
| HighRes screenshot | `take_high_res_screenshot` | 任意倍率 (HighResShot console) |
| PIE world actor 取得 | `get_pie_actors` / `find_pie_actors_by_class` | `GEditor->PlayWorld` から runtime actor 列挙 |
| PIE actor properties | `get_pie_actor_properties` | live state 読み取り |
| Debug Cam toggle | `enable_debug_cam` / `disable_debug_cam` | 自由視点観察モード |
| Time dilation | `set_global_time_dilation` | スローモーション再生 |
| Log tail | `tail_ue_log` | Saved/Logs/<Project>.log 末尾 N 行 |
| Log filter | `filter_ue_log` | category 指定で絞り込み |
| Log verbosity | `set_log_verbosity` | runtime で `LogVoxel Verbose` 等 |

## 前提条件

- UE 5.7 がインストール済 (`C:\Program Files\Epic Games\UE_5.7\`)
- SpirrowBridge plugin が target editor で load されている
- MCP server が起動中 (PowerShell `Get-CimInstance Win32_Process` で確認可)
- 必須運用ルール (`AGENTS.md`): **Python 変更後は MCP server プロセスを kill すること**

## 実装タスク

### Step 0: 既存の `take_screenshot` を Python に expose する (1 分で完了する quick win)

**変更ファイル**: `Python/tools/editor_meta.py`

```python
COMMANDS = {
    # ... 既存 ...
    "take_screenshot": "take_screenshot",  # 追加
    "focus_viewport": "focus_viewport",     # 既に C++ にあるなら同様に追加
}
```

docstring の Commands 列にも追加:
```python
"""Editor: ... commands ..., take_screenshot, focus_viewport"""
```

**既存 C++ 仕様** (`SpirrowBridgeEditorCommands.cpp:786`):
- params: `{"filepath": "C:/path/to/file.png"}` (`.png` 自動付与)
- 動作: `GEditor->GetActiveViewport()->ReadPixels` → `FImageUtils::PNGCompressImageArray` → `FFileHelper::SaveArrayToFile`
- 戻り値: `{"filepath": "..."}` または error
- **注意**: active viewport は editor mode では editor viewport、PIE 中は PIE viewport になる。Phase 1 の `take_pie_screenshot` は同じ実装で OK だが、明示的に PIE viewport を指定したい場合は `GEditor->PlayWorld` から viewport を解決する分岐を追加する。

**MCP server 再起動して動作確認**:
```bash
# 1. プロセス kill
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe', 'uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Select-Object ProcessId, Name | Format-Table -AutoSize"
cmd.exe //C "taskkill /F /PID <uv_pid>"
# 2. 次にツール叩けば自動再接続
```

これで Phase 1 の screenshot 部分は即終わる。残りは C++ 拡張。

### Step 1: PIE 制御コマンド (C++ + Python)

**新規ファイル**: `Commands/SpirrowBridgePIECommands.h` / `.cpp`

#### 必要な UE API

```cpp
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEdEngine.h"  // GEditor
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
```

#### `start_pie`

```cpp
// params:
//   "mode" (optional): "selected_viewport" | "new_window" | "vr"  default "selected_viewport"
//   "spawn_at_player_start" (optional bool): default true; false なら spawn_location/rotation 使用
//   "spawn_location" (optional [x,y,z]): pawn spawn 座標
//   "spawn_rotation" (optional [pitch,yaw,roll]): pawn spawn 回転
//   "single_process" (optional bool): default true (multi-process は今のところ無視)
TSharedPtr<FJsonObject> HandleStartPIE(const TSharedPtr<FJsonObject>& Params)
{
    if (!GEditor) return Error("GEditor null");
    if (GEditor->PlayWorld != nullptr) return Error("PIE already running");

    FRequestPlaySessionParams PlayParams;
    PlayParams.WorldType = EPlaySessionWorldType::PlayInEditor;

    // Mode mapping
    FString Mode;
    Params->TryGetStringField(TEXT("mode"), Mode);
    if (Mode == TEXT("new_window"))
    {
        PlayParams.SessionDestination = EPlaySessionDestinationType::NewProcess;
    }
    // 他は default (selected viewport)

    // Spawn location override
    bool bSpawnAtPlayerStart = true;
    Params->TryGetBoolField(TEXT("spawn_at_player_start"), bSpawnAtPlayerStart);
    if (!bSpawnAtPlayerStart)
    {
        const TArray<TSharedPtr<FJsonValue>>* LocArr;
        if (Params->TryGetArrayField(TEXT("spawn_location"), LocArr) && LocArr->Num() == 3)
        {
            PlayParams.StartLocation = FVector(
                (*LocArr)[0]->AsNumber(),
                (*LocArr)[1]->AsNumber(),
                (*LocArr)[2]->AsNumber());
        }
        const TArray<TSharedPtr<FJsonValue>>* RotArr;
        if (Params->TryGetArrayField(TEXT("spawn_rotation"), RotArr) && RotArr->Num() == 3)
        {
            PlayParams.StartRotation = FRotator(
                (*RotArr)[0]->AsNumber(),
                (*RotArr)[1]->AsNumber(),
                (*RotArr)[2]->AsNumber());
        }
    }

    GEditor->RequestPlaySession(PlayParams);
    // RequestPlaySession は次 tick で実行されるので、即時に "started" を返すか、
    // 1 tick 待ってから PIE world をチェックして返すか選択 (前者の方が安全)。

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), TEXT("requested"));
    Result->SetStringField(TEXT("note"), TEXT("PIE will start on next editor tick. Poll get_pie_state to confirm."));
    return Result;
}
```

#### `stop_pie`

```cpp
TSharedPtr<FJsonObject> HandleStopPIE(const TSharedPtr<FJsonObject>& Params)
{
    if (!GEditor) return Error("GEditor null");
    if (GEditor->PlayWorld == nullptr)
    {
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("status"), TEXT("not_running"));
        return R;
    }
    GEditor->RequestEndPlayMap();
    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetStringField(TEXT("status"), TEXT("requested"));
    return R;
}
```

#### `get_pie_state`

```cpp
TSharedPtr<FJsonObject> HandleGetPIEState(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    if (!GEditor || GEditor->PlayWorld == nullptr)
    {
        R->SetStringField(TEXT("state"), TEXT("stopped"));
        return R;
    }
    UWorld* PIEWorld = GEditor->PlayWorld;
    R->SetStringField(TEXT("state"), PIEWorld->IsPaused() ? TEXT("paused") : TEXT("running"));
    R->SetNumberField(TEXT("time_seconds"), PIEWorld->GetTimeSeconds());
    R->SetNumberField(TEXT("real_time_seconds"), PIEWorld->GetRealTimeSeconds());
    R->SetNumberField(TEXT("delta_seconds"), PIEWorld->GetDeltaSeconds());
    // Player count
    int32 PlayerCount = 0;
    for (FConstPlayerControllerIterator It = PIEWorld->GetPlayerControllerIterator(); It; ++It)
    {
        ++PlayerCount;
    }
    R->SetNumberField(TEXT("player_count"), PlayerCount);
    return R;
}
```

#### `pause_pie` / `resume_pie` (Phase 2)

```cpp
// Pause
APlayerController* PC = UGameplayStatics::GetPlayerController(GEditor->PlayWorld, 0);
PC->SetPause(true);  // または false
```

#### `step_pie_frames` (Phase 2)

UE 標準には「N フレームだけ進める」API は無いので、`PauseSingleStep` console command か `FFrameLimiter` を自前で噛ませる必要あり。実装としては:
1. paused state を解除
2. `FCoreDelegates::OnEndFrame` に N 回 listener 登録、N 回目で再 pause
3. 完了通知を返す (async)

複雑なので Phase 2 で要設計。

### Step 2: PIE Camera 制御

#### `get_pie_camera`

```cpp
TSharedPtr<FJsonObject> HandleGetPIECamera(const TSharedPtr<FJsonObject>& Params)
{
    if (!GEditor || !GEditor->PlayWorld) return Error("PIE not running");
    int32 PlayerIndex = 0;
    Params->TryGetNumberField(TEXT("player_index"), PlayerIndex);

    APlayerController* PC = UGameplayStatics::GetPlayerController(GEditor->PlayWorld, PlayerIndex);
    if (!PC) return Error("PlayerController not found");

    FVector CameraLoc;
    FRotator CameraRot;
    PC->GetPlayerViewPoint(CameraLoc, CameraRot);

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetArrayField(TEXT("location"), {
        MakeShared<FJsonValueNumber>(CameraLoc.X),
        MakeShared<FJsonValueNumber>(CameraLoc.Y),
        MakeShared<FJsonValueNumber>(CameraLoc.Z),
    });
    R->SetArrayField(TEXT("rotation"), {
        MakeShared<FJsonValueNumber>(CameraRot.Pitch),
        MakeShared<FJsonValueNumber>(CameraRot.Yaw),
        MakeShared<FJsonValueNumber>(CameraRot.Roll),
    });
    if (PC->PlayerCameraManager)
    {
        R->SetNumberField(TEXT("fov"), PC->PlayerCameraManager->GetFOVAngle());
    }
    return R;
}
```

#### `set_pie_camera`

ポーンを teleport する版と Debug Cam を使う版の 2 つがあり得る。default は **possessed pawn の SetActorLocation/Rotation**:

```cpp
// params:
//   "location" [x,y,z]
//   "rotation" [pitch,yaw,roll]
//   "player_index" (optional, default 0)
//   "use_debug_cam" (optional bool, default false): true なら DebugCam を起動して位置設定
APawn* Pawn = PC->GetPawn();
if (Pawn)
{
    FHitResult Hit;
    Pawn->SetActorLocationAndRotation(NewLoc, NewRot, false, &Hit, ETeleportType::TeleportPhysics);
    PC->SetControlRotation(NewRot);
}
// else: spectator pawn or debug cam path
```

#### `enable_debug_cam` / `disable_debug_cam` (Phase 2)

```cpp
// DebugCam は console command で toggle
GEngine->Exec(GEditor->PlayWorld, TEXT("ToggleDebugCamera"));
```

### Step 3: Editor viewport camera (PIE 外)

#### `get_editor_camera` / `set_editor_camera`

```cpp
#include "LevelEditorViewport.h"

FLevelEditorViewportClient* GetActivePerspectiveViewport()
{
    if (!GEditor) return nullptr;
    for (FLevelEditorViewportClient* VC : GEditor->GetLevelViewportClients())
    {
        if (VC && VC->IsPerspective())
        {
            return VC;
        }
    }
    return nullptr;
}

TSharedPtr<FJsonObject> HandleGetEditorCamera(const TSharedPtr<FJsonObject>& Params)
{
    FLevelEditorViewportClient* VC = GetActivePerspectiveViewport();
    if (!VC) return Error("No perspective viewport found");

    FVector Loc = VC->GetViewLocation();
    FRotator Rot = VC->GetViewRotation();

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetArrayField(TEXT("location"), {
        MakeShared<FJsonValueNumber>(Loc.X),
        MakeShared<FJsonValueNumber>(Loc.Y),
        MakeShared<FJsonValueNumber>(Loc.Z),
    });
    R->SetArrayField(TEXT("rotation"), {
        MakeShared<FJsonValueNumber>(Rot.Pitch),
        MakeShared<FJsonValueNumber>(Rot.Yaw),
        MakeShared<FJsonValueNumber>(Rot.Roll),
    });
    R->SetNumberField(TEXT("fov"), VC->ViewFOV);
    return R;
}

TSharedPtr<FJsonObject> HandleSetEditorCamera(const TSharedPtr<FJsonObject>& Params)
{
    FLevelEditorViewportClient* VC = GetActivePerspectiveViewport();
    if (!VC) return Error("No perspective viewport found");

    // location / rotation parsing same as set_pie_camera

    VC->SetViewLocation(NewLoc);
    VC->SetViewRotation(NewRot);
    VC->Invalidate();  // 即時再描画
    return Success();
}
```

### Step 4: `exec_console_command`

```cpp
// params:
//   "command" (str): console command 文字列 (例 "stat fps", "showflag.collision 1")
//   "target" (str, optional): "pie" | "editor"  default "pie" if PlayWorld else "editor"
TSharedPtr<FJsonObject> HandleExecConsoleCommand(const TSharedPtr<FJsonObject>& Params)
{
    FString CommandStr;
    if (!Params->TryGetStringField(TEXT("command"), CommandStr))
        return Error("Missing 'command'");

    UWorld* TargetWorld = nullptr;
    FString Target;
    Params->TryGetStringField(TEXT("target"), Target);
    if (Target == TEXT("editor"))
    {
        TargetWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    }
    else
    {
        TargetWorld = (GEditor && GEditor->PlayWorld) ? GEditor->PlayWorld : nullptr;
    }
    if (!TargetWorld) return Error("Target world not available");

    // Output capture: GEngine->Exec returns bool. To capture text output we need
    // a custom FOutputDevice. Phase 1 は output 無視で実行のみ, Phase 2 で capture 追加。
    const bool bOk = GEngine->Exec(TargetWorld, *CommandStr);

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetBoolField(TEXT("success"), bOk);
    return R;
}
```

#### Phase 2: console command output capture

```cpp
class FStringCapture : public FOutputDevice
{
public:
    FString Captured;
    virtual void Serialize(const TCHAR* V, ELogVerbosity::Type, const FName&) override
    {
        Captured += V;
        Captured += TEXT("\n");
    }
};

FStringCapture Cap;
GEngine->Exec(TargetWorld, *CommandStr, Cap);
R->SetStringField(TEXT("output"), Cap.Captured);
```

### Step 5: HighRes Screenshot (Phase 2)

`HighResShot N` console command 経由が安定:
```cpp
// params:
//   "multiplier" (int): 解像度倍率 (1-10)
//   "filepath" (optional str): 出力先 (省略なら Saved/Screenshots/...)
const FString Cmd = FString::Printf(TEXT("HighResShot %d"), Multiplier);
GEngine->Exec(GEditor->PlayWorld ? GEditor->PlayWorld : GEditor->GetEditorWorldContext().World(), *Cmd);
// 出力ファイル名は HighResShot 仕様準拠 (Saved/Screenshots/<Platform>/HighresScreenshot00000.png 等)
// custom path 必要なら CVar `r.HighResScreenshotConfig.FilenameOverride` を事前 set。
```

### Step 6: PIE world actor 取得 (Phase 2)

既存 `get_actors_in_level` は editor world 固定。新コマンド `get_pie_actors` を追加:

```cpp
TSharedPtr<FJsonObject> HandleGetPIEActors(const TSharedPtr<FJsonObject>& Params)
{
    if (!GEditor || !GEditor->PlayWorld) return Error("PIE not running");
    UWorld* World = GEditor->PlayWorld;

    TArray<TSharedPtr<FJsonValue>> Actors;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* A = *It;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("name"), A->GetName());
        Obj->SetStringField(TEXT("class"), A->GetClass()->GetName());
        // location/rotation/scale 同様
        Actors.Add(MakeShared<FJsonValueObject>(Obj));
    }
    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetArrayField(TEXT("actors"), Actors);
    return R;
}
```

### Step 7: Log access (Phase 2)

UE のログは `Saved/Logs/<ProjectName>.log` に append される。tail だけなら ファイル末尾読みで OK:

```cpp
TSharedPtr<FJsonObject> HandleTailUELog(const TSharedPtr<FJsonObject>& Params)
{
    int32 LineCount = 50;
    Params->TryGetNumberField(TEXT("lines"), LineCount);

    const FString LogPath = FPaths::ProjectLogDir() / FApp::GetProjectName() + TEXT(".log");
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *LogPath))
        return Error("Failed to read log");

    TArray<FString> Lines;
    Content.ParseIntoArray(Lines, TEXT("\n"));
    const int32 Start = FMath::Max(0, Lines.Num() - LineCount);
    TArray<TSharedPtr<FJsonValue>> Out;
    for (int32 i = Start; i < Lines.Num(); ++i)
    {
        Out.Add(MakeShared<FJsonValueString>(Lines[i]));
    }
    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetArrayField(TEXT("lines"), Out);
    R->SetNumberField(TEXT("total_lines"), Lines.Num());
    return R;
}
```

`filter_ue_log` は category prefix で grep:
```cpp
// params: "category" (str), "lines" (int default 50)
```

`set_log_verbosity`:
```cpp
GEngine->Exec(nullptr, *FString::Printf(TEXT("Log %s %s"), *Category, *Verbosity));
// Verbosity: "Verbose" / "VeryVerbose" / "Log" / "Display" / "Warning" / "Error" / "Off"
```

## ファイル構成

### 新規作成

```
MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/
├── Public/Commands/SpirrowBridgePIECommands.h
└── Private/Commands/SpirrowBridgePIECommands.cpp

Python/tools/
└── pie_meta.py        # 新カテゴリ "pie"
```

### 編集

```
MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/
├── Private/SpirrowBridge.cpp                     # ExecuteCommand に PIE/screenshot ルーティング追加
└── Private/Commands/SpirrowBridgeEditorCommands.cpp  # take_pie_screenshot, *_editor_camera ハンドラ追加

Python/
├── unreal_mcp_server.py                          # register_pie_meta_tool 呼び出し追加
└── tools/
    ├── editor_meta.py                            # take_screenshot, get_editor_camera, set_editor_camera 登録
    ├── command_schemas.py                        # 新コマンドのスキーマ定義
    └── help_tool.py                              # help() の category 追加
```

## ルーティング更新

**`SpirrowBridge.cpp` ExecuteCommand**:

```cpp
// 既存の Editor block に追加
else if (CommandType == TEXT("get_actors_in_level") || ...
         CommandType == TEXT("take_screenshot") ||
         CommandType == TEXT("take_pie_screenshot") ||
         CommandType == TEXT("get_editor_camera") ||
         CommandType == TEXT("set_editor_camera"))
{
    ResultJson = EditorCommands->HandleCommand(CommandType, Params);
}
// 新ブロック PIE
else if (CommandType == TEXT("start_pie") ||
         CommandType == TEXT("stop_pie") ||
         CommandType == TEXT("get_pie_state") ||
         CommandType == TEXT("pause_pie") ||
         CommandType == TEXT("resume_pie") ||
         CommandType == TEXT("step_pie_frames") ||
         CommandType == TEXT("get_pie_camera") ||
         CommandType == TEXT("set_pie_camera") ||
         CommandType == TEXT("enable_debug_cam") ||
         CommandType == TEXT("disable_debug_cam") ||
         CommandType == TEXT("exec_console_command") ||
         CommandType == TEXT("get_pie_actors") ||
         CommandType == TEXT("find_pie_actors_by_class") ||
         CommandType == TEXT("get_pie_actor_properties") ||
         CommandType == TEXT("set_global_time_dilation") ||
         CommandType == TEXT("take_high_res_screenshot") ||
         CommandType == TEXT("tail_ue_log") ||
         CommandType == TEXT("filter_ue_log") ||
         CommandType == TEXT("set_log_verbosity"))
{
    ResultJson = PIECommands->HandleCommand(CommandType, Params);
}
```

PIECommands インスタンスは SpirrowBridge.cpp の他コマンドと同じパターンでメンバ変数として保持。

## エラーコード割当

`Docs/ERROR_CODES.md` に追加:

```
1700-1799  PIE
  1700  PIE not running
  1701  PIE already running
  1702  PIE start failed
  1703  PlayerController not found
  1704  Pawn not found
  1705  Console command failed
  1706  HighResScreenshot failed
  1707  Log file not accessible
```

## ビルド + 動作確認

### ビルド (target editor を閉じる必要あり)

```bash
# 1. UE editor (voxel project等) を閉じる
# 2. SpirrowBridge を含む target uproject をビルド
powershell.exe -Command "& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' VoxelWorldHostEditor Win64 Development '-Project=C:\Users\owner\Documents\Unreal Projects\VoxelWorldHost\VoxelWorldHost.uproject' -WaitMutex"

# 3. または unrealwise 専用 editor をビルド
powershell.exe -Command "& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' MCPGameProjectEditor Win64 Development '-Project=C:\Users\owner\Documents\Unreal Projects\spirrow-unrealwise\MCPGameProject\MCPGameProject.uproject' -WaitMutex"
```

### MCP server 再起動 (Python 変更後)

```bash
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe', 'uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Select-Object ProcessId, ParentProcessId, Name | Format-Table -AutoSize"
cmd.exe //C "taskkill /F /PID <uv_pid>"
```

## テスト手順

### 統合動作確認シナリオ (voxel SeamOctree 検証を題材に)

```python
# 1. editor 起動 (人間 or 別途 automation)、Spirrow-VoxelWorld の Test_LOD.umap を open
editor("open_level", {"level_path": "/Game/Maps/Test_LOD"})

# 2. Editor camera を LOD 境界が見える位置に設定
editor("set_editor_camera", {
    "location": [3200, 3200, 8000],
    "rotation": [-45, 0, 0]
})

# 3. Editor viewport screenshot (PIE 起動前)
editor("take_screenshot", {"filepath": "C:/temp/before_pie.png"})

# 4. PIE 起動
pie("start_pie", {"spawn_at_player_start": true})

# 5. PIE state 確認 (1-2 秒待機 or polling)
pie("get_pie_state")  # → {"state": "running", ...}

# 6. PIE camera 移動 (LOD 境界の near-far を覗ける位置に)
pie("set_pie_camera", {
    "location": [0, 0, 5000],
    "rotation": [-30, 0, 0]
})

# 7. console command で chunk debug viz on
pie("exec_console_command", {"command": "voxel.show_chunk_borders 1"})

# 8. PIE viewport screenshot
pie("take_pie_screenshot", {"filepath": "C:/temp/lod_boundary_check.png"})

# 9. Log で SeamOctree が動いたか確認
pie("tail_ue_log", {"lines": 100})
pie("filter_ue_log", {"category": "LogVoxel", "lines": 50})

# 10. PIE 停止
pie("stop_pie")
```

### 単体テスト追加 (`Python/tests/test_pie.py` 新規)

```python
def test_screenshot_takes_png():
    result = client.call("editor", {"command": "take_screenshot",
                                      "params": {"filepath": "/tmp/test_ss.png"}})
    assert result["filepath"].endswith(".png")
    assert os.path.exists(result["filepath"])

def test_pie_start_stop_cycle():
    # 初期 stopped
    s1 = client.call("pie", {"command": "get_pie_state"})
    assert s1["state"] == "stopped"

    client.call("pie", {"command": "start_pie", "params": {}})
    time.sleep(2)  # PIE start は async、tick 待ち

    s2 = client.call("pie", {"command": "get_pie_state"})
    assert s2["state"] == "running"

    client.call("pie", {"command": "stop_pie", "params": {}})
    time.sleep(1)

    s3 = client.call("pie", {"command": "get_pie_state"})
    assert s3["state"] == "stopped"
```

## 完了条件

### Phase 1 (MVP)
- [ ] `take_screenshot` が editor viewport / PIE viewport 両方で PNG 出力できる
- [ ] `start_pie` / `stop_pie` / `get_pie_state` が動く
- [ ] `get_pie_camera` / `set_pie_camera` で player camera を取得 + 移動可能
- [ ] `get_editor_camera` / `set_editor_camera` で editor viewport camera 操作可能
- [ ] `exec_console_command` で任意 console command 実行可能
- [ ] 統合シナリオ (voxel LOD 境界スクショ取得) が end-to-end 通る
- [ ] Python tests 新規追加 + 既存テスト全 pass
- [ ] FEATURE_STATUS.md, AGENTS.md, README.md/README_ja.md, Docs/PATTERNS.md (該当箇所) 更新
- [ ] `help("pie")` で category overview が表示される
- [ ] エラーコード 1700-1799 を ERROR_CODES.md に追加

### Phase 2 (高機能化)
- [ ] `pause_pie` / `resume_pie` / `step_pie_frames` 実装
- [ ] `take_high_res_screenshot` で任意倍率出力
- [ ] `get_pie_actors` / `find_pie_actors_by_class` / `get_pie_actor_properties` で PIE world actor inspection
- [ ] `enable_debug_cam` / `disable_debug_cam` 実装
- [ ] `set_global_time_dilation` 実装
- [ ] `tail_ue_log` / `filter_ue_log` / `set_log_verbosity` 実装
- [ ] `exec_console_command` の output capture (FOutputDevice 経由)
- [ ] tests 拡充 (各機能ごと)

## 注意事項

### PIE start の async 性
`RequestPlaySession` は即時に PIE world を作らない。次 tick で実際に start する。MCP コマンドは即時返すしか無いので、status="requested" を返して clients は `get_pie_state` で polling する設計が現実解。1-2 秒の sleep を Python wrapper で挟むのは安全だが、長時間 block するのは MCP の 30 秒タイムアウトに注意。

### viewport / world の選択ミス
- `GEditor->GetEditorWorldContext().World()` = Editor world (always)
- `GEditor->PlayWorld` = PIE world (nullptr if not playing)
- `GEditor->GetActiveViewport()` = active viewport (Editor or PIE)
- 明示的に分けたい場合 `GEngine->GetWorldContexts()` を回して `WorldType == EWorldType::PIE` を探す

### editor 閉じている状態でのコマンド
PIE 関連は editor 起動が前提。SpirrowBridge plugin が読み込まれている = editor が動いている。なので editor 死んだら全 MCP コマンドが死ぬのは想定内。

### Plugin rebuild = editor close 必須
SpirrowBridge を含む project の editor を閉じてからじゃないと DLL ロックで build 失敗。voxel 側の editor を一旦落として build するワークフローになる。これは UnrealWise 開発では避けられない (voxel project の editor で SpirrowBridge を読み込んでいる場合)。

### PIE viewport の screenshot 解像度
default の active viewport は editor の viewport panel サイズ (1280x720 とか)。PIE 起動方法によって解像度が違う。HighResShot を使う方が解像度コントロール可能。

## 参考リンク

- UE Source: `Engine/Source/Editor/UnrealEd/Public/Editor.h` (RequestPlaySession 等)
- UE Source: `Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerController.h` (GetPlayerViewPoint 等)
- UE Source: `Engine/Source/Editor/LevelEditor/Public/LevelEditorViewport.h` (FLevelEditorViewportClient)
- HighResShot 仕様: `Engine/Source/Runtime/Engine/Private/HighResScreenshot.cpp`

## 引き継ぎ後の次セッション開始時の最初のアクション

```python
# 1. プロジェクトコンテキスト取得
get_project_context("spirrow-unrealwise")

# 2. この指示書を読む
# Path: C:/Users/owner/Documents/Unreal Projects/spirrow-unrealwise/Docs/Prompts/Features/PIE_Control_Screenshot_Implementation_Prompt.md

# 3. 既存の Editor commands 実装を確認 (パターン把握)
# C:/Users/owner/Documents/Unreal Projects/spirrow-unrealwise/MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeEditorCommands.cpp:786 (HandleTakeScreenshot 既存実装)

# 4. Step 0 (take_screenshot Python expose) で 1 行修正 + MCP サーバ再起動 → 即動作確認
# 5. Step 1 以降を順次実装
```
