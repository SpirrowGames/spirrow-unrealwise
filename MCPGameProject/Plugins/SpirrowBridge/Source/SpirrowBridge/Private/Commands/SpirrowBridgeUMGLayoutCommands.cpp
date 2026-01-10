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
#include "Styling/SlateColor.h"
#include "Layout/Margin.h"

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
	else if (CommandName == TEXT("get_widget_element_property"))
	{
		return HandleGetWidgetElementProperty(Params);
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
	// Validate required parameters
	FString WidgetName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));

	// ★ New options ★
	bool bIncludeProperties = false;
	bool bExcludeDefaultValues = false;
	FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("include_properties"), bIncludeProperties, false);
	FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("exclude_default_values"), bExcludeDefaultValues, false);

	// Get class_filter array
	TArray<FString> ClassFilter;
	if (Params->HasField(TEXT("class_filter")))
	{
		const TArray<TSharedPtr<FJsonValue>>* ClassFilterArray;
		if (Params->TryGetArrayField(TEXT("class_filter"), ClassFilterArray))
		{
			for (const TSharedPtr<FJsonValue>& FilterValue : *ClassFilterArray)
			{
				ClassFilter.Add(FilterValue->AsString());
			}
		}
	}

	// Get property_filter array
	TArray<FString> PropertyFilter;
	if (Params->HasField(TEXT("property_filter")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PropertyFilterArray;
		if (Params->TryGetArrayField(TEXT("property_filter"), PropertyFilterArray))
		{
			for (const TSharedPtr<FJsonValue>& FilterValue : *PropertyFilterArray)
			{
				PropertyFilter.Add(FilterValue->AsString());
			}
		}
	}

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	// Get WidgetTree
	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetTreeNotFound,
			TEXT("WidgetTree not found"));
	}

	// Collect all widgets
	TArray<TSharedPtr<FJsonValue>> ElementsArray;
	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);

	for (UWidget* Widget : AllWidgets)
	{
		if (!Widget) continue;

		// ★ Apply class_filter ★
		if (ClassFilter.Num() > 0)
		{
			bool bMatchesFilter = false;
			FString WidgetClassName = Widget->GetClass()->GetName();
			for (const FString& FilterClass : ClassFilter)
			{
				if (WidgetClassName.Equals(FilterClass, ESearchCase::IgnoreCase) ||
					WidgetClassName.Contains(FilterClass, ESearchCase::IgnoreCase))
				{
					bMatchesFilter = true;
					break;
				}
			}
			if (!bMatchesFilter)
			{
				continue; // Skip this widget
			}
		}

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

		// ★ Include properties if requested ★
		if (bIncludeProperties)
		{
			TSharedPtr<FJsonObject> PropsObj = MakeShared<FJsonObject>();
			UClass* WidgetClass = Widget->GetClass();
			UObject* DefaultWidget = WidgetClass->GetDefaultObject();

			for (TFieldIterator<FProperty> PropIt(WidgetClass); PropIt; ++PropIt)
			{
				FProperty* Prop = *PropIt;

				// Only include editable properties
				if (!Prop->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst))
				{
					continue;
				}

				// Skip deprecated or hidden properties
				if (Prop->HasAnyPropertyFlags(CPF_Deprecated) || Prop->HasMetaData(TEXT("Hidden")))
				{
					continue;
				}

				FString PropName = Prop->GetName();

				// Apply property_filter if specified
				if (PropertyFilter.Num() > 0)
				{
					bool bMatchesFilter = false;
					for (const FString& FilterProp : PropertyFilter)
					{
						if (PropName.Equals(FilterProp, ESearchCase::IgnoreCase))
						{
							bMatchesFilter = true;
							break;
						}
					}
					if (!bMatchesFilter)
					{
						continue;
					}
				}

				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Widget);

				// ★ Exclude default values if requested ★
				if (bExcludeDefaultValues && DefaultWidget)
				{
					void* DefaultValuePtr = Prop->ContainerPtrToValuePtr<void>(DefaultWidget);
					if (Prop->Identical(ValuePtr, DefaultValuePtr))
					{
						continue; // Skip properties with default values
					}
				}

				// Convert property to JSON
				TSharedPtr<FJsonValue> PropValue = PropertyToJsonValue(Prop, ValuePtr);
				PropsObj->SetField(PropName, PropValue);
			}

			ElementObj->SetObjectField(TEXT("properties"), PropsObj);
		}

		ElementsArray.Add(MakeShared<FJsonValueObject>(ElementObj));
	}

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("root"), WidgetTree->RootWidget ? WidgetTree->RootWidget->GetName() : TEXT(""));
	ResultObj->SetNumberField(TEXT("element_count"), ElementsArray.Num());
	ResultObj->SetArrayField(TEXT("elements"), ElementsArray);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleSetWidgetSlotProperty(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, ElementName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("element_name"), ElementName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	// Find the widget element
	UWidget* Widget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
	if (!Widget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetElementNotFound,
			FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
	}

	// Get Canvas Slot
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (!CanvasSlot)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::CanvasPanelNotFound,
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
	// Validate required parameters
	FString WidgetName, ElementName, PropertyName, PropertyValue;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("element_name"), ElementName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_value"), PropertyValue))
	{
		return Error;
	}

	// Get optional parameters
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	// Find the widget element
	UWidget* Widget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
	if (!Widget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetElementNotFound,
			FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
	}

	bool bSuccess = false;

	// ★ Check if this is a nested property path (e.g., "Brush.TintColor") ★
	TArray<FString> PropertyPath;
	PropertyName.ParseIntoArray(PropertyPath, TEXT("."));

	if (PropertyPath.Num() > 1)
	{
		// Navigate through nested properties
		void* CurrentContainer = Widget;
		UStruct* CurrentStruct = Widget->GetClass();

		for (int32 i = 0; i < PropertyPath.Num() - 1; ++i)
		{
			FProperty* Prop = CurrentStruct->FindPropertyByName(*PropertyPath[i]);
			if (!Prop)
			{
				return FSpirrowBridgeCommonUtils::CreateErrorResponse(
					ESpirrowErrorCode::PropertyNotFound,
					FString::Printf(TEXT("Property '%s' not found in path '%s'"), *PropertyPath[i], *PropertyName));
			}

			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (!StructProp)
			{
				return FSpirrowBridgeCommonUtils::CreateErrorResponse(
					ESpirrowErrorCode::PropertyTypeMismatch,
					FString::Printf(TEXT("Property '%s' is not a struct type in path '%s'"), *PropertyPath[i], *PropertyName));
			}

			CurrentContainer = StructProp->ContainerPtrToValuePtr<void>(CurrentContainer);
			CurrentStruct = StructProp->Struct;
		}

		// Set the final property value
		FString FinalPropertyName = PropertyPath.Last();
		FProperty* FinalProp = CurrentStruct->FindPropertyByName(*FinalPropertyName);
		if (!FinalProp)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::PropertyNotFound,
				FString::Printf(TEXT("Final property '%s' not found in path '%s'"), *FinalPropertyName, *PropertyName));
		}

		// Try to set the property using ImportText
		void* ValuePtr = FinalProp->ContainerPtrToValuePtr<void>(CurrentContainer);
		if (FinalProp->ImportText_Direct(*PropertyValue, ValuePtr, nullptr, PPF_None))
		{
			bSuccess = true;
		}
		else
		{
			// Log the failure for debugging
			UE_LOG(LogTemp, Warning, TEXT("ImportText_Direct failed for nested property '%s' with value '%s'"),
				*PropertyName, *PropertyValue);
		}
	}
	// Handle special cases first (non-nested properties)
	else if (PropertyName == TEXT("Visibility"))
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
			ESpirrowErrorCode::PropertySetFailed,
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
	// Validate required parameters
	FString WidgetName, BoxName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("box_name"), BoxName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, ParentName, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));
	bool bHasParent = Params->TryGetStringField(TEXT("parent_name"), ParentName);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetTreeNotFound,
			TEXT("WidgetTree not found"));
	}

	// Create VerticalBox
	UVerticalBox* VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*BoxName));
	if (!VerticalBox)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create VerticalBox '%s'"), *BoxName));
	}

	// Determine parent
	UPanelWidget* Parent = nullptr;
	if (bHasParent && !ParentName.IsEmpty())
	{
		Parent = Cast<UPanelWidget>(WidgetTree->FindWidget(FName(*ParentName)));
		if (!Parent)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::WidgetElementNotFound,
				FString::Printf(TEXT("Parent widget '%s' not found or not a panel"), *ParentName));
		}
	}
	else
	{
		Parent = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!Parent)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::CanvasPanelNotFound,
				TEXT("Root Canvas Panel not found"));
		}
	}

	// Add to parent
	UPanelSlot* Slot = Parent->AddChild(VerticalBox);

	// If added to CanvasPanel, set slot properties
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
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
	// Validate required parameters
	FString WidgetName, BoxName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("box_name"), BoxName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, ParentName, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));
	bool bHasParent = Params->TryGetStringField(TEXT("parent_name"), ParentName);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetTreeNotFound,
			TEXT("WidgetTree not found"));
	}

	// Create HorizontalBox
	UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*BoxName));
	if (!HorizontalBox)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create HorizontalBox '%s'"), *BoxName));
	}

	// Determine parent
	UPanelWidget* Parent = nullptr;
	if (bHasParent && !ParentName.IsEmpty())
	{
		Parent = Cast<UPanelWidget>(WidgetTree->FindWidget(FName(*ParentName)));
		if (!Parent)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::WidgetElementNotFound,
				FString::Printf(TEXT("Parent widget '%s' not found or not a panel"), *ParentName));
		}
	}
	else
	{
		Parent = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!Parent)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::CanvasPanelNotFound,
				TEXT("Root Canvas Panel not found"));
		}
	}

	// Add to parent
	UPanelSlot* Slot = Parent->AddChild(HorizontalBox);

	// If added to CanvasPanel, set slot properties
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
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
	// Validate required parameters
	FString WidgetName, ElementName, NewParentName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("element_name"), ElementName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("new_parent_name"), NewParentName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));

	double SlotIndexDouble;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("slot_index"), SlotIndexDouble, -1.0);
	int32 SlotIndex = static_cast<int32>(SlotIndexDouble);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetTreeNotFound,
			TEXT("WidgetTree not found"));
	}

	// Find the element to move
	UWidget* Element = WidgetTree->FindWidget(FName(*ElementName));
	if (!Element)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetElementNotFound,
			FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
	}

	// Find the new parent
	UPanelWidget* NewParent = Cast<UPanelWidget>(WidgetTree->FindWidget(FName(*NewParentName)));
	if (!NewParent)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetElementNotFound,
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
	}
	else
	{
		NewSlot = NewParent->AddChild(Element);
	}

	if (!NewSlot)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			TEXT("Failed to add element to new parent"));
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
	// Validate required parameters
	FString WidgetName, ElementName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("element_name"), ElementName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
	if (!WidgetTree)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetTreeNotFound,
			TEXT("WidgetTree not found"));
	}

	// Find the element to remove
	UWidget* Element = WidgetTree->FindWidget(FName(*ElementName));
	if (!Element)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetElementNotFound,
			FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
	}

	// Cannot remove root widget
	if (Element == WidgetTree->RootWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidOperation,
			TEXT("Cannot remove root widget"));
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
			ESpirrowErrorCode::OperationFailed,
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

// ============================================
// get_widget_element_property implementation
// ============================================

TSharedPtr<FJsonValue> FSpirrowBridgeUMGLayoutCommands::StructToJsonValue(FStructProperty* StructProp, void* StructPtr)
{
	if (!StructProp || !StructPtr)
	{
		return MakeShared<FJsonValueNull>();
	}

	UScriptStruct* ScriptStruct = StructProp->Struct;
	FString StructName = ScriptStruct->GetName();

	// Handle known struct types with proper field expansion
	if (StructName == TEXT("LinearColor"))
	{
		FLinearColor* Color = static_cast<FLinearColor*>(StructPtr);
		TSharedPtr<FJsonObject> ColorObj = MakeShared<FJsonObject>();
		ColorObj->SetNumberField(TEXT("r"), Color->R);
		ColorObj->SetNumberField(TEXT("g"), Color->G);
		ColorObj->SetNumberField(TEXT("b"), Color->B);
		ColorObj->SetNumberField(TEXT("a"), Color->A);
		return MakeShared<FJsonValueObject>(ColorObj);
	}
	else if (StructName == TEXT("Color"))
	{
		FColor* Color = static_cast<FColor*>(StructPtr);
		TSharedPtr<FJsonObject> ColorObj = MakeShared<FJsonObject>();
		ColorObj->SetNumberField(TEXT("r"), Color->R);
		ColorObj->SetNumberField(TEXT("g"), Color->G);
		ColorObj->SetNumberField(TEXT("b"), Color->B);
		ColorObj->SetNumberField(TEXT("a"), Color->A);
		return MakeShared<FJsonValueObject>(ColorObj);
	}
	else if (StructName == TEXT("Vector"))
	{
		FVector* Vec = static_cast<FVector*>(StructPtr);
		TSharedPtr<FJsonObject> VecObj = MakeShared<FJsonObject>();
		VecObj->SetNumberField(TEXT("x"), Vec->X);
		VecObj->SetNumberField(TEXT("y"), Vec->Y);
		VecObj->SetNumberField(TEXT("z"), Vec->Z);
		return MakeShared<FJsonValueObject>(VecObj);
	}
	else if (StructName == TEXT("Vector2D"))
	{
		FVector2D* Vec = static_cast<FVector2D*>(StructPtr);
		TSharedPtr<FJsonObject> VecObj = MakeShared<FJsonObject>();
		VecObj->SetNumberField(TEXT("x"), Vec->X);
		VecObj->SetNumberField(TEXT("y"), Vec->Y);
		return MakeShared<FJsonValueObject>(VecObj);
	}
	else if (StructName == TEXT("Rotator"))
	{
		FRotator* Rot = static_cast<FRotator*>(StructPtr);
		TSharedPtr<FJsonObject> RotObj = MakeShared<FJsonObject>();
		RotObj->SetNumberField(TEXT("pitch"), Rot->Pitch);
		RotObj->SetNumberField(TEXT("yaw"), Rot->Yaw);
		RotObj->SetNumberField(TEXT("roll"), Rot->Roll);
		return MakeShared<FJsonValueObject>(RotObj);
	}
	else if (StructName == TEXT("Margin"))
	{
		FMargin* Margin = static_cast<FMargin*>(StructPtr);
		TSharedPtr<FJsonObject> MarginObj = MakeShared<FJsonObject>();
		MarginObj->SetNumberField(TEXT("left"), Margin->Left);
		MarginObj->SetNumberField(TEXT("top"), Margin->Top);
		MarginObj->SetNumberField(TEXT("right"), Margin->Right);
		MarginObj->SetNumberField(TEXT("bottom"), Margin->Bottom);
		return MakeShared<FJsonValueObject>(MarginObj);
	}
	else if (StructName == TEXT("SlateColor"))
	{
		FSlateColor* SlateColor = static_cast<FSlateColor*>(StructPtr);
		FLinearColor LinearColor = SlateColor->GetSpecifiedColor();
		TSharedPtr<FJsonObject> ColorObj = MakeShared<FJsonObject>();
		ColorObj->SetNumberField(TEXT("r"), LinearColor.R);
		ColorObj->SetNumberField(TEXT("g"), LinearColor.G);
		ColorObj->SetNumberField(TEXT("b"), LinearColor.B);
		ColorObj->SetNumberField(TEXT("a"), LinearColor.A);
		return MakeShared<FJsonValueObject>(ColorObj);
	}

	// For unknown structs, use ExportText
	FString ExportedValue;
	StructProp->ExportTextItem_Direct(ExportedValue, StructPtr, nullptr, nullptr, PPF_None);
	return MakeShared<FJsonValueString>(ExportedValue);
}

TSharedPtr<FJsonValue> FSpirrowBridgeUMGLayoutCommands::PropertyToJsonValue(FProperty* Property, void* ValuePtr)
{
	if (!Property || !ValuePtr)
	{
		return MakeShared<FJsonValueNull>();
	}

	// Boolean
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue(ValuePtr));
	}
	// Integer
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(IntProp->GetPropertyValue(ValuePtr));
	}
	// Float
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(FloatProp->GetPropertyValue(ValuePtr));
	}
	// Double
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(DoubleProp->GetPropertyValue(ValuePtr));
	}
	// String
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		return MakeShared<FJsonValueString>(StrProp->GetPropertyValue(ValuePtr));
	}
	// Name
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		return MakeShared<FJsonValueString>(NameProp->GetPropertyValue(ValuePtr).ToString());
	}
	// Text
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		return MakeShared<FJsonValueString>(TextProp->GetPropertyValue(ValuePtr).ToString());
	}
	// Byte/Enum
	else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		UEnum* EnumDef = ByteProp->GetIntPropertyEnum();
		if (EnumDef)
		{
			uint8 ByteValue = ByteProp->GetPropertyValue(ValuePtr);
			return MakeShared<FJsonValueString>(EnumDef->GetNameStringByValue(ByteValue));
		}
		return MakeShared<FJsonValueNumber>(ByteProp->GetPropertyValue(ValuePtr));
	}
	// Enum
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		UEnum* EnumDef = EnumProp->GetEnum();
		FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		if (EnumDef && UnderlyingProp)
		{
			int64 Value = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
			return MakeShared<FJsonValueString>(EnumDef->GetNameStringByValue(Value));
		}
	}
	// Struct
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		return StructToJsonValue(StructProp, ValuePtr);
	}
	// Object reference
	else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		UObject* Obj = ObjProp->GetObjectPropertyValue(ValuePtr);
		if (Obj)
		{
			return MakeShared<FJsonValueString>(Obj->GetPathName());
		}
		return MakeShared<FJsonValueNull>();
	}
	// Class reference
	else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		UClass* Class = Cast<UClass>(ClassProp->GetObjectPropertyValue(ValuePtr));
		if (Class)
		{
			return MakeShared<FJsonValueString>(Class->GetPathName());
		}
		return MakeShared<FJsonValueNull>();
	}
	// Array
	else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ValuePtr);
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			TSharedPtr<FJsonValue> ElementValue = PropertyToJsonValue(ArrayProp->Inner, ArrayHelper.GetRawPtr(i));
			JsonArray.Add(ElementValue);
		}
		return MakeShared<FJsonValueArray>(JsonArray);
	}

	// Fallback: use ExportText
	FString ExportedValue;
	Property->ExportTextItem_Direct(ExportedValue, ValuePtr, nullptr, nullptr, PPF_None);
	return MakeShared<FJsonValueString>(ExportedValue);
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGLayoutCommands::HandleGetWidgetElementProperty(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, ElementName, PropertyName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("element_name"), ElementName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	// Find the widget element
	UWidget* Widget = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
	if (!Widget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetElementNotFound,
			FString::Printf(TEXT("Widget element '%s' not found"), *ElementName));
	}

	// Parse property path for nested properties
	TArray<FString> PropertyPath;
	PropertyName.ParseIntoArray(PropertyPath, TEXT("."));

	void* CurrentContainer = Widget;
	UStruct* CurrentStruct = Widget->GetClass();
	FProperty* TargetProperty = nullptr;

	// Navigate through nested properties
	for (int32 i = 0; i < PropertyPath.Num(); ++i)
	{
		FProperty* Prop = CurrentStruct->FindPropertyByName(*PropertyPath[i]);
		if (!Prop)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::PropertyNotFound,
				FString::Printf(TEXT("Property '%s' not found in path '%s'"), *PropertyPath[i], *PropertyName));
		}

		if (i == PropertyPath.Num() - 1)
		{
			// This is the target property
			TargetProperty = Prop;
		}
		else
		{
			// Navigate to nested struct
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (!StructProp)
			{
				return FSpirrowBridgeCommonUtils::CreateErrorResponse(
					ESpirrowErrorCode::PropertyTypeMismatch,
					FString::Printf(TEXT("Property '%s' is not a struct type in path '%s'"), *PropertyPath[i], *PropertyName));
			}
			CurrentContainer = StructProp->ContainerPtrToValuePtr<void>(CurrentContainer);
			CurrentStruct = StructProp->Struct;
		}
	}

	if (!TargetProperty)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::PropertyNotFound,
			FString::Printf(TEXT("Property '%s' not found"), *PropertyName));
	}

	// Get property value
	void* ValuePtr = TargetProperty->ContainerPtrToValuePtr<void>(CurrentContainer);
	TSharedPtr<FJsonValue> JsonValue = PropertyToJsonValue(TargetProperty, ValuePtr);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("element_name"), ElementName);
	ResultObj->SetStringField(TEXT("property_name"), PropertyName);
	ResultObj->SetStringField(TEXT("property_type"), TargetProperty->GetCPPType());
	ResultObj->SetField(TEXT("value"), JsonValue);
	return ResultObj;
}
