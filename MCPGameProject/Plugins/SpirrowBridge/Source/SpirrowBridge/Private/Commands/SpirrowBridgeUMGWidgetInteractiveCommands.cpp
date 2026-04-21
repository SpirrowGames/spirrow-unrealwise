#include "Commands/SpirrowBridgeUMGWidgetInteractiveCommands.h"
#include "Commands/SpirrowBridgeUMGWidgetCoreCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Kismet2/KismetEditorUtilities.h"

FSpirrowBridgeUMGWidgetInteractiveCommands::FSpirrowBridgeUMGWidgetInteractiveCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("add_button_to_widget"))
	{
		return HandleAddButtonToWidgetV2(Params);
	}
	else if (CommandName == TEXT("add_slider_to_widget"))
	{
		return HandleAddSliderToWidget(Params);
	}
	else if (CommandName == TEXT("add_checkbox_to_widget"))
	{
		return HandleAddCheckBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_combobox_to_widget"))
	{
		return HandleAddComboBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_editabletext_to_widget"))
	{
		return HandleAddEditableTextToWidget(Params);
	}
	else if (CommandName == TEXT("add_spinbox_to_widget"))
	{
		return HandleAddSpinBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_scrollbox_to_widget"))
	{
		return HandleAddScrollBoxToWidget(Params);
	}

	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddButtonToWidget(const TSharedPtr<FJsonObject>& Params)
{
	return HandleAddButtonToWidgetV2(Params);
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddButtonToWidgetV2(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, ButtonName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("button_name"), ButtonName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, Text, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("text"), Text, TEXT(""));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	double FontSizeDouble;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("font_size"), FontSizeDouble, 14.0);
	int32 FontSize = static_cast<int32>(FontSizeDouble);

	FVector2D Size(200.0f, 50.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeArray;
	if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
	{
		Size.X = (*SizeArray)[0]->AsNumber();
		Size.Y = (*SizeArray)[1]->AsNumber();
	}

	// Get colors
	FLinearColor TextColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("text_color"), FLinearColor::White);
	FLinearColor NormalColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("normal_color"), FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));
	FLinearColor HoveredColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("hovered_color"), FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
	FLinearColor PressedColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("pressed_color"), FLinearColor(0.05f, 0.05f, 0.05f, 1.0f));

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignArray;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
	{
		Alignment.X = (*AlignArray)[0]->AsNumber();
		Alignment.Y = (*AlignArray)[1]->AsNumber();
	}

	FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;

	// Resolve parent (parent_name or root canvas)
	TSharedPtr<FJsonObject> ParentError;
	UPanelWidget* Parent = FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, ParentError);
	if (ParentError.IsValid())
	{
		return ParentError;
	}

	UButton* ButtonWidget = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*ButtonName));
	if (!ButtonWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create Button '%s'"), *ButtonName));
	}

	FButtonStyle ButtonStyle = ButtonWidget->GetStyle();
	ButtonStyle.Normal.TintColor = FSlateColor(NormalColor);
	ButtonStyle.Hovered.TintColor = FSlateColor(HoveredColor);
	ButtonStyle.Pressed.TintColor = FSlateColor(PressedColor);
	ButtonWidget->SetStyle(ButtonStyle);

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
			ButtonWidget->AddChild(TextBlock);
		}
	}

	UPanelSlot* AddedSlot = Parent->AddChild(ButtonWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(FVector2D(0, 0));
		CanvasSlot->SetSize(Size);
	}

	WidgetBP->Modify();
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("button_name"), ButtonName);
	ResultObj->SetStringField(TEXT("text"), Text);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, SliderName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("slider_name"), SliderName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, OrientationStr, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("orientation"), OrientationStr, TEXT("Horizontal"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	double Value, MinValue, MaxValue, StepSize;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("value"), Value, 0.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("min_value"), MinValue, 0.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("max_value"), MaxValue, 1.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("step_size"), StepSize, 0.0);

	EOrientation Orientation = (OrientationStr == TEXT("Vertical")) ? Orient_Vertical : Orient_Horizontal;

	FVector2D Size(200.0f, 20.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeArray;
	if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
	{
		Size.X = (*SizeArray)[0]->AsNumber();
		Size.Y = (*SizeArray)[1]->AsNumber();
	}

	FLinearColor BarColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("bar_color"), FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
	FLinearColor HandleColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("handle_color"), FLinearColor::White);

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignArray;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
	{
		Alignment.X = (*AlignArray)[0]->AsNumber();
		Alignment.Y = (*AlignArray)[1]->AsNumber();
	}

	FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;

	// Resolve parent (parent_name or root canvas)
	TSharedPtr<FJsonObject> ParentError;
	UPanelWidget* Parent = FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, ParentError);
	if (ParentError.IsValid())
	{
		return ParentError;
	}

	USlider* SliderWidget = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), FName(*SliderName));
	if (!SliderWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create Slider '%s'"), *SliderName));
	}

	SliderWidget->SetValue(static_cast<float>(Value));
	SliderWidget->SetMinValue(static_cast<float>(MinValue));
	SliderWidget->SetMaxValue(static_cast<float>(MaxValue));
	SliderWidget->SetStepSize(static_cast<float>(StepSize));
	SliderWidget->SetOrientation(Orientation);

	FSliderStyle SliderStyle = SliderWidget->GetWidgetStyle();
	SliderStyle.NormalBarImage.TintColor = FSlateColor(BarColor);
	SliderStyle.HoveredBarImage.TintColor = FSlateColor(BarColor);
	SliderStyle.NormalThumbImage.TintColor = FSlateColor(HandleColor);
	SliderStyle.HoveredThumbImage.TintColor = FSlateColor(HandleColor);
	SliderWidget->SetWidgetStyle(SliderStyle);

	UPanelSlot* AddedSlot = Parent->AddChild(SliderWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(FVector2D(0, 0));
		CanvasSlot->SetSize(Size);
	}

	WidgetBP->Modify();
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("slider_name"), SliderName);
	ResultObj->SetNumberField(TEXT("value"), Value);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, CheckBoxName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("checkbox_name"), CheckBoxName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, LabelText, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("label_text"), LabelText, TEXT(""));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	bool bIsChecked;
	FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("is_checked"), bIsChecked, false);

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignArray;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
	{
		Alignment.X = (*AlignArray)[0]->AsNumber();
		Alignment.Y = (*AlignArray)[1]->AsNumber();
	}

	FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;

	// Resolve parent (parent_name or root canvas)
	TSharedPtr<FJsonObject> ParentError;
	UPanelWidget* Parent = FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, ParentError);
	if (ParentError.IsValid())
	{
		return ParentError;
	}

	UCheckBox* CheckBoxWidget = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), FName(*CheckBoxName));
	if (!CheckBoxWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create CheckBox '%s'"), *CheckBoxName));
	}

	CheckBoxWidget->SetIsChecked(bIsChecked);

	UWidget* WidgetToAdd = CheckBoxWidget;

	if (!LabelText.IsEmpty())
	{
		FString ContainerName = CheckBoxName + TEXT("_Container");
		UHorizontalBox* Container = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(*ContainerName));
		Container->AddChild(CheckBoxWidget);

		FString LabelName = CheckBoxName + TEXT("_Label");
		UTextBlock* LabelWidget = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*LabelName));
		if (LabelWidget)
		{
			LabelWidget->SetText(FText::FromString(LabelText));
			Container->AddChild(LabelWidget);
			if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(LabelWidget->Slot))
			{
				LabelSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
				LabelSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
		WidgetToAdd = Container;
	}

	UPanelSlot* AddedSlot = Parent->AddChild(WidgetToAdd);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(FVector2D(0, 0));
		CanvasSlot->SetAutoSize(true);
	}

	WidgetBP->Modify();
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("checkbox_name"), CheckBoxName);
	ResultObj->SetBoolField(TEXT("is_checked"), bIsChecked);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddComboBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, ComboBoxName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("combobox_name"), ComboBoxName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	TArray<FString> Options;
	const TArray<TSharedPtr<FJsonValue>>* OptionsArray;
	if (Params->TryGetArrayField(TEXT("options"), OptionsArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *OptionsArray)
		{
			Options.Add(Val->AsString());
		}
	}

	double SelectedIndexDouble;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("selected_index"), SelectedIndexDouble, 0.0);
	int32 SelectedIndex = static_cast<int32>(SelectedIndexDouble);

	FVector2D Size(200.0f, 40.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeArray;
	if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
	{
		Size.X = (*SizeArray)[0]->AsNumber();
		Size.Y = (*SizeArray)[1]->AsNumber();
	}

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignArray;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
	{
		Alignment.X = (*AlignArray)[0]->AsNumber();
		Alignment.Y = (*AlignArray)[1]->AsNumber();
	}

	FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;

	// Resolve parent (parent_name or root canvas)
	TSharedPtr<FJsonObject> ParentError;
	UPanelWidget* Parent = FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, ParentError);
	if (ParentError.IsValid())
	{
		return ParentError;
	}

	UComboBoxString* ComboBoxWidget = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), FName(*ComboBoxName));
	if (!ComboBoxWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create ComboBox '%s'"), *ComboBoxName));
	}

	for (const FString& Option : Options)
	{
		ComboBoxWidget->AddOption(Option);
	}

	if (SelectedIndex >= 0 && SelectedIndex < Options.Num())
	{
		ComboBoxWidget->SetSelectedIndex(SelectedIndex);
	}

	UPanelSlot* AddedSlot = Parent->AddChild(ComboBoxWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("combobox_name"), ComboBoxName);
	ResultObj->SetNumberField(TEXT("option_count"), Options.Num());
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddEditableTextToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, TextName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("text_name"), TextName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, HintText, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("hint_text"), HintText, TEXT(""));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	bool bIsPassword, bIsMultiline;
	FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("is_password"), bIsPassword, false);
	FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("is_multiline"), bIsMultiline, false);

	FVector2D Size(200.0f, 40.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeArray;
	if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
	{
		Size.X = (*SizeArray)[0]->AsNumber();
		Size.Y = (*SizeArray)[1]->AsNumber();
	}

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignArray;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
	{
		Alignment.X = (*AlignArray)[0]->AsNumber();
		Alignment.Y = (*AlignArray)[1]->AsNumber();
	}

	FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;

	// Resolve parent (parent_name or root canvas)
	TSharedPtr<FJsonObject> ParentError;
	UPanelWidget* Parent = FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, ParentError);
	if (ParentError.IsValid())
	{
		return ParentError;
	}

	UWidget* TextWidget = nullptr;

	if (bIsMultiline)
	{
		UMultiLineEditableTextBox* MultiLineWidget = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(
			UMultiLineEditableTextBox::StaticClass(), FName(*TextName));
		if (!MultiLineWidget)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::WidgetCreationFailed,
				FString::Printf(TEXT("Failed to create MultiLineEditableTextBox '%s'"), *TextName));
		}
		MultiLineWidget->SetHintText(FText::FromString(HintText));
		TextWidget = MultiLineWidget;
	}
	else
	{
		UEditableTextBox* SingleLineWidget = WidgetTree->ConstructWidget<UEditableTextBox>(
			UEditableTextBox::StaticClass(), FName(*TextName));
		if (!SingleLineWidget)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::WidgetCreationFailed,
				FString::Printf(TEXT("Failed to create EditableTextBox '%s'"), *TextName));
		}
		SingleLineWidget->SetHintText(FText::FromString(HintText));
		SingleLineWidget->SetIsPassword(bIsPassword);
		TextWidget = SingleLineWidget;
	}

	UPanelSlot* AddedSlot = Parent->AddChild(TextWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("text_name"), TextName);
	ResultObj->SetBoolField(TEXT("is_multiline"), bIsMultiline);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddSpinBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, SpinBoxName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("spinbox_name"), SpinBoxName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	double Value, MinValue, MaxValue, Delta;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("value"), Value, 0.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("min_value"), MinValue, 0.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("max_value"), MaxValue, 100.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("delta"), Delta, 1.0);

	FVector2D Size(150.0f, 40.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeArray;
	if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
	{
		Size.X = (*SizeArray)[0]->AsNumber();
		Size.Y = (*SizeArray)[1]->AsNumber();
	}

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignArray;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
	{
		Alignment.X = (*AlignArray)[0]->AsNumber();
		Alignment.Y = (*AlignArray)[1]->AsNumber();
	}

	FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;

	// Resolve parent (parent_name or root canvas)
	TSharedPtr<FJsonObject> ParentError;
	UPanelWidget* Parent = FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, ParentError);
	if (ParentError.IsValid())
	{
		return ParentError;
	}

	USpinBox* SpinBoxWidget = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), FName(*SpinBoxName));
	if (!SpinBoxWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create SpinBox '%s'"), *SpinBoxName));
	}

	SpinBoxWidget->SetValue(Value);
	SpinBoxWidget->SetMinValue(MinValue);
	SpinBoxWidget->SetMaxValue(MaxValue);
	SpinBoxWidget->SetDelta(Delta);

	UPanelSlot* AddedSlot = Parent->AddChild(SpinBoxWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("spinbox_name"), SpinBoxName);
	ResultObj->SetNumberField(TEXT("value"), Value);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetInteractiveCommands::HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, ScrollBoxName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("scrollbox_name"), ScrollBoxName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, OrientationStr, ScrollBarVisibilityStr, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("orientation"), OrientationStr, TEXT("Vertical"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("scroll_bar_visibility"), ScrollBarVisibilityStr, TEXT("Visible"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	FVector2D Size(300.0f, 200.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeArray;
	if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
	{
		Size.X = (*SizeArray)[0]->AsNumber();
		Size.Y = (*SizeArray)[1]->AsNumber();
	}

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignArray;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
	{
		Alignment.X = (*AlignArray)[0]->AsNumber();
		Alignment.Y = (*AlignArray)[1]->AsNumber();
	}

	FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);

	// Validate and load Widget Blueprint
	UWidgetBlueprint* WidgetBP = nullptr;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateWidgetBlueprint(WidgetName, Path, WidgetBP))
	{
		return Error;
	}

	UWidgetTree* WidgetTree = WidgetBP->WidgetTree;

	// Resolve parent (parent_name or root canvas)
	TSharedPtr<FJsonObject> ParentError;
	UPanelWidget* Parent = FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget(WidgetTree, Params, ParentError);
	if (ParentError.IsValid())
	{
		return ParentError;
	}

	UScrollBox* ScrollBoxWidget = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName(*ScrollBoxName));
	if (!ScrollBoxWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create ScrollBox '%s'"), *ScrollBoxName));
	}

	EOrientation Orientation = (OrientationStr == TEXT("Horizontal")) ? Orient_Horizontal : Orient_Vertical;
	ScrollBoxWidget->SetOrientation(Orientation);

	ESlateVisibility ScrollBarVisibility = ESlateVisibility::Visible;
	if (ScrollBarVisibilityStr == TEXT("Hidden"))
	{
		ScrollBarVisibility = ESlateVisibility::Hidden;
	}
	else if (ScrollBarVisibilityStr == TEXT("Collapsed"))
	{
		ScrollBarVisibility = ESlateVisibility::Collapsed;
	}
	ScrollBoxWidget->SetScrollBarVisibility(ScrollBarVisibility);

	UPanelSlot* AddedSlot = Parent->AddChild(ScrollBoxWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("scrollbox_name"), ScrollBoxName);
	ResultObj->SetStringField(TEXT("orientation"), OrientationStr);
	return ResultObj;
}
