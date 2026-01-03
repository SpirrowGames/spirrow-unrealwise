# UMG Phase 4-A: インタラクティブ Widget 実装

## 概要

Phase 4-A では、UI のインタラクティブな要素を MCP から構築可能にする。

**実装対象:**
1. `add_button_to_widget` - 新 API 準拠の Button 追加
2. `bind_widget_component_event` - Widget コンポーネントのイベントバインディング
3. `add_slider_to_widget` - Slider Widget 追加
4. `add_checkbox_to_widget` - CheckBox Widget 追加

---

## 1. add_button_to_widget

### 仕様

新 API 準拠（path 対応、9ポジションアンカー対応）の Button 追加ツール。

**パラメータ:**

| パラメータ | 型 | 必須 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| `widget_name` | string | ✅ | - | Widget Blueprint 名 |
| `button_name` | string | ✅ | - | 新規ボタンの名前 |
| `text` | string | - | "" | ボタン内のテキスト |
| `font_size` | int | - | 14 | フォントサイズ |
| `text_color` | [R,G,B,A] | - | [1,1,1,1] | テキスト色 |
| `normal_color` | [R,G,B,A] | - | [0.1,0.1,0.1,1] | 通常時の背景色 |
| `hovered_color` | [R,G,B,A] | - | [0.2,0.2,0.2,1] | ホバー時の背景色 |
| `pressed_color` | [R,G,B,A] | - | [0.05,0.05,0.05,1] | 押下時の背景色 |
| `size` | [W,H] | - | [200,50] | サイズ |
| `anchor` | string | - | "Center" | 9ポジションアンカー |
| `alignment` | [X,Y] | - | [0.5,0.5] | アライメント |
| `path` | string | - | "/Game/UI" | パス |

### Python 実装 (`tools/umg_tools.py` に追加)

```python
@mcp.tool()
def add_button_to_widget(
    ctx: Context,
    widget_name: str,
    button_name: str,
    text: str = "",
    font_size: int = 14,
    text_color: List[float] = [1.0, 1.0, 1.0, 1.0],
    normal_color: List[float] = [0.1, 0.1, 0.1, 1.0],
    hovered_color: List[float] = [0.2, 0.2, 0.2, 1.0],
    pressed_color: List[float] = [0.05, 0.05, 0.05, 1.0],
    size: List[float] = [200.0, 50.0],
    anchor: str = "Center",
    alignment: List[float] = [0.5, 0.5],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a Button widget to a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        button_name: Name for the new Button
        text: Text to display on the button
        font_size: Font size in points (default: 14)
        text_color: [R, G, B, A] text color values 0.0-1.0 (default: white)
        normal_color: [R, G, B, A] normal state background color (default: dark gray)
        hovered_color: [R, G, B, A] hovered state background color
        pressed_color: [R, G, B, A] pressed state background color
        size: [Width, Height] size in pixels (default: [200, 50])
        anchor: Anchor position - "Center", "TopLeft", "TopCenter", "TopRight",
                "MiddleLeft", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight"
        alignment: [X, Y] alignment values 0.0-1.0 (default: [0.5, 0.5])
        path: Content browser path to the widget (default: "/Game/UI")

    Returns:
        Dict containing success status and button properties

    Example:
        add_button_to_widget(
            widget_name="WBP_MainMenu",
            button_name="StartButton",
            text="Start Game",
            font_size=18,
            size=[250, 60],
            anchor="Center",
            path="/Game/UI"
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
            "button_name": button_name,
            "text": text,
            "font_size": font_size,
            "text_color": text_color,
            "normal_color": normal_color,
            "hovered_color": hovered_color,
            "pressed_color": pressed_color,
            "size": size,
            "anchor": anchor,
            "alignment": alignment,
            "path": path
        }

        logger.info(f"Adding Button to widget with params: {params}")
        response = unreal.send_command("add_button_to_widget_v2", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Add Button to widget response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error adding Button to widget: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 実装

#### SpirrowBridgeUMGCommands.h に追加

```cpp
// Phase 4-A: Interactive Widgets
TSharedPtr<FJsonObject> HandleAddButtonToWidgetV2(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("add_button_to_widget_v2"))
{
    return HandleAddButtonToWidgetV2(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddButtonToWidgetV2(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString ButtonName;
    if (!Params->TryGetStringField(TEXT("button_name"), ButtonName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'button_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    FString Text;
    Params->TryGetStringField(TEXT("text"), Text);

    int32 FontSize = 14;
    if (Params->HasField(TEXT("font_size")))
    {
        FontSize = Params->GetIntegerField(TEXT("font_size"));
    }

    // Get size [Width, Height]
    FVector2D Size(200.0f, 50.0f);
    if (Params->HasField(TEXT("size")))
    {
        const TArray<TSharedPtr<FJsonValue>>* SizeArray;
        if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
        {
            Size.X = (*SizeArray)[0]->AsNumber();
            Size.Y = (*SizeArray)[1]->AsNumber();
        }
    }

    // Get text color [R, G, B, A]
    FLinearColor TextColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (Params->HasField(TEXT("text_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("text_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            TextColor.R = (*ColorArray)[0]->AsNumber();
            TextColor.G = (*ColorArray)[1]->AsNumber();
            TextColor.B = (*ColorArray)[2]->AsNumber();
            TextColor.A = (*ColorArray)[3]->AsNumber();
        }
    }

    // Get normal color
    FLinearColor NormalColor(0.1f, 0.1f, 0.1f, 1.0f);
    if (Params->HasField(TEXT("normal_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("normal_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            NormalColor.R = (*ColorArray)[0]->AsNumber();
            NormalColor.G = (*ColorArray)[1]->AsNumber();
            NormalColor.B = (*ColorArray)[2]->AsNumber();
            NormalColor.A = (*ColorArray)[3]->AsNumber();
        }
    }

    // Get hovered color
    FLinearColor HoveredColor(0.2f, 0.2f, 0.2f, 1.0f);
    if (Params->HasField(TEXT("hovered_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("hovered_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            HoveredColor.R = (*ColorArray)[0]->AsNumber();
            HoveredColor.G = (*ColorArray)[1]->AsNumber();
            HoveredColor.B = (*ColorArray)[2]->AsNumber();
            HoveredColor.A = (*ColorArray)[3]->AsNumber();
        }
    }

    // Get pressed color
    FLinearColor PressedColor(0.05f, 0.05f, 0.05f, 1.0f);
    if (Params->HasField(TEXT("pressed_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("pressed_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            PressedColor.R = (*ColorArray)[0]->AsNumber();
            PressedColor.G = (*ColorArray)[1]->AsNumber();
            PressedColor.B = (*ColorArray)[2]->AsNumber();
            PressedColor.A = (*ColorArray)[3]->AsNumber();
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
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
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
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }

    // Create Button widget
    UButton* ButtonWidget = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*ButtonName));
    if (!ButtonWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Button widget"));
    }

    // Set button style colors
    FButtonStyle ButtonStyle = ButtonWidget->GetStyle();
    ButtonStyle.Normal.TintColor = FSlateColor(NormalColor);
    ButtonStyle.Hovered.TintColor = FSlateColor(HoveredColor);
    ButtonStyle.Pressed.TintColor = FSlateColor(PressedColor);
    ButtonWidget->SetStyle(ButtonStyle);

    // Create TextBlock for button text if text is provided
    if (!Text.IsEmpty())
    {
        FString TextBlockName = ButtonName + TEXT("_Text");
        UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*TextBlockName));
        if (TextBlock)
        {
            TextBlock->SetText(FText::FromString(Text));
            TextBlock->SetColorAndOpacity(FSlateColor(TextColor));
            
            FSlateFontInfo FontInfo = TextBlock->GetFont();
            FontInfo.Size = FontSize;
            TextBlock->SetFont(FontInfo);

            // Add TextBlock as child of Button
            ButtonWidget->AddChild(TextBlock);
        }
    }

    // Add to Canvas Panel
    UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ButtonWidget);
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
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("button_name"), ButtonName);
    ResultObj->SetStringField(TEXT("text"), Text);
    return ResultObj;
}
```

---

## 2. bind_widget_component_event

### 仕様

Widget コンポーネント（Button 等）のイベント（OnClicked, OnHovered 等）を Blueprint 関数にバインドする。

**パラメータ:**

| パラメータ | 型 | 必須 | デフォルト | 説明 |
|-----------|-----|------|-----------|------|
| `widget_name` | string | ✅ | - | Widget Blueprint 名 |
| `component_name` | string | ✅ | - | イベントをバインドするコンポーネント名 |
| `event_type` | string | ✅ | - | イベントタイプ |
| `function_name` | string | - | 自動生成 | バインド先の関数名 |
| `create_function` | bool | - | true | 関数を自動作成するか |
| `path` | string | - | "/Game/UI" | パス |

**対応イベントタイプ:**

| event_type | 対象 | 説明 |
|------------|------|------|
| `OnClicked` | Button | クリック時 |
| `OnPressed` | Button | 押下時 |
| `OnReleased` | Button | 離した時 |
| `OnHovered` | Button | ホバー開始 |
| `OnUnhovered` | Button | ホバー終了 |
| `OnValueChanged` | Slider, CheckBox | 値変更時 |

### Python 実装 (`tools/umg_tools.py` に追加)

```python
@mcp.tool()
def bind_widget_component_event(
    ctx: Context,
    widget_name: str,
    component_name: str,
    event_type: str,
    function_name: str = "",
    create_function: bool = True,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Bind an event on a widget component to a Blueprint function.

    Creates an event binding that calls a function when the specified event fires.
    Optionally creates the target function automatically.

    Args:
        widget_name: Name of the Widget Blueprint
        component_name: Name of the widget component (e.g., "StartButton")
        event_type: Type of event to bind:
            - "OnClicked": Button click event
            - "OnPressed": Button press event
            - "OnReleased": Button release event
            - "OnHovered": Button hover start event
            - "OnUnhovered": Button hover end event
            - "OnValueChanged": Slider/CheckBox value change event
        function_name: Name of the function to bind to (auto-generated if empty)
        create_function: Whether to automatically create the target function (default: True)
        path: Content browser path to the widget (default: "/Game/UI")

    Returns:
        Dict containing success status, function name, and node ID

    Example:
        bind_widget_component_event(
            widget_name="WBP_MainMenu",
            component_name="StartButton",
            event_type="OnClicked",
            function_name="HandleStartButtonClicked",
            path="/Game/UI"
        )
    """
    from unreal_mcp_server import get_unreal_connection

    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.error("Failed to connect to Unreal Engine")
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        # Auto-generate function name if not provided
        if not function_name:
            function_name = f"{component_name}_{event_type}"

        params = {
            "widget_name": widget_name,
            "component_name": component_name,
            "event_type": event_type,
            "function_name": function_name,
            "create_function": create_function,
            "path": path
        }

        logger.info(f"Binding widget component event with params: {params}")
        response = unreal.send_command("bind_widget_component_event", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Bind widget component event response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error binding widget component event: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 実装

#### SpirrowBridgeUMGCommands.h に追加

```cpp
TSharedPtr<FJsonObject> HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("bind_widget_component_event"))
{
    return HandleBindWidgetComponentEvent(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString EventType;
    if (!Params->TryGetStringField(TEXT("event_type"), EventType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'event_type' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        FunctionName = ComponentName + TEXT("_") + EventType;
    }

    bool bCreateFunction = true;
    Params->TryGetBoolField(TEXT("create_function"), bCreateFunction);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    // Find widget component
    UWidget* WidgetComponent = WidgetBP->WidgetTree->FindWidget(FName(*ComponentName));
    if (!WidgetComponent)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget component '%s' not found"), *ComponentName));
    }

    // Validate event type for component
    UButton* ButtonWidget = Cast<UButton>(WidgetComponent);
    USlider* SliderWidget = Cast<USlider>(WidgetComponent);
    UCheckBox* CheckBoxWidget = Cast<UCheckBox>(WidgetComponent);

    bool bValidEvent = false;
    FName EventPropertyName;

    if (ButtonWidget)
    {
        if (EventType == TEXT("OnClicked"))
        {
            EventPropertyName = FName("OnClicked");
            bValidEvent = true;
        }
        else if (EventType == TEXT("OnPressed"))
        {
            EventPropertyName = FName("OnPressed");
            bValidEvent = true;
        }
        else if (EventType == TEXT("OnReleased"))
        {
            EventPropertyName = FName("OnReleased");
            bValidEvent = true;
        }
        else if (EventType == TEXT("OnHovered"))
        {
            EventPropertyName = FName("OnHovered");
            bValidEvent = true;
        }
        else if (EventType == TEXT("OnUnhovered"))
        {
            EventPropertyName = FName("OnUnhovered");
            bValidEvent = true;
        }
    }
    else if (SliderWidget || CheckBoxWidget)
    {
        if (EventType == TEXT("OnValueChanged"))
        {
            EventPropertyName = FName("OnValueChanged");
            bValidEvent = true;
        }
    }

    if (!bValidEvent)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Invalid event type '%s' for component type"), *EventType));
    }

    // Find or create Event Graph
    UEdGraph* EventGraph = nullptr;
    for (UEdGraph* Graph : WidgetBP->UbergraphPages)
    {
        if (Graph->GetFName() == TEXT("EventGraph"))
        {
            EventGraph = Graph;
            break;
        }
    }

    if (!EventGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Event Graph not found"));
    }

    // Create the function if requested
    UEdGraph* FuncGraph = nullptr;
    if (bCreateFunction)
    {
        // Check if function already exists
        for (UEdGraph* Graph : WidgetBP->FunctionGraphs)
        {
            if (Graph->GetFName() == FName(*FunctionName))
            {
                FuncGraph = Graph;
                break;
            }
        }

        if (!FuncGraph)
        {
            // Create new function graph
            FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
                WidgetBP,
                FName(*FunctionName),
                UEdGraph::StaticClass(),
                UEdGraphSchema_K2::StaticClass()
            );

            if (FuncGraph)
            {
                FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);
            }
        }
    }

    // Create event binding node
    // Find the multicast delegate property on the widget
    FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
        WidgetComponent->GetClass(), EventPropertyName);

    if (!DelegateProp)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Event property '%s' not found on component"), *EventPropertyName.ToString()));
    }

    // Calculate position for the node
    float MaxY = 0.0f;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        MaxY = FMath::Max(MaxY, (float)Node->NodePosY);
    }
    FVector2D NodePos(200, MaxY + 200);

    // Create the event dispatcher node using component reference
    // This creates an event node that fires when the delegate is broadcast
    UK2Node_ComponentBoundEvent* EventNode = NewObject<UK2Node_ComponentBoundEvent>(EventGraph);
    EventNode->InitializeComponentBoundEventParams(
        Cast<FObjectProperty>(WidgetComponent->GetClass()->FindPropertyByName(TEXT("Slot"))),
        DelegateProp
    );
    
    EventGraph->AddNode(EventNode, false, false);
    EventNode->NodePosX = NodePos.X;
    EventNode->NodePosY = NodePos.Y;
    EventNode->AllocateDefaultPins();

    // If we have a function, connect the event to call it
    if (FuncGraph)
    {
        // Create call function node
        UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(EventGraph);
        CallFuncNode->SetFromFunction(WidgetBP->GeneratedClass->FindFunctionByName(FName(*FunctionName)));
        EventGraph->AddNode(CallFuncNode, false, false);
        CallFuncNode->NodePosX = NodePos.X + 300;
        CallFuncNode->NodePosY = NodePos.Y;
        CallFuncNode->AllocateDefaultPins();

        // Connect exec pins
        UEdGraphPin* EventExecPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
        UEdGraphPin* FuncExecPin = CallFuncNode->FindPin(UEdGraphSchema_K2::PN_Execute);
        if (EventExecPin && FuncExecPin)
        {
            EventExecPin->MakeLinkTo(FuncExecPin);
        }
    }

    // Mark Blueprint as modified and compile
    FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("component_name"), ComponentName);
    ResultObj->SetStringField(TEXT("event_type"), EventType);
    ResultObj->SetStringField(TEXT("function_name"), FunctionName);
    ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
    return ResultObj;
}
```

**注意:** `UK2Node_ComponentBoundEvent` の使用には追加インクルードが必要:

```cpp
#include "K2Node_ComponentBoundEvent.h"
```

---

## 3. add_slider_to_widget

### 仕様

Slider Widget を追加するツール。

### Python 実装 (`tools/umg_tools.py` に追加)

```python
@mcp.tool()
def add_slider_to_widget(
    ctx: Context,
    widget_name: str,
    slider_name: str,
    value: float = 0.0,
    min_value: float = 0.0,
    max_value: float = 1.0,
    step_size: float = 0.0,
    orientation: str = "Horizontal",
    bar_color: List[float] = [0.2, 0.2, 0.2, 1.0],
    handle_color: List[float] = [1.0, 1.0, 1.0, 1.0],
    size: List[float] = [200.0, 20.0],
    anchor: str = "Center",
    alignment: List[float] = [0.5, 0.5],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a Slider widget to a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        slider_name: Name for the new Slider
        value: Initial value (0.0-1.0, default: 0.0)
        min_value: Minimum value (default: 0.0)
        max_value: Maximum value (default: 1.0)
        step_size: Step size for discrete values, 0 for continuous (default: 0.0)
        orientation: "Horizontal" or "Vertical" (default: "Horizontal")
        bar_color: [R, G, B, A] slider bar color (default: dark gray)
        handle_color: [R, G, B, A] slider handle color (default: white)
        size: [Width, Height] size in pixels (default: [200, 20])
        anchor: Anchor position (default: "Center")
        alignment: [X, Y] alignment values (default: [0.5, 0.5])
        path: Content browser path to the widget (default: "/Game/UI")

    Returns:
        Dict containing success status and slider properties

    Example:
        add_slider_to_widget(
            widget_name="WBP_Settings",
            slider_name="VolumeSlider",
            value=0.5,
            min_value=0.0,
            max_value=1.0,
            size=[300, 25],
            path="/Game/UI"
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
            "slider_name": slider_name,
            "value": value,
            "min_value": min_value,
            "max_value": max_value,
            "step_size": step_size,
            "orientation": orientation,
            "bar_color": bar_color,
            "handle_color": handle_color,
            "size": size,
            "anchor": anchor,
            "alignment": alignment,
            "path": path
        }

        logger.info(f"Adding Slider to widget with params: {params}")
        response = unreal.send_command("add_slider_to_widget", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Add Slider to widget response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error adding Slider to widget: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 実装

#### SpirrowBridgeUMGCommands.h に追加

```cpp
TSharedPtr<FJsonObject> HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("add_slider_to_widget"))
{
    return HandleAddSliderToWidget(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString SliderName;
    if (!Params->TryGetStringField(TEXT("slider_name"), SliderName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'slider_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    float Value = 0.0f;
    if (Params->HasField(TEXT("value")))
    {
        Value = Params->GetNumberField(TEXT("value"));
    }

    float MinValue = 0.0f;
    if (Params->HasField(TEXT("min_value")))
    {
        MinValue = Params->GetNumberField(TEXT("min_value"));
    }

    float MaxValue = 1.0f;
    if (Params->HasField(TEXT("max_value")))
    {
        MaxValue = Params->GetNumberField(TEXT("max_value"));
    }

    float StepSize = 0.0f;
    if (Params->HasField(TEXT("step_size")))
    {
        StepSize = Params->GetNumberField(TEXT("step_size"));
    }

    FString OrientationStr = TEXT("Horizontal");
    Params->TryGetStringField(TEXT("orientation"), OrientationStr);
    EOrientation Orientation = (OrientationStr == TEXT("Vertical")) ? Orient_Vertical : Orient_Horizontal;

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

    // Get bar color [R, G, B, A]
    FLinearColor BarColor(0.2f, 0.2f, 0.2f, 1.0f);
    if (Params->HasField(TEXT("bar_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("bar_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            BarColor.R = (*ColorArray)[0]->AsNumber();
            BarColor.G = (*ColorArray)[1]->AsNumber();
            BarColor.B = (*ColorArray)[2]->AsNumber();
            BarColor.A = (*ColorArray)[3]->AsNumber();
        }
    }

    // Get handle color [R, G, B, A]
    FLinearColor HandleColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (Params->HasField(TEXT("handle_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("handle_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            HandleColor.R = (*ColorArray)[0]->AsNumber();
            HandleColor.G = (*ColorArray)[1]->AsNumber();
            HandleColor.B = (*ColorArray)[2]->AsNumber();
            HandleColor.A = (*ColorArray)[3]->AsNumber();
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

    // Parse anchor presets (same as other tools)
    if (AnchorStr == TEXT("TopLeft")) Anchors = FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
    else if (AnchorStr == TEXT("TopCenter")) Anchors = FAnchors(0.5f, 0.0f, 0.5f, 0.0f);
    else if (AnchorStr == TEXT("TopRight")) Anchors = FAnchors(1.0f, 0.0f, 1.0f, 0.0f);
    else if (AnchorStr == TEXT("MiddleLeft")) Anchors = FAnchors(0.0f, 0.5f, 0.0f, 0.5f);
    else if (AnchorStr == TEXT("Center")) Anchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
    else if (AnchorStr == TEXT("MiddleRight")) Anchors = FAnchors(1.0f, 0.5f, 1.0f, 0.5f);
    else if (AnchorStr == TEXT("BottomLeft")) Anchors = FAnchors(0.0f, 1.0f, 0.0f, 1.0f);
    else if (AnchorStr == TEXT("BottomCenter")) Anchors = FAnchors(0.5f, 1.0f, 0.5f, 1.0f);
    else if (AnchorStr == TEXT("BottomRight")) Anchors = FAnchors(1.0f, 1.0f, 1.0f, 1.0f);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
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
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }

    // Create Slider widget
    USlider* SliderWidget = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), FName(*SliderName));
    if (!SliderWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Slider widget"));
    }

    // Set slider properties
    SliderWidget->SetValue(Value);
    SliderWidget->SetMinValue(MinValue);
    SliderWidget->SetMaxValue(MaxValue);
    SliderWidget->SetStepSize(StepSize);
    SliderWidget->SetOrientation(Orientation);

    // Set slider style colors
    FSliderStyle SliderStyle = SliderWidget->GetWidgetStyle();
    SliderStyle.NormalBarImage.TintColor = FSlateColor(BarColor);
    SliderStyle.HoveredBarImage.TintColor = FSlateColor(BarColor);
    SliderStyle.DisabledBarImage.TintColor = FSlateColor(BarColor * 0.5f);
    SliderStyle.NormalThumbImage.TintColor = FSlateColor(HandleColor);
    SliderStyle.HoveredThumbImage.TintColor = FSlateColor(HandleColor);
    SliderStyle.DisabledThumbImage.TintColor = FSlateColor(HandleColor * 0.5f);
    SliderWidget->SetWidgetStyle(SliderStyle);

    // Add to Canvas Panel
    UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(SliderWidget);
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
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("slider_name"), SliderName);
    ResultObj->SetNumberField(TEXT("value"), Value);
    ResultObj->SetNumberField(TEXT("min_value"), MinValue);
    ResultObj->SetNumberField(TEXT("max_value"), MaxValue);
    return ResultObj;
}
```

**必要なインクルード:**

```cpp
#include "Components/Slider.h"
```

---

## 4. add_checkbox_to_widget

### 仕様

CheckBox Widget を追加するツール。

### Python 実装 (`tools/umg_tools.py` に追加)

```python
@mcp.tool()
def add_checkbox_to_widget(
    ctx: Context,
    widget_name: str,
    checkbox_name: str,
    is_checked: bool = False,
    label_text: str = "",
    checked_color: List[float] = [1.0, 1.0, 1.0, 1.0],
    unchecked_color: List[float] = [0.5, 0.5, 0.5, 1.0],
    anchor: str = "Center",
    alignment: List[float] = [0.5, 0.5],
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a CheckBox widget to a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        checkbox_name: Name for the new CheckBox
        is_checked: Initial checked state (default: False)
        label_text: Label text to display next to the checkbox (optional)
        checked_color: [R, G, B, A] color when checked (default: white)
        unchecked_color: [R, G, B, A] color when unchecked (default: gray)
        anchor: Anchor position (default: "Center")
        alignment: [X, Y] alignment values (default: [0.5, 0.5])
        path: Content browser path to the widget (default: "/Game/UI")

    Returns:
        Dict containing success status and checkbox properties

    Example:
        add_checkbox_to_widget(
            widget_name="WBP_Settings",
            checkbox_name="MuteCheckbox",
            is_checked=False,
            label_text="Mute Audio",
            path="/Game/UI"
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
            "checkbox_name": checkbox_name,
            "is_checked": is_checked,
            "label_text": label_text,
            "checked_color": checked_color,
            "unchecked_color": unchecked_color,
            "anchor": anchor,
            "alignment": alignment,
            "path": path
        }

        logger.info(f"Adding CheckBox to widget with params: {params}")
        response = unreal.send_command("add_checkbox_to_widget", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Add CheckBox to widget response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error adding CheckBox to widget: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 実装

#### SpirrowBridgeUMGCommands.h に追加

```cpp
TSharedPtr<FJsonObject> HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("add_checkbox_to_widget"))
{
    return HandleAddCheckBoxToWidget(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString CheckBoxName;
    if (!Params->TryGetStringField(TEXT("checkbox_name"), CheckBoxName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'checkbox_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    bool bIsChecked = false;
    Params->TryGetBoolField(TEXT("is_checked"), bIsChecked);

    FString LabelText;
    Params->TryGetStringField(TEXT("label_text"), LabelText);

    // Get checked color [R, G, B, A]
    FLinearColor CheckedColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (Params->HasField(TEXT("checked_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("checked_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            CheckedColor.R = (*ColorArray)[0]->AsNumber();
            CheckedColor.G = (*ColorArray)[1]->AsNumber();
            CheckedColor.B = (*ColorArray)[2]->AsNumber();
            CheckedColor.A = (*ColorArray)[3]->AsNumber();
        }
    }

    // Get unchecked color [R, G, B, A]
    FLinearColor UncheckedColor(0.5f, 0.5f, 0.5f, 1.0f);
    if (Params->HasField(TEXT("unchecked_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ColorArray;
        if (Params->TryGetArrayField(TEXT("unchecked_color"), ColorArray) && ColorArray->Num() >= 4)
        {
            UncheckedColor.R = (*ColorArray)[0]->AsNumber();
            UncheckedColor.G = (*ColorArray)[1]->AsNumber();
            UncheckedColor.B = (*ColorArray)[2]->AsNumber();
            UncheckedColor.A = (*ColorArray)[3]->AsNumber();
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
    if (AnchorStr == TEXT("TopLeft")) Anchors = FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
    else if (AnchorStr == TEXT("TopCenter")) Anchors = FAnchors(0.5f, 0.0f, 0.5f, 0.0f);
    else if (AnchorStr == TEXT("TopRight")) Anchors = FAnchors(1.0f, 0.0f, 1.0f, 0.0f);
    else if (AnchorStr == TEXT("MiddleLeft")) Anchors = FAnchors(0.0f, 0.5f, 0.0f, 0.5f);
    else if (AnchorStr == TEXT("Center")) Anchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
    else if (AnchorStr == TEXT("MiddleRight")) Anchors = FAnchors(1.0f, 0.5f, 1.0f, 0.5f);
    else if (AnchorStr == TEXT("BottomLeft")) Anchors = FAnchors(0.0f, 1.0f, 0.0f, 1.0f);
    else if (AnchorStr == TEXT("BottomCenter")) Anchors = FAnchors(0.5f, 1.0f, 0.5f, 1.0f);
    else if (AnchorStr == TEXT("BottomRight")) Anchors = FAnchors(1.0f, 1.0f, 1.0f, 1.0f);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
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
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }

    // If label text is provided, create a HorizontalBox container with CheckBox and TextBlock
    UWidget* WidgetToAdd = nullptr;

    // Create CheckBox widget
    UCheckBox* CheckBoxWidget = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), FName(*CheckBoxName));
    if (!CheckBoxWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create CheckBox widget"));
    }

    // Set checkbox initial state
    CheckBoxWidget->SetIsChecked(bIsChecked);

    // Set checkbox style colors
    FCheckBoxStyle CheckBoxStyle = CheckBoxWidget->GetWidgetStyle();
    CheckBoxStyle.CheckedImage.TintColor = FSlateColor(CheckedColor);
    CheckBoxStyle.CheckedHoveredImage.TintColor = FSlateColor(CheckedColor);
    CheckBoxStyle.CheckedPressedImage.TintColor = FSlateColor(CheckedColor);
    CheckBoxStyle.UncheckedImage.TintColor = FSlateColor(UncheckedColor);
    CheckBoxStyle.UncheckedHoveredImage.TintColor = FSlateColor(UncheckedColor);
    CheckBoxStyle.UncheckedPressedImage.TintColor = FSlateColor(UncheckedColor);
    CheckBoxWidget->SetWidgetStyle(CheckBoxStyle);

    if (!LabelText.IsEmpty())
    {
        // Create HorizontalBox container
        FString ContainerName = CheckBoxName + TEXT("_Container");
        UHorizontalBox* Container = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*ContainerName));

        // Add CheckBox to container
        Container->AddChild(CheckBoxWidget);

        // Create and add TextBlock for label
        FString LabelName = CheckBoxName + TEXT("_Label");
        UTextBlock* LabelWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*LabelName));
        if (LabelWidget)
        {
            LabelWidget->SetText(FText::FromString(LabelText));
            Container->AddChild(LabelWidget);

            // Add padding to label
            if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(LabelWidget->Slot))
            {
                LabelSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
                LabelSlot->SetVerticalAlignment(VAlign_Center);
            }
        }

        WidgetToAdd = Container;
    }
    else
    {
        WidgetToAdd = CheckBoxWidget;
    }

    // Add to Canvas Panel
    UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(WidgetToAdd);
    if (Slot)
    {
        Slot->SetAnchors(Anchors);
        Slot->SetAlignment(Alignment);
        Slot->SetPosition(FVector2D(0, 0));
        Slot->SetAutoSize(true);
    }

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("checkbox_name"), CheckBoxName);
    ResultObj->SetBoolField(TEXT("is_checked"), bIsChecked);
    ResultObj->SetStringField(TEXT("label_text"), LabelText);
    return ResultObj;
}
```

**必要なインクルード:**

```cpp
#include "Components/CheckBox.h"
```

---

## 必要な変更まとめ

### Python側 (`tools/umg_tools.py`)

以下の4つの関数を追加:
1. `add_button_to_widget` (既存ツールの置き換え、新APIコマンド `add_button_to_widget_v2` を使用)
2. `bind_widget_component_event`
3. `add_slider_to_widget`
4. `add_checkbox_to_widget`

### C++側

#### インクルード追加 (`SpirrowBridgeUMGCommands.cpp`)

```cpp
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "K2Node_ComponentBoundEvent.h"
```

#### ヘッダー追加 (`SpirrowBridgeUMGCommands.h`)

```cpp
// Phase 4-A: Interactive Widgets
TSharedPtr<FJsonObject> HandleAddButtonToWidgetV2(const TSharedPtr<FJsonObject>& Params);
TSharedPtr<FJsonObject> HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params);
TSharedPtr<FJsonObject> HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params);
TSharedPtr<FJsonObject> HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params);
```

#### HandleCommand ルーティング追加

```cpp
// Phase 4-A: Interactive Widgets
else if (CommandName == TEXT("add_button_to_widget_v2"))
{
    return HandleAddButtonToWidgetV2(Params);
}
else if (CommandName == TEXT("bind_widget_component_event"))
{
    return HandleBindWidgetComponentEvent(Params);
}
else if (CommandName == TEXT("add_slider_to_widget"))
{
    return HandleAddSliderToWidget(Params);
}
else if (CommandName == TEXT("add_checkbox_to_widget"))
{
    return HandleAddCheckBoxToWidget(Params);
}
```

---

## テスト手順

### 1. Button テスト

```python
# Widget Blueprint 作成
create_umg_widget_blueprint(
    widget_name="WBP_TestMenu",
    path="/Game/UI"
)

# Button 追加
add_button_to_widget(
    widget_name="WBP_TestMenu",
    button_name="TestButton",
    text="Click Me!",
    font_size=16,
    size=[200, 50],
    anchor="Center",
    path="/Game/UI"
)

# イベントバインディング
bind_widget_component_event(
    widget_name="WBP_TestMenu",
    component_name="TestButton",
    event_type="OnClicked",
    path="/Game/UI"
)
```

### 2. Slider テスト

```python
add_slider_to_widget(
    widget_name="WBP_TestMenu",
    slider_name="VolumeSlider",
    value=0.5,
    min_value=0.0,
    max_value=1.0,
    size=[300, 25],
    anchor="BottomCenter",
    alignment=[0.5, 1.0],
    path="/Game/UI"
)
```

### 3. CheckBox テスト

```python
add_checkbox_to_widget(
    widget_name="WBP_TestMenu",
    checkbox_name="MuteCheckbox",
    is_checked=False,
    label_text="Mute Audio",
    anchor="TopLeft",
    alignment=[0.0, 0.0],
    path="/Game/UI"
)
```

---

## 補足事項

### bind_widget_component_event の注意点

`UK2Node_ComponentBoundEvent` を使用してイベントバインディングを作成する場合、Widget Blueprint の構造に依存するため、完全なバインディングには手動での調整が必要になる可能性がある。

代替として、以下のアプローチも検討:
1. Widget Blueprint の Event Graph に直接イベントノードを追加
2. `FKismetEditorUtilities::CreateNewBoundEventForComponent` の使用

実装時にこれらの選択肢を検討し、最も安定した方法を採用すること。

### スタイル設定について

Button, Slider, CheckBox のスタイル設定は、UE のバージョンによって API が異なる場合がある。UE 5.7 で動作確認を行い、必要に応じて調整すること。

---

**作成日**: 2024-12-24
**対象フェーズ**: UMG Phase 4-A
**ステータス**: 実装待ち
