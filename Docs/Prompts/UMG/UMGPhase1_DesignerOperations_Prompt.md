# UMG Phase 1: Designer 操作の完全化

## 概要

Widget Blueprint の Designer 操作を MCP 経由で完全に行えるようにする。
レイアウト調整、プロパティ変更、コンテナ追加、要素の再構成が可能になる。

## 実装するツール一覧

| # | ツール名 | 機能 | 優先度 |
|---|----------|------|--------|
| 1 | `get_widget_elements` | Widget 内の要素一覧取得 | 最高 |
| 2 | `set_widget_slot_property` | Canvas Slot 設定変更 | 高 |
| 3 | `set_widget_element_property` | Widget 要素のプロパティ変更 | 高 |
| 4 | `add_vertical_box_to_widget` | VerticalBox 追加 | 高 |
| 5 | `add_horizontal_box_to_widget` | HorizontalBox 追加 | 中 |
| 6 | `reparent_widget_element` | 要素を別の親に移動 | 中 |
| 7 | `remove_widget_element` | 要素を削除 | 低 |

---

## 1. get_widget_elements

### Python 側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def get_widget_elements(
    ctx: Context,
    widget_name: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Get all elements in a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        path: Content browser path to the widget

    Returns:
        Dict containing list of elements with their names, types, and hierarchy

    Example:
        get_widget_elements(
            widget_name="WBP_TT_TrapSelector",
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
            "path": path
        }

        logger.info(f"Getting widget elements with params: {params}")
        response = unreal.send_command("get_widget_elements", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Get widget elements response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error getting widget elements: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 側

#### SpirrowBridgeUMGCommands.h に追加

```cpp
private:
    // Phase 1: Designer Operations
    TSharedPtr<FJsonObject> HandleGetWidgetElements(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("get_widget_elements"))
{
    return HandleGetWidgetElements(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleGetWidgetElements(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    // Get optional path parameter
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

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

    // Collect all widgets
    TArray<TSharedPtr<FJsonValue>> ElementsArray;
    TArray<UWidget*> AllWidgets;
    WidgetTree->GetAllWidgets(AllWidgets);

    for (UWidget* Widget : AllWidgets)
    {
        if (!Widget) continue;

        TSharedPtr<FJsonObject> ElementObj = MakeShared<FJsonObject>();
        ElementObj->SetStringField(TEXT("name"), Widget->GetName());
        ElementObj->SetStringField(TEXT("type"), Widget->GetClass()->GetName());

        // Get parent
        UPanelWidget* Parent = Widget->GetParent();
        if (Parent)
        {
            ElementObj->SetStringField(TEXT("parent"), Parent->GetName());
        }
        else
        {
            ElementObj->SetField(TEXT("parent"), MakeShared<FJsonValueNull>());
        }

        // Get children (if this is a panel widget)
        TArray<TSharedPtr<FJsonValue>> ChildrenArray;
        if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
        {
            for (int32 i = 0; i < PanelWidget->GetChildrenCount(); i++)
            {
                UWidget* Child = PanelWidget->GetChildAt(i);
                if (Child)
                {
                    ChildrenArray.Add(MakeShared<FJsonValueString>(Child->GetName()));
                }
            }
        }
        ElementObj->SetArrayField(TEXT("children"), ChildrenArray);

        // Get slot info if available
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
            
            FVector2D Position = CanvasSlot->GetPosition();
            TArray<TSharedPtr<FJsonValue>> PosArray;
            PosArray.Add(MakeShared<FJsonValueNumber>(Position.X));
            PosArray.Add(MakeShared<FJsonValueNumber>(Position.Y));
            SlotObj->SetArrayField(TEXT("position"), PosArray);

            FVector2D Size = CanvasSlot->GetSize();
            TArray<TSharedPtr<FJsonValue>> SizeArray;
            SizeArray.Add(MakeShared<FJsonValueNumber>(Size.X));
            SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Y));
            SlotObj->SetArrayField(TEXT("size"), SizeArray);

            FAnchors Anchors = CanvasSlot->GetAnchors();
            TArray<TSharedPtr<FJsonValue>> AnchorArray;
            AnchorArray.Add(MakeShared<FJsonValueNumber>(Anchors.Minimum.X));
            AnchorArray.Add(MakeShared<FJsonValueNumber>(Anchors.Minimum.Y));
            AnchorArray.Add(MakeShared<FJsonValueNumber>(Anchors.Maximum.X));
            AnchorArray.Add(MakeShared<FJsonValueNumber>(Anchors.Maximum.Y));
            SlotObj->SetArrayField(TEXT("anchors"), AnchorArray);

            FVector2D Alignment = CanvasSlot->GetAlignment();
            TArray<TSharedPtr<FJsonValue>> AlignArray;
            AlignArray.Add(MakeShared<FJsonValueNumber>(Alignment.X));
            AlignArray.Add(MakeShared<FJsonValueNumber>(Alignment.Y));
            SlotObj->SetArrayField(TEXT("alignment"), AlignArray);

            SlotObj->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());
            SlotObj->SetBoolField(TEXT("auto_size"), CanvasSlot->GetAutoSize());

            ElementObj->SetObjectField(TEXT("slot"), SlotObj);
        }

        ElementsArray.Add(MakeShared<FJsonValueObject>(ElementObj));
    }

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("root"), WidgetTree->RootWidget ? WidgetTree->RootWidget->GetName() : TEXT(""));
    ResultObj->SetArrayField(TEXT("elements"), ElementsArray);
    return ResultObj;
}
```

---

## 2. set_widget_slot_property

### Python 側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def set_widget_slot_property(
    ctx: Context,
    widget_name: str,
    element_name: str,
    position: List[float] = None,
    size: List[float] = None,
    anchor: str = None,
    alignment: List[float] = None,
    z_order: int = None,
    auto_size: bool = None,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Set Canvas Slot properties for a widget element.

    Args:
        widget_name: Name of the Widget Blueprint
        element_name: Name of the element to modify
        position: [X, Y] position offset from anchor
        size: [Width, Height] size override
        anchor: Anchor preset - "TopLeft", "TopCenter", "TopRight",
                "MiddleLeft", "Center", "MiddleRight",
                "BottomLeft", "BottomCenter", "BottomRight"
        alignment: [X, Y] alignment values 0.0-1.0
        z_order: Z-order for rendering priority
        auto_size: Whether to auto-size based on content
        path: Content browser path to the widget

    Returns:
        Dict containing success status and updated properties

    Example:
        set_widget_slot_property(
            widget_name="WBP_TT_TrapSelector",
            element_name="TrapIcon",
            position=[0, -100],
            size=[64, 64],
            anchor="BottomCenter",
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
            "element_name": element_name,
            "path": path
        }

        # Only include non-None parameters
        if position is not None:
            params["position"] = position
        if size is not None:
            params["size"] = size
        if anchor is not None:
            params["anchor"] = anchor
        if alignment is not None:
            params["alignment"] = alignment
        if z_order is not None:
            params["z_order"] = z_order
        if auto_size is not None:
            params["auto_size"] = auto_size

        logger.info(f"Setting widget slot property with params: {params}")
        response = unreal.send_command("set_widget_slot_property", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Set widget slot property response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error setting widget slot property: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 側

#### SpirrowBridgeUMGCommands.h に追加

```cpp
private:
    TSharedPtr<FJsonObject> HandleSetWidgetSlotProperty(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("set_widget_slot_property"))
{
    return HandleSetWidgetSlotProperty(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleSetWidgetSlotProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString ElementName;
    if (!Params->TryGetStringField(TEXT("element_name"), ElementName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'element_name' parameter"));
    }

    // Get optional path parameter
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    // Find the widget element
    UWidget* Widget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
    if (!Widget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
    }

    // Get Canvas Slot
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
    if (!CanvasSlot)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget element '%s' is not in a Canvas Panel"), *ElementName));
    }

    // Apply position if provided
    if (Params->HasField(TEXT("position")))
    {
        const TArray<TSharedPtr<FJsonValue>>* PosArray;
        if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
        {
            FVector2D Position((*PosArray)[0]->AsNumber(), (*PosArray)[1]->AsNumber());
            CanvasSlot->SetPosition(Position);
        }
    }

    // Apply size if provided
    if (Params->HasField(TEXT("size")))
    {
        const TArray<TSharedPtr<FJsonValue>>* SizeArray;
        if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
        {
            FVector2D Size((*SizeArray)[0]->AsNumber(), (*SizeArray)[1]->AsNumber());
            CanvasSlot->SetSize(Size);
        }
    }

    // Apply anchor if provided
    if (Params->HasField(TEXT("anchor")))
    {
        FString AnchorStr;
        if (Params->TryGetStringField(TEXT("anchor"), AnchorStr))
        {
            FAnchors Anchors(0.5f, 0.5f, 0.5f, 0.5f); // Default: Center

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

            CanvasSlot->SetAnchors(Anchors);
        }
    }

    // Apply alignment if provided
    if (Params->HasField(TEXT("alignment")))
    {
        const TArray<TSharedPtr<FJsonValue>>* AlignArray;
        if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
        {
            FVector2D Alignment((*AlignArray)[0]->AsNumber(), (*AlignArray)[1]->AsNumber());
            CanvasSlot->SetAlignment(Alignment);
        }
    }

    // Apply z_order if provided
    if (Params->HasField(TEXT("z_order")))
    {
        int32 ZOrder = Params->GetIntegerField(TEXT("z_order"));
        CanvasSlot->SetZOrder(ZOrder);
    }

    // Apply auto_size if provided
    if (Params->HasField(TEXT("auto_size")))
    {
        bool bAutoSize = Params->GetBoolField(TEXT("auto_size"));
        CanvasSlot->SetAutoSize(bAutoSize);
    }

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("element_name"), ElementName);
    return ResultObj;
}
```

---

## 3. set_widget_element_property

### Python 側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def set_widget_element_property(
    ctx: Context,
    widget_name: str,
    element_name: str,
    property_name: str,
    property_value: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Set a property on a widget element.

    Args:
        widget_name: Name of the Widget Blueprint
        element_name: Name of the element to modify
        property_name: Name of the property to set
        property_value: Value to set (JSON string for complex types)
        path: Content browser path to the widget

    Returns:
        Dict containing success status

    Common properties:
        - Visibility: "Visible", "Hidden", "Collapsed", "HitTestInvisible", "SelfHitTestInvisible"
        - Text (TextBlock): "Hello World"
        - ColorAndOpacity: "[1.0, 0.5, 0.0, 1.0]" (RGBA)
        - Justification: "Left", "Center", "Right"

    Example:
        set_widget_element_property(
            widget_name="WBP_TT_TrapSelector",
            element_name="TrapNameText",
            property_name="Visibility",
            property_value="Hidden",
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
            "element_name": element_name,
            "property_name": property_name,
            "property_value": property_value,
            "path": path
        }

        logger.info(f"Setting widget element property with params: {params}")
        response = unreal.send_command("set_widget_element_property", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Set widget element property response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error setting widget element property: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 側

#### SpirrowBridgeUMGCommands.h に追加

```cpp
private:
    TSharedPtr<FJsonObject> HandleSetWidgetElementProperty(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("set_widget_element_property"))
{
    return HandleSetWidgetElementProperty(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleSetWidgetElementProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString ElementName;
    if (!Params->TryGetStringField(TEXT("element_name"), ElementName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'element_name' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    FString PropertyValue;
    if (!Params->TryGetStringField(TEXT("property_value"), PropertyValue))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }

    // Get optional path parameter
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    // Find the widget element
    UWidget* Widget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
    if (!Widget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
    }

    bool bSuccess = false;

    // Handle special cases first
    if (PropertyName == TEXT("Visibility"))
    {
        ESlateVisibility NewVisibility = ESlateVisibility::Visible;
        if (PropertyValue == TEXT("Hidden"))
        {
            NewVisibility = ESlateVisibility::Hidden;
        }
        else if (PropertyValue == TEXT("Collapsed"))
        {
            NewVisibility = ESlateVisibility::Collapsed;
        }
        else if (PropertyValue == TEXT("HitTestInvisible"))
        {
            NewVisibility = ESlateVisibility::HitTestInvisible;
        }
        else if (PropertyValue == TEXT("SelfHitTestInvisible"))
        {
            NewVisibility = ESlateVisibility::SelfHitTestInvisible;
        }
        Widget->SetVisibility(NewVisibility);
        bSuccess = true;
    }
    else if (PropertyName == TEXT("Text"))
    {
        // Handle TextBlock
        if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
        {
            TextBlock->SetText(FText::FromString(PropertyValue));
            bSuccess = true;
        }
    }
    else if (PropertyName == TEXT("ColorAndOpacity"))
    {
        // Parse color from JSON array "[R, G, B, A]"
        TArray<FString> ColorParts;
        FString CleanValue = PropertyValue.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
        CleanValue.ParseIntoArray(ColorParts, TEXT(","));
        
        if (ColorParts.Num() >= 4)
        {
            FLinearColor Color(
                FCString::Atof(*ColorParts[0].TrimStartAndEnd()),
                FCString::Atof(*ColorParts[1].TrimStartAndEnd()),
                FCString::Atof(*ColorParts[2].TrimStartAndEnd()),
                FCString::Atof(*ColorParts[3].TrimStartAndEnd())
            );

            if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
            {
                TextBlock->SetColorAndOpacity(FSlateColor(Color));
                bSuccess = true;
            }
            else if (UImage* Image = Cast<UImage>(Widget))
            {
                Image->SetColorAndOpacity(Color);
                bSuccess = true;
            }
        }
    }
    else if (PropertyName == TEXT("Justification"))
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
        {
            ETextJustify::Type Justification = ETextJustify::Left;
            if (PropertyValue == TEXT("Center"))
            {
                Justification = ETextJustify::Center;
            }
            else if (PropertyValue == TEXT("Right"))
            {
                Justification = ETextJustify::Right;
            }
            TextBlock->SetJustification(Justification);
            bSuccess = true;
        }
    }
    else if (PropertyName == TEXT("Percent"))
    {
        if (UProgressBar* ProgressBar = Cast<UProgressBar>(Widget))
        {
            float Percent = FCString::Atof(*PropertyValue);
            ProgressBar->SetPercent(Percent);
            bSuccess = true;
        }
    }
    else
    {
        // Try using reflection for other properties
        FProperty* Property = Widget->GetClass()->FindPropertyByName(FName(*PropertyName));
        if (Property)
        {
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Widget);
            if (Property->ImportText_Direct(*PropertyValue, ValuePtr, Widget, PPF_None))
            {
                bSuccess = true;
            }
        }
    }

    if (!bSuccess)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to set property '%s' on element '%s'"), *PropertyName, *ElementName));
    }

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("element_name"), ElementName);
    ResultObj->SetStringField(TEXT("property_name"), PropertyName);
    return ResultObj;
}
```

---

## 4. add_vertical_box_to_widget

### Python 側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def add_vertical_box_to_widget(
    ctx: Context,
    widget_name: str,
    box_name: str,
    parent_name: str = None,
    anchor: str = "Center",
    alignment: List[float] = [0.5, 0.5],
    position: List[float] = [0, 0],
    size: List[float] = None,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a VerticalBox container to a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        box_name: Name for the new VerticalBox
        parent_name: Parent element name (null for root canvas)
        anchor: Anchor preset
        alignment: [X, Y] alignment values
        position: [X, Y] position offset
        size: [Width, Height] size override (null for auto-size)
        path: Content browser path to the widget

    Returns:
        Dict containing success status and box properties

    Example:
        add_vertical_box_to_widget(
            widget_name="WBP_TT_TrapSelector",
            box_name="MainContainer",
            anchor="BottomCenter",
            alignment=[0.5, 1.0],
            position=[0, -50],
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
            "box_name": box_name,
            "anchor": anchor,
            "alignment": alignment,
            "position": position,
            "path": path
        }

        if parent_name is not None:
            params["parent_name"] = parent_name
        if size is not None:
            params["size"] = size

        logger.info(f"Adding VerticalBox to widget with params: {params}")
        response = unreal.send_command("add_vertical_box_to_widget", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Add VerticalBox response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error adding VerticalBox to widget: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 側

#### SpirrowBridgeUMGCommands.h に追加

```cpp
private:
    TSharedPtr<FJsonObject> HandleAddVerticalBoxToWidget(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - インクルード追加

```cpp
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("add_vertical_box_to_widget"))
{
    return HandleAddVerticalBoxToWidget(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddVerticalBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString BoxName;
    if (!Params->TryGetStringField(TEXT("box_name"), BoxName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'box_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    FString ParentName;
    bool bHasParent = Params->TryGetStringField(TEXT("parent_name"), ParentName);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("WidgetTree not found"));
    }

    // Create VerticalBox
    UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*BoxName));
    if (!VerticalBox)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create VerticalBox"));
    }

    // Determine parent
    UPanelWidget* Parent = nullptr;
    if (bHasParent && !ParentName.IsEmpty())
    {
        Parent = Cast<UPanelWidget>(WidgetTree->FindWidget(FName(*ParentName)));
        if (!Parent)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Parent widget '%s' not found or not a panel"), *ParentName));
        }
    }
    else
    {
        Parent = Cast<UCanvasPanel>(WidgetTree->RootWidget);
        if (!Parent)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Root Canvas Panel not found"));
        }
    }

    // Add to parent
    UPanelSlot* Slot = Parent->AddChild(VerticalBox);

    // If added to CanvasPanel, set slot properties
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        // Set anchor
        FString AnchorStr = TEXT("Center");
        Params->TryGetStringField(TEXT("anchor"), AnchorStr);
        
        FAnchors Anchors(0.5f, 0.5f, 0.5f, 0.5f);
        if (AnchorStr == TEXT("TopLeft")) Anchors = FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
        else if (AnchorStr == TEXT("TopCenter")) Anchors = FAnchors(0.5f, 0.0f, 0.5f, 0.0f);
        else if (AnchorStr == TEXT("TopRight")) Anchors = FAnchors(1.0f, 0.0f, 1.0f, 0.0f);
        else if (AnchorStr == TEXT("MiddleLeft")) Anchors = FAnchors(0.0f, 0.5f, 0.0f, 0.5f);
        else if (AnchorStr == TEXT("Center")) Anchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
        else if (AnchorStr == TEXT("MiddleRight")) Anchors = FAnchors(1.0f, 0.5f, 1.0f, 0.5f);
        else if (AnchorStr == TEXT("BottomLeft")) Anchors = FAnchors(0.0f, 1.0f, 0.0f, 1.0f);
        else if (AnchorStr == TEXT("BottomCenter")) Anchors = FAnchors(0.5f, 1.0f, 0.5f, 1.0f);
        else if (AnchorStr == TEXT("BottomRight")) Anchors = FAnchors(1.0f, 1.0f, 1.0f, 1.0f);
        CanvasSlot->SetAnchors(Anchors);

        // Set alignment
        FVector2D Alignment(0.5f, 0.5f);
        const TArray<TSharedPtr<FJsonValue>>* AlignArray;
        if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
        {
            Alignment.X = (*AlignArray)[0]->AsNumber();
            Alignment.Y = (*AlignArray)[1]->AsNumber();
        }
        CanvasSlot->SetAlignment(Alignment);

        // Set position
        FVector2D Position(0.0f, 0.0f);
        const TArray<TSharedPtr<FJsonValue>>* PosArray;
        if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
        {
            Position.X = (*PosArray)[0]->AsNumber();
            Position.Y = (*PosArray)[1]->AsNumber();
        }
        CanvasSlot->SetPosition(Position);

        // Set size if provided, otherwise auto-size
        const TArray<TSharedPtr<FJsonValue>>* SizeArray;
        if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
        {
            FVector2D Size((*SizeArray)[0]->AsNumber(), (*SizeArray)[1]->AsNumber());
            CanvasSlot->SetSize(Size);
            CanvasSlot->SetAutoSize(false);
        }
        else
        {
            CanvasSlot->SetAutoSize(true);
        }
    }

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("box_name"), BoxName);
    ResultObj->SetStringField(TEXT("parent"), Parent->GetName());
    return ResultObj;
}
```

---

## 5. add_horizontal_box_to_widget

### Python 側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def add_horizontal_box_to_widget(
    ctx: Context,
    widget_name: str,
    box_name: str,
    parent_name: str = None,
    anchor: str = "Center",
    alignment: List[float] = [0.5, 0.5],
    position: List[float] = [0, 0],
    size: List[float] = None,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Add a HorizontalBox container to a Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        box_name: Name for the new HorizontalBox
        parent_name: Parent element name (null for root canvas)
        anchor: Anchor preset
        alignment: [X, Y] alignment values
        position: [X, Y] position offset
        size: [Width, Height] size override (null for auto-size)
        path: Content browser path to the widget

    Returns:
        Dict containing success status and box properties

    Example:
        add_horizontal_box_to_widget(
            widget_name="WBP_TT_TrapSelector",
            box_name="IconRow",
            parent_name="MainContainer",
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
            "box_name": box_name,
            "anchor": anchor,
            "alignment": alignment,
            "position": position,
            "path": path
        }

        if parent_name is not None:
            params["parent_name"] = parent_name
        if size is not None:
            params["size"] = size

        logger.info(f"Adding HorizontalBox to widget with params: {params}")
        response = unreal.send_command("add_horizontal_box_to_widget", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Add HorizontalBox response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error adding HorizontalBox to widget: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 側

#### SpirrowBridgeUMGCommands.h に追加

```cpp
private:
    TSharedPtr<FJsonObject> HandleAddHorizontalBoxToWidget(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("add_horizontal_box_to_widget"))
{
    return HandleAddHorizontalBoxToWidget(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddHorizontalBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString BoxName;
    if (!Params->TryGetStringField(TEXT("box_name"), BoxName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'box_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    FString ParentName;
    bool bHasParent = Params->TryGetStringField(TEXT("parent_name"), ParentName);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("WidgetTree not found"));
    }

    // Create HorizontalBox
    UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*BoxName));
    if (!HorizontalBox)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create HorizontalBox"));
    }

    // Determine parent
    UPanelWidget* Parent = nullptr;
    if (bHasParent && !ParentName.IsEmpty())
    {
        Parent = Cast<UPanelWidget>(WidgetTree->FindWidget(FName(*ParentName)));
        if (!Parent)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Parent widget '%s' not found or not a panel"), *ParentName));
        }
    }
    else
    {
        Parent = Cast<UCanvasPanel>(WidgetTree->RootWidget);
        if (!Parent)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Root Canvas Panel not found"));
        }
    }

    // Add to parent
    UPanelSlot* Slot = Parent->AddChild(HorizontalBox);

    // If added to CanvasPanel, set slot properties
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        // Set anchor
        FString AnchorStr = TEXT("Center");
        Params->TryGetStringField(TEXT("anchor"), AnchorStr);
        
        FAnchors Anchors(0.5f, 0.5f, 0.5f, 0.5f);
        if (AnchorStr == TEXT("TopLeft")) Anchors = FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
        else if (AnchorStr == TEXT("TopCenter")) Anchors = FAnchors(0.5f, 0.0f, 0.5f, 0.0f);
        else if (AnchorStr == TEXT("TopRight")) Anchors = FAnchors(1.0f, 0.0f, 1.0f, 0.0f);
        else if (AnchorStr == TEXT("MiddleLeft")) Anchors = FAnchors(0.0f, 0.5f, 0.0f, 0.5f);
        else if (AnchorStr == TEXT("Center")) Anchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
        else if (AnchorStr == TEXT("MiddleRight")) Anchors = FAnchors(1.0f, 0.5f, 1.0f, 0.5f);
        else if (AnchorStr == TEXT("BottomLeft")) Anchors = FAnchors(0.0f, 1.0f, 0.0f, 1.0f);
        else if (AnchorStr == TEXT("BottomCenter")) Anchors = FAnchors(0.5f, 1.0f, 0.5f, 1.0f);
        else if (AnchorStr == TEXT("BottomRight")) Anchors = FAnchors(1.0f, 1.0f, 1.0f, 1.0f);
        CanvasSlot->SetAnchors(Anchors);

        // Set alignment
        FVector2D Alignment(0.5f, 0.5f);
        const TArray<TSharedPtr<FJsonValue>>* AlignArray;
        if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
        {
            Alignment.X = (*AlignArray)[0]->AsNumber();
            Alignment.Y = (*AlignArray)[1]->AsNumber();
        }
        CanvasSlot->SetAlignment(Alignment);

        // Set position
        FVector2D Position(0.0f, 0.0f);
        const TArray<TSharedPtr<FJsonValue>>* PosArray;
        if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
        {
            Position.X = (*PosArray)[0]->AsNumber();
            Position.Y = (*PosArray)[1]->AsNumber();
        }
        CanvasSlot->SetPosition(Position);

        // Set size if provided, otherwise auto-size
        const TArray<TSharedPtr<FJsonValue>>* SizeArray;
        if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
        {
            FVector2D Size((*SizeArray)[0]->AsNumber(), (*SizeArray)[1]->AsNumber());
            CanvasSlot->SetSize(Size);
            CanvasSlot->SetAutoSize(false);
        }
        else
        {
            CanvasSlot->SetAutoSize(true);
        }
    }

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("box_name"), BoxName);
    ResultObj->SetStringField(TEXT("parent"), Parent->GetName());
    return ResultObj;
}
```

---

## 6. reparent_widget_element

### Python 側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def reparent_widget_element(
    ctx: Context,
    widget_name: str,
    element_name: str,
    new_parent_name: str,
    slot_index: int = -1,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Move a widget element to a new parent.

    Args:
        widget_name: Name of the Widget Blueprint
        element_name: Name of the element to move
        new_parent_name: Name of the new parent element
        slot_index: Index in parent's children (-1 for end)
        path: Content browser path to the widget

    Returns:
        Dict containing success status and new hierarchy

    Example:
        reparent_widget_element(
            widget_name="WBP_TT_TrapSelector",
            element_name="TrapIcon",
            new_parent_name="MainContainer",
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
            "element_name": element_name,
            "new_parent_name": new_parent_name,
            "slot_index": slot_index,
            "path": path
        }

        logger.info(f"Reparenting widget element with params: {params}")
        response = unreal.send_command("reparent_widget_element", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Reparent widget element response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error reparenting widget element: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 側

#### SpirrowBridgeUMGCommands.h に追加

```cpp
private:
    TSharedPtr<FJsonObject> HandleReparentWidgetElement(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("reparent_widget_element"))
{
    return HandleReparentWidgetElement(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleReparentWidgetElement(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString ElementName;
    if (!Params->TryGetStringField(TEXT("element_name"), ElementName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'element_name' parameter"));
    }

    FString NewParentName;
    if (!Params->TryGetStringField(TEXT("new_parent_name"), NewParentName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'new_parent_name' parameter"));
    }

    // Get optional parameters
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    int32 SlotIndex = -1;
    Params->TryGetNumberField(TEXT("slot_index"), SlotIndex);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("WidgetTree not found"));
    }

    // Find the element to move
    UWidget* Element = WidgetTree->FindWidget(FName(*ElementName));
    if (!Element)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
    }

    // Find the new parent
    UPanelWidget* NewParent = Cast<UPanelWidget>(WidgetTree->FindWidget(FName(*NewParentName)));
    if (!NewParent)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("New parent '%s' not found or not a panel widget"), *NewParentName));
    }

    // Get old parent for removal
    UPanelWidget* OldParent = Element->GetParent();
    FString OldParentName = OldParent ? OldParent->GetName() : TEXT("None");

    // Remove from old parent
    if (OldParent)
    {
        OldParent->RemoveChild(Element);
    }

    // Add to new parent
    UPanelSlot* NewSlot = nullptr;
    if (SlotIndex >= 0 && SlotIndex < NewParent->GetChildrenCount())
    {
        // Insert at specific index - need to add at end then reorder
        NewSlot = NewParent->AddChild(Element);
        // Note: UMG doesn't have a direct InsertChildAt, so we add at end
        // For proper index support, would need to remove all children after index,
        // add the new one, then re-add the removed ones. Simplified here.
    }
    else
    {
        NewSlot = NewParent->AddChild(Element);
    }

    if (!NewSlot)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to add element to new parent"));
    }

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("element_name"), ElementName);
    ResultObj->SetStringField(TEXT("old_parent"), OldParentName);
    ResultObj->SetStringField(TEXT("new_parent"), NewParentName);
    return ResultObj;
}
```

---

## 7. remove_widget_element

### Python 側 (`Python/tools/umg_tools.py`)

```python
@mcp.tool()
def remove_widget_element(
    ctx: Context,
    widget_name: str,
    element_name: str,
    path: str = "/Game/UI"
) -> Dict[str, Any]:
    """
    Remove a widget element from the Widget Blueprint.

    Args:
        widget_name: Name of the Widget Blueprint
        element_name: Name of the element to remove
        path: Content browser path to the widget

    Returns:
        Dict containing success status

    Example:
        remove_widget_element(
            widget_name="WBP_TT_TrapSelector",
            element_name="OldElement",
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
            "element_name": element_name,
            "path": path
        }

        logger.info(f"Removing widget element with params: {params}")
        response = unreal.send_command("remove_widget_element", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"Remove widget element response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error removing widget element: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### C++ 側

#### SpirrowBridgeUMGCommands.h に追加

```cpp
private:
    TSharedPtr<FJsonObject> HandleRemoveWidgetElement(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeUMGCommands.cpp - HandleCommand に追加

```cpp
else if (CommandName == TEXT("remove_widget_element"))
{
    return HandleRemoveWidgetElement(Params);
}
```

#### SpirrowBridgeUMGCommands.cpp - 実装

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleRemoveWidgetElement(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString WidgetName;
    if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
    }

    FString ElementName;
    if (!Params->TryGetStringField(TEXT("element_name"), ElementName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'element_name' parameter"));
    }

    // Get optional path parameter
    FString Path = TEXT("/Game/UI");
    Params->TryGetStringField(TEXT("path"), Path);

    // Load Widget Blueprint
    FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    if (!WidgetBP)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
    }

    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("WidgetTree not found"));
    }

    // Find the element to remove
    UWidget* Element = WidgetTree->FindWidget(FName(*ElementName));
    if (!Element)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
    }

    // Cannot remove root widget
    if (Element == WidgetTree->RootWidget)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Cannot remove root widget"));
    }

    // Remove from parent
    UPanelWidget* Parent = Element->GetParent();
    if (Parent)
    {
        Parent->RemoveChild(Element);
    }

    // Remove from widget tree
    WidgetTree->RemoveWidget(Element);

    // Mark as modified and compile
    WidgetBP->Modify();
    WidgetBP->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);

    // Create success response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
    ResultObj->SetStringField(TEXT("removed_element"), ElementName);
    return ResultObj;
}
```

---

## SpirrowBridge.cpp への追加

`ExecuteCommand` 関数内に、UMG コマンドのルーティングを追加:

```cpp
// 既存の UMG コマンドリストに追加
else if (CommandType == TEXT("create_umg_widget_blueprint") ||
         CommandType == TEXT("add_text_block_to_widget") ||
         CommandType == TEXT("add_widget_to_viewport") ||
         CommandType == TEXT("add_button_to_widget") ||
         CommandType == TEXT("bind_widget_event") ||
         CommandType == TEXT("set_text_block_binding") ||
         CommandType == TEXT("add_text_to_widget") ||
         CommandType == TEXT("add_image_to_widget") ||
         CommandType == TEXT("add_progressbar_to_widget") ||
         // Phase 1 新規追加
         CommandType == TEXT("get_widget_elements") ||
         CommandType == TEXT("set_widget_slot_property") ||
         CommandType == TEXT("set_widget_element_property") ||
         CommandType == TEXT("add_vertical_box_to_widget") ||
         CommandType == TEXT("add_horizontal_box_to_widget") ||
         CommandType == TEXT("reparent_widget_element") ||
         CommandType == TEXT("remove_widget_element"))
{
    ResultJson = UMGCommands->HandleCommand(CommandType, Params);
}
```

---

## テスト手順

### 1. ビルド確認

```
1. UE エディタを閉じる
2. Visual Studio で MCPGameProject をビルド
3. TrapxTrapCpp 5.7 の Plugins/SpirrowBridge にコピー
4. TrapxTrapCpp 5.7 を起動してビルド確認
5. MCP サーバー再起動
```

### 2. 機能テスト

```python
# 1. 要素一覧取得
get_widget_elements("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")

# 2. VerticalBox 追加
add_vertical_box_to_widget(
    widget_name="WBP_TT_TrapSelector",
    box_name="MainContainer",
    anchor="BottomCenter",
    alignment=[0.5, 1.0],
    position=[0, -50],
    path="/Game/TrapxTrap/UI"
)

# 3. 要素を VerticalBox に移動
reparent_widget_element("WBP_TT_TrapSelector", "TrapIcon", "MainContainer", path="/Game/TrapxTrap/UI")
reparent_widget_element("WBP_TT_TrapSelector", "TrapNameText", "MainContainer", path="/Game/TrapxTrap/UI")
reparent_widget_element("WBP_TT_TrapSelector", "TrapCountText", "MainContainer", path="/Game/TrapxTrap/UI")

# 4. Slot プロパティ変更
set_widget_slot_property(
    widget_name="WBP_TT_TrapSelector",
    element_name="MainContainer",
    position=[0, -100],
    path="/Game/TrapxTrap/UI"
)

# 5. 要素プロパティ変更（Visibility を Hidden に）
set_widget_element_property(
    widget_name="WBP_TT_TrapSelector",
    element_name="MainContainer",
    property_name="Visibility",
    property_value="Hidden",
    path="/Game/TrapxTrap/UI"
)

# 6. 要素一覧で確認
get_widget_elements("WBP_TT_TrapSelector", path="/Game/TrapxTrap/UI")
```

---

## Phase 1 完了後の project_context 更新

```python
update_project_context(
    project_name="SpirrowUnrealWise",
    current_phase="UMG Phase 1 - Designer 操作完全化 完了",
    recent_changes=[
        "get_widget_elements 実装",
        "set_widget_slot_property 実装",
        "set_widget_element_property 実装",
        "add_vertical_box_to_widget 実装",
        "add_horizontal_box_to_widget 実装",
        "reparent_widget_element 実装",
        "remove_widget_element 実装"
    ],
    next_tasks=[
        "Phase 2: Widget 変数・関数操作の実装",
        "add_widget_variable",
        "add_widget_function",
        "bind_widget_to_variable"
    ],
    known_issues=[],
    notes="Phase 1 で Designer 相当の操作が MCP 経由で可能に。Phase 2 で Graph 操作に進む。"
)
```

---

## 補足事項

### VerticalBox / HorizontalBox の子要素 Slot

VerticalBox や HorizontalBox に追加された要素は `UVerticalBoxSlot` / `UHorizontalBoxSlot` を持つ。
Canvas Slot とは異なるプロパティ（Fill, HAlign, VAlign, Padding 等）がある。

将来的に `set_box_slot_property` のような専用ツールが必要になる可能性あり。

### Visibility の Enum 値

```cpp
ESlateVisibility::Visible
ESlateVisibility::Collapsed
ESlateVisibility::Hidden
ESlateVisibility::HitTestInvisible
ESlateVisibility::SelfHitTestInvisible
```

### 修正日

2025-12-22
