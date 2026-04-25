# take_pie_screenshot が PIE viewport を描画しない問題 - 修正指示書

> **発見**: 2026-04-26 spirrow-voxelworld の SeamOctree C5 修正 PIE 視覚検証中
> **症状**: `pie.take_pie_screenshot` が緑/黒の placeholder PNG を返す
> **対照**: `pie.take_high_res_screenshot` (HighResShot console 経由) は正常に terrain を撮影できる
> **対照**: `editor.take_screenshot` も editor viewport (level editor preview) を正常撮影
> **影響**: PIE 中に多角度から visual regression 比較ができない (今は HighResShot でしか取れず file path 制御も効かない)

---

## 2026-04-26 02:00 1 回目修正後の追加検証で判明したより深い問題

1 回目の修正 (Fix B/C) 完了後の 2 回目検証で、当初想定していなかった **より根本的な問題** を発見。

### 1 回目修正の動作確認結果

| Fix | 状態 | 確認 |
|---|---|---|
| **A) take_pie_screenshot の render sync** | ❌ **未効** | 修正後も `pie.take_pie_screenshot` は PIE viewport を映さず緑のまま。`FlushRenderingCommands` 1 行追加では足りない可能性 |
| **B) take_high_res_screenshot filepath override** | ✅ 効いた | `C:/temp/foo.png` 指定通りに出力される |
| **C) set_pie_camera response の location** | ✅ 効いた | target/previous 両方が response に含まれるようになった |

### より深い問題: PIE pawn camera が screenshot に反映しない

**現象** (2026-04-26 04:11 検証):
- PIE 起動 + DefaultPawn を spawn
- `set_pie_camera` で 3 つの異なる極端な位置に teleport: (3200,3200,5000) → (10000,10000,4000) → (-50000,-50000,10000)
- 各位置で `pie.take_high_res_screenshot multiplier=1 filepath="C:/temp/v[1-3].png"` 実行
- 結果: **3 枚とも視覚的に同一画像** (compare_screenshots で diff_pixel_pct=0.06%)
- get_pie_camera で確認すると pawn は確かにその位置に居る

**つまり**: HighResShot は PIE pawn の POV ではなく **editor viewport (level editor の固定カメラ)** を撮っている。`take_pie_screenshot` も同じく editor viewport 経由 (緑の placeholder は active viewport の clear color を読んでるだけ)。

PIE 中に PIE pawn の POV を捕まえる経路が **MCP 経由では現状存在しない**。

### 検証エビデンス
| ファイル | 撮影位置 | 結果 |
|---|---|---|
| `C:/temp/c5_baseline/after_v1.png` | PIE pawn=(3200, 3200, 5000) rot=[-45,0,0] | terrain 見える |
| `C:/temp/c5_baseline/after_v2.png` | PIE pawn=(10000, 10000, 4000) rot=[-30,-135,0] | **after_v1 と完全同一** |
| `C:/temp/c5_baseline/after_v3.png` | PIE pawn=(6400, 0, 3500) rot=[-15,90,0] | **after_v1 と完全同一** |
| `C:/temp/c5_baseline/test_far.png` | PIE pawn=(-50000,-50000,10000) rot=[-45,45,0] | **after_v1 と完全同一** (50km 離れても!) |

→ 全部 editor viewport の固定 camera からの screenshot。PIE pawn 位置は無視されてる。

### 真の root cause

`HighResShot N` console コマンドは **active viewport** (`GEditor->GetActiveViewport()`) を render する。
- PIE が `selected_viewport` モードで起動 → active viewport は editor の level viewport
- だが UE 5.7 の最近の挙動として、PIE 中も level viewport の **editor camera** はそのまま (PIE pawn POV にはならない)
- HighResShot はその editor camera の view を撮るので、PIE pawn 移動とは無関係

`pie.take_pie_screenshot` の C++ 側はおそらく `GEditor->PlayWorld->GetGameViewport()->Viewport->ReadPixels` を呼んでるが、こちらの GameViewport は PIE 中も「画面に描画されない仮想 viewport」になってて、ReadPixels すると初期 clear color (緑) を返す。

つまり **PIE の rendering 出力先が editor viewport ではなく非表示の back buffer** になっており、それを読むには別の経路が必要。

### 修正方針 (推奨アプローチ)

#### 案 1: `selected_viewport` モードで editor camera を PIE pawn に追従させる

`start_pie` 後に `GLevelEditorModeTools().SetCamera(PawnLoc, PawnRot)` を毎フレーム呼ぶ、もしくは `FLevelEditorViewportClient::SetCameraToActor(PawnActor)` で actor 追従設定。

これで level viewport の editor camera が PIE pawn を follow → HighResShot/editor.take_screenshot で PIE pawn POV を撮れる。

#### 案 2: PIE を `new_window` モードで起動 + その window の captured texture を読む

`FRequestPlaySessionParams::SessionDestination = NewProcess` で別 window 起動 → window handle から OS-level capture (Windows API `BitBlt` など) → かなり重い、推奨しない。

#### 案 3: `SceneCapture2D` actor を PIE world に spawn して毎フレーム capture

PIE world に SceneCapture2D actor を spawn (位置 = PIE pawn と同じ)、SceneCaptureComponent2D の output texture を `RenderTarget->ReadPixels` で読む。

PIE pawn 移動に追従して capture actor も move、texture を PNG エンコードして file 出力。

```cpp
// 新コマンド: pie.take_pie_pov_screenshot
// 内部実装:
// 1. PIE world に ASceneCapture2D を spawn (1回のみ、static keep)
// 2. capture component の TextureTarget を 1920x1080 程度の UTextureRenderTarget2D で create
// 3. PIE pawn の location/rotation を read
// 4. capture actor をその位置に teleport
// 5. capture->CaptureScene() を呼んで render trigger
// 6. FlushRenderingCommands()
// 7. RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels()
// 8. PNG encode + save to filepath
APlayerController* PC = UGameplayStatics::GetPlayerController(PIEWorld, 0);
FVector ViewLoc; FRotator ViewRot;
PC->GetPlayerViewPoint(ViewLoc, ViewRot);

ASceneCapture2D* Capture = GetOrSpawnPIECaptureActor(PIEWorld);  // cached
Capture->SetActorLocationAndRotation(ViewLoc, ViewRot);
USceneCaptureComponent2D* Comp = Capture->GetCaptureComponent2D();
Comp->FOVAngle = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetFOVAngle() : 90.0f;
Comp->TextureTarget = GetOrCreatePIECaptureRT(1920, 1080);  // cached UTextureRenderTarget2D
Comp->CaptureScene();
FlushRenderingCommands();

// readback
TArray<FColor> Bitmap;
Comp->TextureTarget->GameThread_GetRenderTargetResource()->ReadPixels(Bitmap);
// PNG 圧縮 + save
```

**利点**: PIE pawn POV を確実に取れる、headless 動作可能、HUD 等 game viewport 固有の overlay も入る (CaptureSource 設定で調整可)
**欠点**: SceneCapture2D は post-process が異なる (TemporalAA 切れる等)、game viewport の HUD は写らない、初回 spawn コスト

#### 案 4: PIE GameViewport の back buffer を直接 readback (要 RHI 知識)

GameViewport が rendering している back buffer が `FViewport::GetSlateTextureRHIRef` 経由で取れるなら、そこから RHI readback。これは複雑なので case-by-case。

### 推奨実装

**案 3 (SceneCapture2D)** を `pie.take_pie_pov_screenshot` という新コマンドで追加。既存の `take_pie_screenshot` は editor viewport 撮影 (実質 `editor.take_screenshot` と同じ動作) として残し、新しい POV 専用コマンドで PIE pawn 視点を撮る。

ユーザの操作的には:
- `take_pie_pov_screenshot` → PIE pawn の POV
- `take_pie_screenshot` → PIE 中の editor viewport (= editor.take_screenshot と etc.)
- `take_high_res_screenshot` → HighResShot (editor viewport の高解像度版)

### Phase B 視覚 regression 検証への影響

spirrow-voxelworld の C5 修正視覚 diff は **`take_pie_pov_screenshot` 実装後にやり直す** 必要あり。現状の MCP では editor camera が固定なので、PIE 中の正しい POV diff が取れない。

代替: editor 側で **`UVoxelEditorPreviewComponent`** が seam mesh を生成しているなら、editor viewport (PIE 不要) で diff 可能。要確認 (voxel 側コードで `BuildSeamMeshForChunk` が editor preview path から呼ばれてるか)。

## コンテキスト

- **プロジェクト**: spirrow-unrealwise
- **branch**: `feat/v0.9.9-layout-polish` ベース推奨
- **対象 UE バージョン**: 5.7
- **影響を受けるツール**: `pie.take_pie_screenshot`, `pie.take_high_res_screenshot` (filepath override 機能), `pie.set_pie_camera` (response data の stale 問題)
- **検証用 target project**: `C:/Users/owner/Documents/Unreal Projects/VoxelWorldHost/`

## 検証で取れた具体例

| ファイル | 撮影方法 | 結果 |
|---|---|---|
| `C:/temp/editor_during_pie.png` | `editor.take_screenshot` (PIE 起動中) | ✅ Editor viewport の terrain が見える (LOD step 線含む) |
| `C:/temp/lod_view1.png` | `pie.take_pie_screenshot` (z=5000 直下視点) | ❌ ほぼ単色緑 (sky atmosphere?) |
| `C:/temp/lod_topdown.png` | `pie.take_pie_screenshot` (z=5000, pitch=-89) | ❌ 同上、緑 |
| `C:/temp/lod_with_stat.png` | `pie.take_pie_screenshot` (stat scenerendering 後) | ❌ 黒、stat オーバーレイは見える |
| `C:/Users/owner/Documents/Unreal Projects/VoxelWorldHost/Saved/Screenshots/WindowsEditor/HighresScreenshot00001.png` | `pie.take_high_res_screenshot multiplier=1` | ✅ Terrain & LOD step 全部見える |

→ **HighResShot 経路は OK、`Viewport->ReadPixels` 直接呼び経路が NG**。仮説: render thread と sync 取らずに ReadPixels してるため、フレーム描画前の clear color (緑 = level の SkyAtmosphere clear color) を読んでいる。

## 修正対象

### A) `take_pie_screenshot` を render-thread 同期に修正 (**最優先**)

**現状の実装場所** (推定): `Commands/SpirrowBridgePIECommands.cpp` の `HandleTakePIEScreenshot` (新コマンド)
**または既存 `HandleTakeScreenshot` (`SpirrowBridgeEditorCommands.cpp:786`) を流用**

#### 現状コード (推定)

```cpp
TSharedPtr<FJsonObject> HandleTakePIEScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GEditor ? GEditor->PlayWorld : nullptr;
    if (!PIEWorld) return Error("PIE not running");

    UGameViewportClient* GVC = PIEWorld->GetGameViewport();
    if (!GVC || !GVC->Viewport) return Error("No game viewport");

    FViewport* Viewport = GVC->Viewport;
    TArray<FColor> Bitmap;
    FIntRect Rect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
    Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), Rect);
    // PNG compress + save
    ...
}
```

問題: `ReadPixels` は **その時点の back buffer を即読み**。直前の `set_pie_camera` で teleport したフレームの描画はまだ render thread キューに積まれただけで GPU 完了前。結果として前フレーム (or clear color) を読む。

#### 修正案 1: `FlushRenderingCommands` を挟む (簡単、確実)

```cpp
TSharedPtr<FJsonObject> HandleTakePIEScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GEditor ? GEditor->PlayWorld : nullptr;
    if (!PIEWorld) return Error("PIE not running");

    UGameViewportClient* GVC = PIEWorld->GetGameViewport();
    if (!GVC || !GVC->Viewport) return Error("No game viewport");

    FViewport* Viewport = GVC->Viewport;

    // 1) render thread のキューを drain する (GPU 完了まで block)
    FlushRenderingCommands();

    // 2) その上で ReadPixels (ここでようやく最新フレームの内容)
    TArray<FColor> Bitmap;
    FIntRect Rect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
    if (!Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), Rect))
    {
        return Error("ReadPixels failed");
    }

    // 3) PNG compress + save (既存と同じ)
    ...
}
```

**利点**: 1 行追加で済む、editor 内部 API のみ
**欠点**: game thread を block する (PIE 一時停止せざるを得ない)。1 frame 分の wait 程度なので実用上問題なし

#### 修正案 2: `FScreenshotRequest` 経由 (推奨、UE 標準)

```cpp
#include "ImageWriteQueue.h"
#include "UnrealClient.h"   // FScreenshotRequest

TSharedPtr<FJsonObject> HandleTakePIEScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), FilePath))
        return Error("Missing 'filepath'");

    UWorld* PIEWorld = GEditor ? GEditor->PlayWorld : nullptr;
    if (!PIEWorld) return Error("PIE not running");

    UGameViewportClient* GVC = PIEWorld->GetGameViewport();
    if (!GVC || !GVC->Viewport) return Error("No game viewport");

    // Request a screenshot — UE schedules it for the END of the current frame
    FScreenshotRequest::RequestScreenshot(FilePath, /*bShowUI=*/false, /*bAddFilenameSuffix=*/false);

    // Block the calling thread until the request completes (~1 frame).
    // FScreenshotRequest::OnScreenshotCaptured() を待つか、シンプルに 1 frame
    // FlushRenderingCommands() でも可。
    GVC->Viewport->Draw();
    FlushRenderingCommands();

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetStringField(TEXT("filepath"), FilePath);
    R->SetStringField(TEXT("source"), TEXT("pie_viewport_request"));
    return R;
}
```

**利点**: UE の標準 screenshot pipeline 経由、render thread の sync は UE が面倒見る
**欠点**: file 出力経路は UE が決める (filepath 引数で path 指定可だが UE 側が拡張子・suffix を勝手に付ける場合あり)

#### 推奨: 案 1 を default + 案 2 を `mode: "request"` で選択可能に

```cpp
// params:
//   "filepath": str
//   "mode" (optional): "readpixels" (default, sync block) | "request" (UE pipeline)
```

### B) `take_high_res_screenshot` の filepath override が効いてない

**現状**: `multiplier=1, filepath="C:/temp/foo.png"` で叩いても `Saved/Screenshots/WindowsEditor/HighresScreenshotNNNNN.png` に出力される。response の `filepath_override` field は記録されてるが反映されてない。

**原因**: HighResShot console は内部で `r.HighResScreenshotConfig.FilenameOverride` CVar を見るが、コマンド発行前に CVar セットしてない可能性高い。

**修正**: `HandleTakeHighResScreenshot` 内で HighResShot console 発行**前**に CVar set:

```cpp
TSharedPtr<FJsonObject> HandleTakeHighResScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    int32 Multiplier = 1;
    Params->TryGetNumberField(TEXT("multiplier"), Multiplier);
    Multiplier = FMath::Clamp(Multiplier, 1, 10);

    FString FilePath;
    const bool bHasFilePath = Params->TryGetStringField(TEXT("filepath"), FilePath);

    UWorld* World = (GEditor && GEditor->PlayWorld)
        ? GEditor->PlayWorld
        : (GEditor ? GEditor->GetEditorWorldContext().World() : nullptr);
    if (!World) return Error("No world available");

    if (bHasFilePath)
    {
        // Ensure parent dir exists, normalize separator etc.
        FPaths::MakePathRelativeTo(FilePath, *FPaths::ProjectDir());  // optional
        // CVar 経由で次の HighResShot のファイル名を上書き
        IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(
            TEXT("r.HighResScreenshotConfig.FilenameOverride"));
        if (CVar) CVar->Set(*FilePath);
    }

    const FString Cmd = FString::Printf(TEXT("HighResShot %d"), Multiplier);
    GEngine->Exec(World, *Cmd);

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetStringField(TEXT("status"), TEXT("requested"));
    R->SetNumberField(TEXT("multiplier"), Multiplier);
    R->SetStringField(TEXT("filepath_override"), bHasFilePath ? FilePath : TEXT(""));
    R->SetStringField(TEXT("note"),
        bHasFilePath
            ? TEXT("HighResShot is async - file written to specified filepath on the next frame")
            : TEXT("HighResShot is async - file written to Saved/Screenshots/<Platform>/HighresScreenshotNNNNN.png"));
    return R;
}
```

注意: `r.HighResScreenshotConfig.FilenameOverride` の正確な CVar 名が UE バージョンで違う場合あり。`UE_5.7/Engine/Source/Runtime/Engine/Private/HighResScreenshot.cpp` で `Filename` 関連の CVar 探す。代替案として直接 `FHighResScreenshotConfig::SetFilename(FilePath)` を呼ぶ:

```cpp
#include "HighResScreenshot.h"
...
GetHighResScreenshotConfig().SetFilename(FilePath);
GEngine->Exec(World, *FString::Printf(TEXT("HighResShot %d"), Multiplier));
```

これは `FHighResScreenshotConfig` のメンバ名を確認してから (`Filename` or `OverrideName` etc.)。

### C) `set_pie_camera` の response data が stale な座標を返す

**現状**: 
```json
// request: location=[3200, 3200, 5000]
// response.data.location: [0, 0, 0]   ← teleport BEFORE の座標を返す
// response.data.pawn_teleported: true
```

直後の `get_pie_camera` を叩くと正しい新座標が返る。応答だけが stale。

**原因**: handler 内で `Pawn->SetActorLocationAndRotation` 呼んだ後に `Pawn->GetActorLocation()` を読んでいるが、`SetActorLocationAndRotation` が次 tick で反映される設定 (or move component が override してる)。

**修正**: response の location は **request された target location** をそのまま返す (`Pawn->GetActorLocation` 読みは廃止):

```cpp
TSharedPtr<FJsonObject> HandleSetPIECamera(const TSharedPtr<FJsonObject>& Params)
{
    // ... parse target loc/rot ...

    APawn* Pawn = PC->GetPawn();
    bool bTeleported = false;
    if (Pawn && !bUseDebugCam)
    {
        FHitResult Hit;
        bTeleported = Pawn->SetActorLocationAndRotation(
            TargetLoc, TargetRot, false, &Hit, ETeleportType::TeleportPhysics);
        if (PC) PC->SetControlRotation(TargetRot);
    }

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    // 重要: target を返す (現在の Actor location ではなく)
    R->SetArrayField(TEXT("location"), {
        MakeShared<FJsonValueNumber>(TargetLoc.X),
        MakeShared<FJsonValueNumber>(TargetLoc.Y),
        MakeShared<FJsonValueNumber>(TargetLoc.Z),
    });
    R->SetArrayField(TEXT("rotation"), {
        MakeShared<FJsonValueNumber>(TargetRot.Pitch),
        MakeShared<FJsonValueNumber>(TargetRot.Yaw),
        MakeShared<FJsonValueNumber>(TargetRot.Roll),
    });
    R->SetBoolField(TEXT("pawn_teleported"), bTeleported);
    R->SetBoolField(TEXT("debug_cam_toggled"), bUseDebugCam);
    return R;
}
```

または response に `target_location` と `previous_location` の両方を含めて誤解を防ぐ。

## 実装後の検証手順

1. C++ 修正 + Python の docstring 更新 (filepath override が効くようになった旨)
2. SpirrowBridge plugin rebuild (target project の editor 閉 → Build.bat)
3. MCP server プロセス kill (Python は変えてないなら不要だが安全のため)
4. UE editor 再起動
5. **動作確認シナリオ** (spirrow-voxelworld の検証で実際に詰まった操作):

```python
# A) PIE 起動 + 高所視点で screenshot
pie("start_pie", {})
# wait 3-5 秒
pie("set_pie_camera", {
    "location": [3200, 3200, 5000],
    "rotation": [-89, 0, 0]
})
# wait 1 秒 (teleport 反映)
pie("take_pie_screenshot", {"filepath": "C:/temp/test_pie_topdown.png"})
# 期待: terrain が見える PNG が出力される (緑 placeholder ではない)

# B) HighRes filepath override
pie("take_high_res_screenshot", {
    "multiplier": 2,
    "filepath": "C:/temp/test_highres_x2.png"
})
# wait 2 秒
# 期待: C:/temp/test_highres_x2.png が出力される
#       (Saved/Screenshots/... に行かず指定先に出る)

# C) set_pie_camera response が target を返す
result = pie("set_pie_camera", {
    "location": [10000, 10000, 8000],
    "rotation": [-45, 90, 0]
})
assert result["data"]["location"] == [10000, 10000, 8000]  # target そのまま
```

## 完了条件

- [ ] `take_pie_screenshot` が PIE viewport の最新描画フレームを PNG 出力できる (緑 placeholder ではない)
- [ ] `take_pie_screenshot` の filepath param 通りに出力される
- [ ] `take_high_res_screenshot` の filepath override が効く (Saved/Screenshots/ に行かない)
- [ ] `set_pie_camera` の response.data.location が target 値を返す (現 Actor 座標ではない)
- [ ] 上記検証シナリオ A/B/C 全通る
- [ ] FEATURE_STATUS.md に修正済の旨追記
- [ ] Python docstring 更新 (filepath override 動作の保証を明記)

## ファイル

### 編集
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgePIECommands.cpp`
  - `HandleTakePIEScreenshot` (FlushRenderingCommands or FScreenshotRequest 経由化)
  - `HandleTakeHighResScreenshot` (filepath override 反映)
  - `HandleSetPIECamera` (response の location を target に変更)
- `Python/tools/pie_meta.py`
  - docstring 更新

### 関連参考
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeEditorCommands.cpp:786` (既存 `HandleTakeScreenshot` - editor viewport 用、同じ ReadPixels パターンだが editor では sync 問題出てない、参考に)
- `Engine/Source/Runtime/Engine/Private/HighResScreenshot.cpp` (CVar 名確認)
- `Engine/Source/Runtime/Engine/Public/HighResScreenshot.h` (`FHighResScreenshotConfig` API)

## 修正後にやる Phase B 視覚 regression 検証 (spirrow-voxelworld 側)

修正完了後、以下を AI 自身で end-to-end 自動化できる:

```python
# 1. C5 修正後 baseline screenshot
pie("start_pie", {})
pie("set_pie_camera", {"location": [3200, 3200, 5000], "rotation": [-45, 0, 0]})
pie("take_pie_screenshot", {"filepath": "C:/temp/c5_after.png"})
pie("stop_pie", {})

# 2. C5 直前へ checkout + rebuild (別プロセスで)
# git -C ".../Spirrow-VoxelWorld" checkout e173e6f
# UE editor close
# Build.bat VoxelWorldHostEditor ...
# UE editor 再起動

# 3. C5 直前 baseline
pie("start_pie", {})
pie("set_pie_camera", {"location": [3200, 3200, 5000], "rotation": [-45, 0, 0]})
pie("take_pie_screenshot", {"filepath": "C:/temp/c5_before.png"})
pie("stop_pie", {})

# 4. Diff
compare_screenshots(
    path_a="C:/temp/c5_before.png",
    path_b="C:/temp/c5_after.png",
    output_diff_path="C:/temp/c5_diff.png",
    threshold=10
)
# 期待: diff_pixel_pct が小さい正の値 (LOD 境界の seam tris 追加分のみ差異)
#       全体の terrain は同一
```

この自動化が現状できないため C 先行 → B が依頼の流れ。

## 注意事項 (Phase 2 でも残る課題)

- **PIE 中の voxel chunk mesh 生成タイミング**: PIE 起動直後は chunks は spawn されてるが mesh が async 生成中。`take_pie_screenshot` した時点で完成してないと地形が映らない。`pie.wait_for_chunk_meshes` のような新コマンド (chunk actor の mesh component vertex count > 0 を polling) があると visual diff の安定性 ↑。優先度は低い (sleep で十分な場合多い)。
- **`enable_debug_cam` / `disable_debug_cam` が同じ console コマンドの toggle**: 状態 mismatch しやすい。internal flag で track して "ensure_debug_cam_state(true|false)" のような state-machine 化も改善ポイント (低優先度)。
- **`set_pie_camera` の `use_debug_cam` パラメータ**: 現状 toggle した上で teleport だが、debug cam は別の actor (ASpectatorPawn 派生)。debug cam mode 中の teleport は normal pawn ではなく debug cam actor へ送るべき。pawn_teleported=true が誤っているケースあり (debug cam に send したのに pawn 動かしてる、と表示される)。

## 引き継ぎ後の最初のアクション

```python
# 1. プロジェクトコンテキスト
get_project_context("spirrow-unrealwise")

# 2. この指示書を読む
# Path: C:/Users/owner/Documents/Unreal Projects/spirrow-unrealwise/Docs/Prompts/BugFix/PIE_Screenshot_Render_Sync_Fix_Prompt.md

# 3. 症状を再現する screenshot を確認 (証拠)
# C:/temp/lod_view1.png         (緑 placeholder)
# C:/temp/lod_topdown.png       (同上)
# C:/temp/lod_with_stat.png     (黒 + stat オーバーレイ)
# C:/Users/owner/Documents/Unreal Projects/VoxelWorldHost/Saved/Screenshots/WindowsEditor/HighresScreenshot00001.png (HighResShot 経路は正常)

# 4. 既存 PIE command 実装を確認
# C:/Users/owner/Documents/Unreal Projects/spirrow-unrealwise/MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgePIECommands.cpp

# 5. 修正案 1 (FlushRenderingCommands) で実装 → ビルド → 検証シナリオ A 確認
# 6. 案 2 が必要なら追加実装
# 7. B/C 別 issue として処理
```
