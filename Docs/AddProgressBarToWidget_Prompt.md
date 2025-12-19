# SpirrowUnrealWise 拡張: add_progressbar_to_widget

## 概要

UMGウィジェットにProgressBarを追加するツールを実装する。

## 実装ファイル

1. `Python/tools/umg_tools.py` - Python側ツール関数
2. `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeUMGCommands.h` - 関数宣言
3. `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeUMGCommands.cpp` - 実装

## 仕様

### パラメータ

| パラメータ | 型 | 必須 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| widget_name | str | ✅ | - | Widget Blueprint名 |
| progressbar_name | str | ✅ | - | ProgressBar名 |
| percent | float | | 0.0 | 初期値（0.0〜1.0） |
| fill_color | [R,G,B,A] | | [0.0, 0.5, 1.0, 1.0] | バー色（青系） |
| background_color | [R,G,B,A] | | [0.1, 0.1, 0.1, 1.0] | 背景色 |
| size | [W,H] | | [200, 20] | サイズ |
| anchor | str | | "Center" | アンカー位置 |
| alignment | [X,Y] | | [0.5, 0.5] | アライメント |
| path | str | | "/Game/UI" | Widgetパス |

### 使用例

```python
add_progressbar_to_widget(
    widget_name="WBP_DisarmProgress",
    progressbar_name="DisarmBar",
    percent=0.0,
    fill_color=[0.0, 1.0, 0.0, 1.0],  # 緑
    size=[150, 15],
    anchor="Center",
    path="/Game/TrapxTrap/UI"
)
```

## 実装内容

### 1. Python側 (`umg_tools.py`)

`add_image_to_widget` の後に追加:

```python
@mcp.tool()
def add_progressbar_to_widget(
    ctx: Context,
    widget_name: str,
    progressbar_name: str,
    percent: float = 0.0,
    fill_color: List[float] = [0.0, 0.5, 1.0, 1.0],
    background_color: List[float] = [0.1, 0.1, 0.1, 1.0],
    size: List[float] = [200.0, 20.0],
    anchor: str = "Center",
    alignment: List[float] = [0.5, 0.5],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a ProgressBar widget to a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        progressbar_name: Name for the new ProgressBar
        percent: Initial fill percentage 0.0-1.0 (default: 0.0)
        fill_color: [R, G, B, A] bar color values 0.0-1.0 (default: blue)
        background_color: [R, G, B, A] background color values 0.0-1.0 (default: dark gray)
        size: [Width, Height] size in pixels (default: [200, 20])
        anchor: Anchor position - "Center", "TopLeft", etc. (default: "Center")
        alignment: [X, Y] alignment values 0.0-1.0 (default: [0.5, 0.5])
        path: Content browser path to the widget (default: "/Game/UI")

    Returns:
        Dict containing success status and progressbar properties

    Example:
        add_progressbar_to_widget(
            widget_name="WBP_DisarmProgress",
            progressbar_name="DisarmBar",
            percent=0.0,
            fill_color=[0.0, 1.0, 0.0, 1.0],
            size=[150, 15],
            path="/Game/TrapxTrap/UI"
        )
    """
    from unreal_mcp_server import get_unreal_connection

    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.error("Failed to connect to Unreal Engine")
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        params = {
            "widget_name": widget_name,
            "progressbar_name": progressbar_name,
            "percent": percent,
            "fill_color": fill_color,
            "background_color": background_color,
            "size": size,
            "anchor": anchor,
            "alignment": alignment,
            "path": path
        }

        logger.info(f"Adding ProgressBar to widget with params: {params}")
        response = unreal.send_command("add_progressbar_to_widget", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Add ProgressBar to widget response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error adding ProgressBar to widget: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### 2. ヘッダー (`SpirrowBridgeUMGCommands.h`)

`HandleAddImageToWidget` の後に追加:

```cpp
/**
 * Add a ProgressBar widget to a UMG Widget Blueprint
 * @param Params - Must include:
 *                "widget_name" - Name of the Widget Blueprint
 *                "progressbar_name" - Name for the new ProgressBar
 *                "percent" - Initial fill percentage 0.0-1.0 (optional, default: 0.0)
 *                "fill_color" - [R, G, B, A] bar color (optional, default: blue)
 *                "background_color" - [R, G, B, A] background color (optional)
 *                "size" - [Width, Height] size (optional, default: [200, 20])
 *                "anchor" - Anchor position (optional, default: "Center")
 *                "alignment" - [X, Y] alignment (optional, default: [0.5, 0.5])
 *                "path" - Widget path (optional, default: "/Game/UI")
 * @return JSON response with the added widget details
 */
TSharedPtr<FJsonObject> HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params);
```

### 3. 実装 (`SpirrowBridgeUMGCommands.cpp`)

#### HandleCommand に追加

```cpp
else if (CommandName == TEXT("add_progressbar_to_widget"))
{
    return HandleAddProgressBarToWidget(Params);
}
```

#### インクルード追加

```cpp
#include "Components/ProgressBar.h"
```

#### HandleAddProgressBarToWidget 実装

`HandleAddImageToWidget` の後に追加:

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString ProgressBarName;
    if (!Params->TryGetStringField(TEXT("progressbar_name"), ProgressBarName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'progressbar_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    float Percent = 0.0f;
    if (Params->HasField(TEXT("percent")))
    {
        Percent = Params->GetNumberField(TEXT("percent"));
    }

    // Get size [Width, Height]
    FVector2D Size(200.0f, 20.0f);
    if (Params->HasField(TEXT("size")))
    {
        const TArray<TSharedPtr<FJsonValue>>* SizeArray;
        if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
        {
            Size.X = (*SizeArray)[0]->AsNumber();
            Size.Y = (*SizeArray)[1]->AsNumber();
        }
    }

    // Get fill color [R, G, B, A]
    FLinearColor FillColor(0.0f, 0.5f, 1.0f, 1.0f);  // Default blue
    if (Params->HasField(TEXT("fill_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("fill_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            FillColor.R = (*ColorArray)[0]->AsNumber();
            FillColor.G = (*ColorArray)[1]->AsNumber();
            FillColor.B = (*ColorArray)[2]->AsNumber();
            FillColor.A = (*ColorArray)[3]->AsNumber();
        }
    }

    // Get background color [R, G, B, A]
    FLinearColor BackgroundColor(0.1f, 0.1f, 0.1f, 1.0f);  // Default dark gray
    if (Params->HasField(TEXT("background_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("background_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            BackgroundColor.R = (*ColorArray)[0]->AsNumber();
            BackgroundColor.G = (*ColorArray)[1]->AsNumber();
            BackgroundColor.B = (*ColorArray)[2]->AsNumber();
            BackgroundColor.A = (*ColorArray)[3]->AsNumber();
        }
    }

    // Get alignment [X, Y]
    FVector2D Alignment(0.5f, 0.5f);
    if (Params->HasField(TEXT("alignment")))
    {
        const TArray<TSharedPtr<FJsonValue>>* AlignmentArray;
        if (Params->TryGetArrayField(TEXT("alignment"), AlignmentArray) && AlignmentArray->Num() >= 2)
        {
            Alignment.X = (*AlignmentArray)[0]->AsNumber();
            Alignment.Y = (*AlignmentArray)[1]->AsNumber();
        }
    }

    // Get anchor
    FString AnchorStr = TEXT("Center");
    Params->TryGetStringField(TEXT("anchor"), AnchorStr);
    FAnchors Anchors(0.5f, 0.5f, 0.5f, 0.5f);  // Center default

    // Parse anchor presets
    if (AnchorStr == TEXT("TopLeft"))
    {
        Anchors = FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else if (AnchorStr == TEXT("TopCenter"))
    {
        Anchors = FAnchors(0.5f, 0.0f, 0.5f, 0.0f);
    }
    else if (AnchorStr == TEXT("TopRight"))
    {
        Anchors = FAnchors(1.0f, 0.0f, 1.0f, 0.0f);
    }
    else if (AnchorStr == TEXT("MiddleLeft"))
    {
        Anchors = FAnchors(0.0f, 0.5f, 0.0f, 0.5f);
    }
    else if (AnchorStr == TEXT("Center"))
    {
        Anchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
    }
    else if (AnchorStr == TEXT("MiddleRight"))
    {
        Anchors = FAnchors(1.0f, 0.5f, 1.0f, 0.5f);
    }
    else if (AnchorStr == TEXT("BottomLeft"))
    {
        Anchors = FAnchors(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else if (AnchorStr == TEXT("BottomCenter"))
    {
        Anchors = FAnchors(0.5f, 1.0f, 0.5f, 1.0f);
    }
    else if (AnchorStr == TEXT("BottomRight"))
    {
        Anchors = FAnchors(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    // Get WidgetTree
    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("WidgetTree not found"));
    }

    // Get or create root Canvas Panel
    UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!RootCanvas)
    {
        // Create Canvas Panel if it doesn't exist
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }

    // Create ProgressBar widget
    UProgressBar* ProgressBarWidget = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(*ProgressBarName));
    if (!ProgressBarWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create ProgressBar widget"));
    }

    // Set percent
    ProgressBarWidget->SetPercent(Percent);

    // Set fill color
    ProgressBarWidget->SetFillColorAndOpacity(FillColor);

    // Set background color via WidgetStyle
    FProgressBarStyle Style = ProgressBarWidget->GetWidgetStyle();
    Style.BackgroundImage.TintColor = FSlateColor(BackgroundColor);
    ProgressBarWidget->SetWidgetStyle(Style);

    // Add to Canvas Panel
    UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ProgressBarWidget);
    if (Slot)
    {
        Slot->SetAnchors(Anchors);
        Slot->SetAlignment(Alignment);
        Slot->SetPosition(FVector2D(0, 0));
        Slot->SetSize(Size);
    }

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("widget"), WidgetName);
    ResultObj->SetStringField(TEXT("progressbar_name"), ProgressBarName);
    ResultObj->SetNumberField(TEXT("percent"), Percent);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}
```

## テスト

実装後、MCPサーバーを再起動して以下をテスト:

```python
add_progressbar_to_widget(
    widget_name="WBP_DisarmProgress",
    progressbar_name="DisarmBar",
    percent=0.5,
    fill_color=[0.0, 1.0, 0.0, 1.0],
    size=[150, 15],
    path="/Game/TrapxTrap/UI"
)
```

## 注意事項

- Unrealプラグインのビルドが必要（SpirrowBridge）
- MCPサーバー再起動が必要
- Widget Blueprint が既に存在している必要がある
