# USceneCaptureComponent2D + ProceduralMeshComponent Fully Shaded Capture Analysis

## 調査概要

UE 5.7 において、`USceneCaptureComponent2D` で `ProceduralMeshComponent` を fully shaded（材質 + ライティング + 大気）でキャプチャする際、wireframe + 単色緑で出力される問題の根本原因と解決策を調査しました。

---

## 1. USceneCaptureComponent のデフォルト初期設定

### コンストラクタでの初期化 (`SceneCaptureComponent.cpp:167-199`)

```cpp
USceneCaptureComponent::USceneCaptureComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), ShowFlags(FEngineShowFlags(ESFIM_Game))
{
    CaptureSource = SCS_SceneColorHDR;  // デフォルトは HDR 出力
    PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    
    // 明示的に無効化されるフラグ
    ShowFlags.SetMotionBlur(0);
    ShowFlags.SetSeparateTranslucency(0);
    ShowFlags.SetHMDDistortion(0);
    ShowFlags.SetOnScreenDebug(0);
    
    // ⚠️ Materials/Wireframe/Lighting は明示変更なし
}
```

**重要 findings:**
- `ShowFlags` は `ESFIM_Game` モードで初期化（`ESFIM_SceneCapture` は存在しない）
- `CaptureSource` デフォルト = `SCS_SceneColorHDR`
- **Materials/Wireframe フラグはコンストラクタで変更されていない**

---

## 2. FEngineShowFlags::Init() の実装とデフォルト値

### 初期化フロー (`ShowFlags.h:381-539`)

```cpp
void FEngineShowFlags::Init(EShowFlagInitMode InitMode)
{
    if (InitMode == ESFIM_All0)
    {
        FMemory::Memset(this, 0x00, STRUCT_OFFSET(FEngineShowFlags, CustomShowFlags));
        return;
    }

    // ⚠️ 大多数のフラグは ON (0xff/true) で初期化
    FMemory::Memset(this, uint8(true), STRUCT_OFFSET(FEngineShowFlags, CustomShowFlags));
    InitCustomShowFlags(InitMode);
    
    // その後、特定フラグを明示的に OFF
    SetWireframe(false);      // Line 439: Wireframe はデフォルト OFF
    SetMotionBlur(false);     // Line 445
    // ...
}
```

### デフォルトフラグ値テーブル

| フラグ | デフォルト値 | 根拠 |
|--------|-------------|------|
| **Materials** | `true` (ON) | `memset 0xff` + 明示的 OFF なし |
| **Lighting** | `true` (ON) | `memset 0xff` + 明示的 OFF なし |
| **Wireframe** | `false` (OFF) | Line 439: `SetWireframe(false);` |
| **Fog** | `true` (ON) | `memset 0xff` + 明示的 OFF なし |
| **Atmosphere** | `true` (ON) | `memset 0xff` + 明示的 OFF なし |

**結論:** `ESFIM_Game` 初期化では、**Materials/Lighting は ON、Wireframe は OFF** が期待される。

---

## 3. EShowFlagInitMode 利用可能なモード

調査したコードベースで確認されたモード:

| モード | 用途 | SceneCapture での使用 |
|--------|------|---------------------|
| `ESFIM_All0` | 全フラグ OFF | ❌ |
| `ESFIM_Game` | ゲーム/ランタイムビューポート | ✅ デフォルト |
| `ESFIM_Editor` | エディタビューポート | ❌ |
| `ESFIM_VREditing` | VR 編集モード | ❌ |
| `ESFIM_SceneCapture` | **存在しない** | N/A |

**重要:** `ESFIM_SceneCapture` という専用モードは**存在しない**。SceneCapture は `ESFIM_Game` を使用。

---

## 4. Wireframe + 単色緑 症状の既知原因

### 原因 1: ShowFlags.Wireframe が真に設定されている
```cpp
// 意図せず Wireframe が有効化されている可能性
ShowFlags.SetWireframe(true);  // ❌ これだとワイヤーフレームのみ
```

### 原因 2: ViewMode のオーバーライド (`ShowFlags.h:381-539`)
```cpp
// ApplyViewMode() が ShowFlags を上書きする可能性
// VMI_Wireframe や VMI_Unlit が設定されている場合
```

### 原因 3: PrimitiveRenderMode の設定
```cpp
// PRM_RenderScenePrimitives (デフォルト) はメッシュ描画を含む
// PRM_UsePrimitivesFromScene (変更時) はシーンから取得
```

### 原因 4: ProceduralMeshComponent の SceneProxy フラグ
`FProceduralMeshSceneProxy` が SceneCapture の ViewFamily で:
- `bRenderInMainPass = false` になっている
- `GetViewRelevance()` でフィルタリングされている

---

## 5. ESceneCaptureSource の挙動

### 利用可能な値 (`Scene.h`)

| 値 | 説明 | Fully Shaded への適性 |
|----|------|---------------------|
| `SCS_SceneColorHDR` | HDR シーンカラー | ✅ デフォルト推奨 |
| `SCS_FinalColorLDR` | PostProcess 適用後 LDR | ✅ 最終出力に近い |
| `SCS_SceneColorSceneDepth` | シーンカラー + デプス | ⚠️ デプス情報含む |
| `SCS_Normal` | 法線マップ | ❌ |
| `SCS_BaseColor` | ベースカラー | ❌ |

**推奨:** `SCS_FinalColorLDR` または `SCS_SceneColorHDR`

---

## 6. 最小設定セット サンプルコード

### SpirrowBridge プラグイン用実装例

```cpp
// SpirrowBridgeCommands.cpp
void USpirrowBridgeFunctionLibrary::TakePiePovScreenshot()
{
    UWorld* World = GEngine->GetPlayInEditorWorld();
    if (!World) return;
    
    // 1. ASceneCapture2D を spawn
    ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>(
        ASceneCapture2D::StaticClass(),
        PawnTransform,  // PIE Pawn の transform で teleport
        FActorSpawnParameters()
    );
    
    // 2. USceneCaptureComponent2D 設定
    USceneCaptureComponent2D* CaptureComp = CaptureActor->GetCaptureComponent2D();
    
    // --- 必須設定 ---
    CaptureComp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;  // PostProcess 適用
    CaptureComp->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    
    // --- ShowFlags 明示設定 (デフォルト値の確認と上書き) ---
    FEngineShowFlags& ShowFlags = CaptureComp->ShowFlags;
    ShowFlags.SetMaterials(true);           // 材質 ON
    ShowFlags.SetLighting(true);            // ライティング ON
    ShowFlags.SetWireframe(false);          // ワイヤーフレーム OFF
    ShowFlags.SetFog(true);                 // 霧 ON
    ShowFlags.SetAtmosphere(true);          // 大気 ON
    ShowFlags.SetMotionBlur(false);         // モーションブラー OFF (SceneCapture 推奨)
    ShowFlags.SetSeparateTranslucency(false);
    
    // --- TextureTarget 設定 ---
    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->Init(1920, 1080, ETextureRenderTargetFormat::TF_RGBA8);
    RenderTarget->UpdateResourceImmediate(true);
    CaptureComp->TextureTarget = RenderTarget;
    
    // 3. Capture 実行
    CaptureComp->CaptureScene();
    
    // 4. PNG 出力 (RenderTarget から読み出し)
    FRenderTarget* RT = RenderTarget->GameThread_GetRenderTargetResource();
    TArray<FColor> Pixels;
    RT->ReadPixels(Pixels);
    
    // ... PNG 保存処理
}
```

### 設定チェックリスト

| 設定項目 | 推奨値 | 確認方法 |
|---------|--------|---------|
| `CaptureSource` | `SCS_FinalColorLDR` または `SCS_SceneColorHDR` | コンストラクタ後確認 |
| `PrimitiveRenderMode` | `PRM_RenderScenePrimitives` | デフォルト値 |
| `ShowFlags.Materials` | `true` | 明示設定推奨 |
| `ShowFlags.Lighting` | `true` | 明示設定推奨 |
| `ShowFlags.Wireframe` | `false` | **最重要** |
| `ShowFlags.Fog` | `true` | 大気効果含む場合 |
| `ShowFlags.Atmosphere` | `true` | 大気効果含む場合 |

---

## 7. トラブルシューティングパス

### Wireframe + 緑 が続く場合の調査手順

1. **ShowFlags 値のダンプ**
   ```cpp
   UE_LOG(LogTemp, Log, TEXT("Wireframe=%d, Materials=%d, Lighting=%d"),
       CaptureComp->ShowFlags.IsWireframe(),
       CaptureComp->ShowFlags.IsMaterials(),
       CaptureComp->ShowFlags.IsLighting());
   ```

2. **ViewMode の確認**
   ```cpp
   // FSceneView::ApplyViewMode() が ShowFlags を上書きしていないか
   ```

3. **ProceduralMeshComponent の SceneProxy**
   - `FProceduralMeshSceneProxy::GetViewRelevance()` で `bDrawRelevance` が true か
   - `bRenderInMainPass` フラグの確認

4. **マテリアルの有効性**
   - ProceduralMesh のセクションに有効なマテリアルが割り当てられているか
   - マテリアルが `UsedWithSceneCapture = true` になっているか（一部ケース）

---

## 8. 結論と推奨

### 根本原因の特定

デフォルト設定 (`ESFIM_Game`) では Materials/Lighting は ON、Wireframe は OFF のはずですが、以下のいずれかが原因で wireframe+緑 出力が発生:

1. **ShowFlags.Wireframe が真に設定されている**（最も可能性が高い）
2. **ViewMode が Wireframe/Unlit を強制している**
3. **ProceduralMeshComponent の SceneProxy が SceneCapture ViewFamily でフィルタリングされている**

### 推奨対応

1. **ShowFlags を明示設定**（デフォルト値に依存しない）
2. **CaptureSource を `SCS_FinalColorLDR` に設定**（PostProcess 効果を含むため）
3. **ProceduralMeshComponent のマテリアル設定を確認**

上記「最小設定セット」サンプルコードを `take_pie_pov_screenshot` コマンド実装に適用することで、ProceduralMesh が fully shaded でキャプチャされるはずです。

---

## 参照ファイル

| ファイルパス | 内容 |
|-------------|------|
| `Engine/Source/Runtime/Engine/Private/SceneCaptureComponent.cpp:167-199` | USceneCaptureComponent コンストラクタ |
| `Engine/Source/Runtime/Engine/Public/ShowFlags.h:381-539` | FEngineShowFlags::Init() 実装 |
| `Engine/Source/Runtime/Engine/Public/ShowFlags.h:186-190` | FEngineShowFlags デフォルトコンストラクタ |
| `Engine/Classes/Engine/Scene.h` | ESceneCaptureSource 定義 |
| `Engine/Plugins/Runtime/ProceduralMeshComponent/` | ProceduralMeshComponent 実装 |