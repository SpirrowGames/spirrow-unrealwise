#include "Commands/SpirrowBridgeUMGLayoutCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "EditorAssetLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Kismet2/KismetEditorUtilities.h"

FSpirrowBridgeUMGLayoutCommands::FSpirrowBridgeUMGLayoutCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("add_vertical_box_to_widget"))
	{
		return HandleAddVerticalBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_horizontal_box_to_widget"))
	{
		return HandleAddHorizontalBoxToWidget(Params);
	}
	else if (CommandName == TEXT("get_widget_elements"))
	{
		return HandleGetWidgetElements(Params);
	}
	else if (CommandName == TEXT("set_widget_slot_property"))
	{
		return HandleSetWidgetSlotProperty(Params);
	}
	else if (CommandName == TEXT("set_widget_element_property"))
	{
		return HandleSetWidgetElementProperty(Params);
	}
	else if (CommandName == TEXT("reparent_widget_element"))
	{
		return HandleReparentWidgetElement(Params);
	}
	else if (CommandName == TEXT("remove_widget_element"))
	{
		return HandleRemoveWidgetElement(Params);
	}

	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleGetWidgetElements(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleSetWidgetSlotProperty(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleSetWidgetElementProperty(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleAddVerticalBoxToWidget(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleAddHorizontalBoxToWidget(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleReparentWidgetElement(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleRemoveWidgetElement(const TSharedPtr<FJsonObject>& Params)
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

	// Mark blueprint as modified BEFORE making changes
	WidgetBP->Modify();

	// Get parent for removal
	UPanelWidget* Parent = Element->GetParent();
	FString ParentName = Parent ? Parent->GetName() : TEXT("None");

	// Remove from parent first
	if (Parent)
	{
		Parent->RemoveChild(Element);
	}

	// Remove from widget tree and check result
	bool bRemoved = WidgetTree->RemoveWidget(Element);

	if (!bRemoved)
	{
		UE_LOG(LogTemp, Warning, TEXT("RemoveWidget returned false for '%s'"), *ElementName);
	}

	// Force garbage collection hint
	Element->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
	Element->MarkAsGarbage();

	// Mark package dirty and recompile
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Verify removal
	UWidget* VerifyWidget = WidgetTree->FindWidget(FName(*ElementName));
	if (VerifyWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to completely remove widget '%s' from WidgetTree"), *ElementName));
	}

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("removed_element"), ElementName);
	ResultObj->SetStringField(TEXT("former_parent"), ParentName);
	return ResultObj;
}
