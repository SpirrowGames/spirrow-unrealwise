#include "Commands/SpirrowBridgeUMGWidgetBasicCommands.h"
#include "Commands/SpirrowBridgeUMGWidgetCoreCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"

FSpirrowBridgeUMGWidgetBasicCommands::FSpirrowBridgeUMGWidgetBasicCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetBasicCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("add_text_to_widget"))
	{
		return HandleAddTextToWidget(Params);
	}
	else if (CommandName == TEXT("add_image_to_widget"))
	{
		return HandleAddImageToWidget(Params);
	}
	else if (CommandName == TEXT("add_progressbar_to_widget"))
	{
		return HandleAddProgressBarToWidget(Params);
	}
	else if (CommandName == TEXT("add_border_to_widget"))
	{
		return HandleAddBorderToWidget(Params);
	}

	// Not handled by this class
	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetBasicCommands::HandleAddTextToWidget(const TSharedPtr<FJsonObject>& Params)
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
	FString Path, Text, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("text"), Text, TEXT("+"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	double FontSizeDouble;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("font_size"), FontSizeDouble, 32.0);
	int32 FontSize = static_cast<int32>(FontSizeDouble);

	// Get color array [R, G, B, A]
	FLinearColor Color = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("color"), FLinearColor::White);

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

	// Create TextBlock
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*TextName));
	if (!TextBlock)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create TextBlock '%s'"), *TextName));
	}

	// Set text
	TextBlock->SetText(FText::FromString(Text));

	// Set font size
	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);

	// Set color
	TextBlock->SetColorAndOpacity(FSlateColor(Color));

	// Add to parent; apply CanvasPanelSlot-specific layout only when parent is a canvas
	UPanelSlot* AddedSlot = Parent->AddChild(TextBlock);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(FVector2D(0, 0));
		CanvasSlot->SetAutoSize(true);
	}

	// Mark as modified and compile
	WidgetBP->Modify();
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("widget"), WidgetName);
	ResultObj->SetStringField(TEXT("text_name"), TextName);
	ResultObj->SetStringField(TEXT("text"), Text);
	ResultObj->SetNumberField(TEXT("font_size"), FontSize);
	ResultObj->SetStringField(TEXT("parent"), Parent->GetName());
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetBasicCommands::HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, ImageName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("image_name"), ImageName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, TexturePath, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("texture_path"), TexturePath, TEXT(""));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	// Get size [Width, Height]
	FVector2D Size(32.0f, 32.0f);
	if (Params->HasField(TEXT("size")))
	{
		const TArray<TSharedPtr<FJsonValue>>* SizeArray;
		if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() >= 2)
		{
			Size.X = (*SizeArray)[0]->AsNumber();
			Size.Y = (*SizeArray)[1]->AsNumber();
		}
	}

	// Get color tint [R, G, B, A]
	FLinearColor ColorTint = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("color_tint"), FLinearColor::White);

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

	// Create Image widget
	UImage* ImageWidget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*ImageName));
	if (!ImageWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create Image widget '%s'"), *ImageName));
	}

	// Load and set texture if path is provided
	if (!TexturePath.IsEmpty())
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
		if (Texture)
		{
			ImageWidget->SetBrushFromTexture(Texture);
		}
	}

	// Set size
	ImageWidget->SetDesiredSizeOverride(Size);

	// Set color tint
	ImageWidget->SetColorAndOpacity(ColorTint);

	// Add to parent; apply CanvasPanelSlot-specific layout only when parent is a canvas
	UPanelSlot* AddedSlot = Parent->AddChild(ImageWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(FVector2D(0, 0));
		CanvasSlot->SetSize(Size);
	}

	// Mark as modified and compile
	WidgetBP->Modify();
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("widget"), WidgetName);
	ResultObj->SetStringField(TEXT("image_name"), ImageName);
	ResultObj->SetStringField(TEXT("texture_path"), TexturePath);
	ResultObj->SetStringField(TEXT("parent"), Parent->GetName());
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetBasicCommands::HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString WidgetName, ProgressBarName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("progressbar_name"), ProgressBarName))
	{
		return Error;
	}

	// Get optional parameters
	FString Path, AnchorStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));

	double Percent;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("percent"), Percent, 0.0);

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

	// Get colors
	FLinearColor FillColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("fill_color"), FLinearColor(0.0f, 0.5f, 1.0f, 1.0f));
	FLinearColor BackgroundColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("background_color"), FLinearColor(0.1f, 0.1f, 0.1f, 1.0f));

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

	// Create ProgressBar widget
	UProgressBar* ProgressBarWidget = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), FName(*ProgressBarName));
	if (!ProgressBarWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create ProgressBar widget '%s'"), *ProgressBarName));
	}

	// Set percent
	ProgressBarWidget->SetPercent(static_cast<float>(Percent));

	// Set fill color
	ProgressBarWidget->SetFillColorAndOpacity(FillColor);

	// Set background color
	FProgressBarStyle Style = ProgressBarWidget->GetWidgetStyle();
	Style.BackgroundImage.TintColor = FSlateColor(BackgroundColor);
	ProgressBarWidget->SetWidgetStyle(Style);

	// Add to parent; apply CanvasPanelSlot-specific layout only when parent is a canvas
	UPanelSlot* AddedSlot = Parent->AddChild(ProgressBarWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(FVector2D(0, 0));
		CanvasSlot->SetSize(Size);
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
	ResultObj->SetStringField(TEXT("parent"), Parent->GetName());
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetBasicCommands::HandleAddBorderToWidget(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName, BorderName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), WidgetName))
	{
		return Error;
	}
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("border_name"), BorderName))
	{
		return Error;
	}

	FString Path, ParentName, AnchorStr, HAlignStr, VAlignStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("anchor"), AnchorStr, TEXT("Center"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("horizontal_alignment"), HAlignStr, TEXT("Fill"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("vertical_alignment"), VAlignStr, TEXT("Fill"));
	bool bHasParent = Params->TryGetStringField(TEXT("parent_name"), ParentName);

	FLinearColor BrushColor = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("brush_color"), FLinearColor::White);
	FLinearColor ContentTint = FSpirrowBridgeCommonUtils::GetLinearColorFromJson(Params, TEXT("content_color_and_opacity"), FLinearColor::White);

	FMargin Padding(0.0f);
	const TArray<TSharedPtr<FJsonValue>>* PadArr;
	if (Params->TryGetArrayField(TEXT("padding"), PadArr) && PadArr->Num() >= 4)
	{
		Padding = FMargin(
			static_cast<float>((*PadArr)[0]->AsNumber()),
			static_cast<float>((*PadArr)[1]->AsNumber()),
			static_cast<float>((*PadArr)[2]->AsNumber()),
			static_cast<float>((*PadArr)[3]->AsNumber()));
	}

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

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*BorderName));
	if (!Border)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create Border '%s'"), *BorderName));
	}

	Border->SetBrushColor(BrushColor);
	Border->SetContentColorAndOpacity(ContentTint);
	Border->SetPadding(Padding);

	EHorizontalAlignment HAlign = HAlign_Fill;
	if (HAlignStr == TEXT("Left")) HAlign = HAlign_Left;
	else if (HAlignStr == TEXT("Center")) HAlign = HAlign_Center;
	else if (HAlignStr == TEXT("Right")) HAlign = HAlign_Right;
	Border->SetHorizontalAlignment(HAlign);

	EVerticalAlignment VAlign = VAlign_Fill;
	if (VAlignStr == TEXT("Top")) VAlign = VAlign_Top;
	else if (VAlignStr == TEXT("Center")) VAlign = VAlign_Center;
	else if (VAlignStr == TEXT("Bottom")) VAlign = VAlign_Bottom;
	Border->SetVerticalAlignment(VAlign);

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
			Parent = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
			WidgetTree->RootWidget = Parent;
		}
	}

	UPanelSlot* Slot = Parent->AddChild(Border);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		FAnchors Anchors = FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(AnchorStr);
		CanvasSlot->SetAnchors(Anchors);

		FVector2D Alignment(0.5f, 0.5f);
		const TArray<TSharedPtr<FJsonValue>>* AlignArray;
		if (Params->TryGetArrayField(TEXT("alignment"), AlignArray) && AlignArray->Num() >= 2)
		{
			Alignment.X = (*AlignArray)[0]->AsNumber();
			Alignment.Y = (*AlignArray)[1]->AsNumber();
		}
		CanvasSlot->SetAlignment(Alignment);

		FVector2D Position(0.0f, 0.0f);
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
		{
			Position.X = (*PosArray)[0]->AsNumber();
			Position.Y = (*PosArray)[1]->AsNumber();
		}
		CanvasSlot->SetPosition(Position);

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

	WidgetBP->Modify();
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget"), WidgetName);
	ResultObj->SetStringField(TEXT("border_name"), BorderName);
	ResultObj->SetStringField(TEXT("parent"), Parent->GetName());
	return ResultObj;
}
