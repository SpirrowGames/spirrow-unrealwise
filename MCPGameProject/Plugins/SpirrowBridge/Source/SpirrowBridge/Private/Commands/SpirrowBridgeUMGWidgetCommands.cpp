#include "Commands/SpirrowBridgeUMGWidgetCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "WidgetBlueprint.h"
#include "Factories/Factory.h"
#include "WidgetBlueprintEditor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "JsonObjectConverter.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/Button.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_Event.h"
#include "Misc/PackageName.h"
#include "EdGraphSchema_K2.h"
#include "UObject/StructOnScope.h"
// Phase 4-A: Interactive Widgets
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "K2Node_ComponentBoundEvent.h"
// Phase 4-B
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ScrollBox.h"

FSpirrowBridgeUMGWidgetCommands::FSpirrowBridgeUMGWidgetCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("create_umg_widget_blueprint"))
	{
		return HandleCreateUMGWidgetBlueprint(Params);
	}
	else if (CommandName == TEXT("add_text_to_widget"))
	{
		return HandleAddTextToWidget(Params);
	}
	else if (CommandName == TEXT("add_text_block_to_widget"))
	{
		return HandleAddTextBlockToWidget(Params);
	}
	else if (CommandName == TEXT("add_image_to_widget"))
	{
		return HandleAddImageToWidget(Params);
	}
	else if (CommandName == TEXT("add_progressbar_to_widget"))
	{
		return HandleAddProgressBarToWidget(Params);
	}
	else if (CommandName == TEXT("add_button_to_widget"))
	{
		return HandleAddButtonToWidget(Params);
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
	else if (CommandName == TEXT("add_widget_to_viewport"))
	{
		return HandleAddWidgetToViewport(Params);
	}

	return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown UMG Widget command: %s"), *CommandName));
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters - support both "widget_name" and "name" for compatibility
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("widget_name"), BlueprintName))
	{
		// Fallback to "name" for backward compatibility
		if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
		}
	}

	// Get optional path parameter (default: /Game/UI)
	FString PackagePath = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), PackagePath);

	// Ensure path ends without trailing slash for consistent formatting
	if (PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath = PackagePath.LeftChop(1);
	}

	// Get optional parent class (default: UserWidget)
	FString ParentClassName = TEXT("UserWidget");
	Params->TryGetStringField(TEXT("parent_class"), ParentClassName);

	// Find the parent class
	UClass* ParentClass = nullptr;
	if (ParentClassName == TEXT("UserWidget"))
	{
		ParentClass = UUserWidget::StaticClass();
	}
	else
	{
		// Try to find custom widget class using FindFirstObject (UE 5.1+ replacement for ANY_PACKAGE)
		ParentClass = FindFirstObject<UClass>(*ParentClassName, EFindFirstObjectOptions::None);
		if (!ParentClass)
		{
			// Try with full path
			FString FullParentPath = FString::Printf(TEXT("/Script/UMG.%s"), *ParentClassName);
			ParentClass = LoadObject<UClass>(nullptr, *FullParentPath);
		}
		if (!ParentClass)
		{
			// Fallback to UserWidget
			UE_LOG(LogTemp, Warning, TEXT("Parent class '%s' not found, using UserWidget"), *ParentClassName);
			ParentClass = UUserWidget::StaticClass();
		}
	}

	// Create the full asset path
	FString AssetName = BlueprintName;
	FString FullPath = PackagePath + TEXT("/") + AssetName;

	// Check if asset already exists
	if (UEditorAssetLibrary::DoesAssetExist(FullPath))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' already exists at '%s'"), *BlueprintName, *FullPath));
	}

	// Create package
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create Widget Blueprint using KismetEditorUtilities
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,                 // Parent class (dynamic)
		Package,                     // Outer package
		FName(*AssetName),           // Blueprint name
		BPTYPE_Normal,               // Blueprint type
		UWidgetBlueprint::StaticClass(),   // Blueprint class - use UWidgetBlueprint for widgets
		UBlueprintGeneratedClass::StaticClass(), // Generated class
		FName("CreateUMGWidget")     // Creation method name
	);

	// Make sure the Blueprint was created successfully
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewBlueprint);
	if (!WidgetBlueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Widget Blueprint"));
	}

	// Add a default Canvas Panel if one doesn't exist
	if (!WidgetBlueprint->WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
	}

	// Mark the package dirty and notify asset registry
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(WidgetBlueprint);

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

	// Save the asset using UE 5.0+ API
	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.SaveFlags = SAVE_NoError;
	UPackage::SavePackage(Package, WidgetBlueprint, *PackageFileName, SaveArgs);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), BlueprintName);
	ResultObj->SetStringField(TEXT("path"), FullPath);
	ResultObj->SetStringField(TEXT("parent_class"), ParentClass->GetName());
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddTextBlockToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	// Find the Widget Blueprint
	FString FullPath = TEXT("/Game/Widgets/") + BlueprintName;
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));
	if (!WidgetBlueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Get optional parameters
	FString InitialText = TEXT("New Text Block");
	Params->TryGetStringField(TEXT("text"), InitialText);

	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
		{
			Position.X = (*PosArray)[0]->AsNumber();
			Position.Y = (*PosArray)[1]->AsNumber();
		}
	}

	// Create Text Block widget
	UTextBlock* TextBlock = WidgetBlueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *WidgetName);
	if (!TextBlock)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Text Block widget"));
	}

	// Set initial text
	TextBlock->SetText(FText::FromString(InitialText));

	// Add to canvas panel
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Root Canvas Panel not found"));
	}

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(TextBlock);
	PanelSlot->SetPosition(Position);

	// Mark the package dirty and compile
	WidgetBlueprint->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("text"), InitialText);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	// Find the Widget Blueprint
	FString FullPath = TEXT("/Game/Widgets/") + BlueprintName;
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));
	if (!WidgetBlueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName));
	}

	// Get optional Z-order parameter
	int32 ZOrder = 0;
	Params->TryGetNumberField(TEXT("z_order"), ZOrder);

	// Create widget instance
	UClass* WidgetClass = WidgetBlueprint->GeneratedClass;
	if (!WidgetClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get widget class"));
	}

	// Note: This creates the widget but doesn't add it to viewport
	// The actual addition to viewport should be done through Blueprint nodes
	// as it requires a game context

	// Create success response with instructions
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetStringField(TEXT("class_path"), WidgetClass->GetPathName());
	ResultObj->SetNumberField(TEXT("z_order"), ZOrder);
	ResultObj->SetStringField(TEXT("note"), TEXT("Widget class ready. Use CreateWidget and AddToViewport nodes in Blueprint to display in game."));
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddButtonToWidget(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing blueprint_name parameter"));
		return Response;
	}

	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing widget_name parameter"));
		return Response;
	}

	FString ButtonText;
	if (!Params->TryGetStringField(TEXT("text"), ButtonText))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing text parameter"));
		return Response;
	}

	// Load the Widget Blueprint
	const FString BlueprintPath = FString::Printf(TEXT("/Game/Widgets/%s.%s"), *BlueprintName, *BlueprintName);
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!WidgetBlueprint)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *BlueprintPath));
		return Response;
	}

	// Create Button widget
	UButton* Button = NewObject<UButton>(WidgetBlueprint->GeneratedClass->GetDefaultObject(), UButton::StaticClass(), *WidgetName);
	if (!Button)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create Button widget"));
		return Response;
	}

	// Set button text
	UTextBlock* ButtonTextBlock = NewObject<UTextBlock>(Button, UTextBlock::StaticClass(), *(WidgetName + TEXT("_Text")));
	if (ButtonTextBlock)
	{
		ButtonTextBlock->SetText(FText::FromString(ButtonText));
		Button->AddChild(ButtonTextBlock);
	}

	// Get canvas panel and add button
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetBlueprint->WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		Response->SetStringField(TEXT("error"), TEXT("Root widget is not a Canvas Panel"));
		return Response;
	}

	// Add to canvas and set position
	UCanvasPanelSlot* ButtonSlot = RootCanvas->AddChildToCanvas(Button);
	if (ButtonSlot)
	{
		const TArray<TSharedPtr<FJsonValue>>* Position;
		if (Params->TryGetArrayField(TEXT("position"), Position) && Position->Num() >= 2)
		{
			FVector2D Pos(
				(*Position)[0]->AsNumber(),
				(*Position)[1]->AsNumber()
			);
			ButtonSlot->SetPosition(Pos);
		}
	}

	// Save the Widget Blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddTextToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	FString TextName;
	if (!Params->TryGetStringField(TEXT("text_name"), TextName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'text_name' parameter"));
	}

	// Get optional parameters
	FString Path = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), Path);

	FString Text = TEXT("+");
	Params->TryGetStringField(TEXT("text"), Text);

	int32 FontSize = 32;
	if (Params->HasField(TEXT("font_size")))
	{
		FontSize = Params->GetNumberField(TEXT("font_size"));
	}

	// Get color array [R, G, B, A]
	FLinearColor Color(1.0f, 1.0f, 1.0f, 1.0f);
	if (Params->HasField(TEXT("color")))
	{
		const TArray<TSharedPtr<FJsonValue>>* ColorArray;
		if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 4)
		{
			Color.R = (*ColorArray)[0]->AsNumber();
			Color.G = (*ColorArray)[1]->AsNumber();
			Color.B = (*ColorArray)[2]->AsNumber();
			Color.A = (*ColorArray)[3]->AsNumber();
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

	// Get anchor (default: Center = 0.5, 0.5, 0.5, 0.5)
	FString AnchorStr = TEXT("Center");
	Params->TryGetStringField(TEXT("anchor"), AnchorStr);
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

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

	// Create TextBlock
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*TextName));
	if (!TextBlock)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create TextBlock"));
	}

	// Set text
	TextBlock->SetText(FText::FromString(Text));

	// Set font size
	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);

	// Set color
	TextBlock->SetColorAndOpacity(FSlateColor(Color));

	// Add to Canvas Panel
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(TextBlock);
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
	ResultObj->SetStringField(TEXT("widget"), WidgetName);
	ResultObj->SetStringField(TEXT("text_name"), TextName);
	ResultObj->SetStringField(TEXT("text"), Text);
	ResultObj->SetNumberField(TEXT("font_size"), FontSize);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	FString ImageName;
	if (!Params->TryGetStringField(TEXT("image_name"), ImageName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'image_name' parameter"));
	}

	// Get optional parameters
	FString Path = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), Path);

	FString TexturePath;
	Params->TryGetStringField(TEXT("texture_path"), TexturePath);

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
	FLinearColor ColorTint(1.0f, 1.0f, 1.0f, 1.0f);
	if (Params->HasField(TEXT("color_tint")))
	{
		const TArray<TSharedPtr<FJsonValue>>* ColorArray;
		if (Params->TryGetArrayField(TEXT("color_tint"), ColorArray) && ColorArray->Num() >= 4)
		{
			ColorTint.R = (*ColorArray)[0]->AsNumber();
			ColorTint.G = (*ColorArray)[1]->AsNumber();
			ColorTint.B = (*ColorArray)[2]->AsNumber();
			ColorTint.A = (*ColorArray)[3]->AsNumber();
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
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

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

	// Create Image widget
	UImage* ImageWidget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*ImageName));
	if (!ImageWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Image widget"));
	}

	// Load and set texture if path is provided
	if (!TexturePath.IsEmpty())
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
		if (Texture)
		{
			ImageWidget->SetBrushFromTexture(Texture);
			UE_LOG(LogTemp, Log, TEXT("Set texture from path: %s"), *TexturePath);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load texture from path: %s"), *TexturePath);
		}
	}

	// Set size
	ImageWidget->SetDesiredSizeOverride(Size);

	// Set color tint
	ImageWidget->SetColorAndOpacity(ColorTint);

	// Add to Canvas Panel
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ImageWidget);
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
	ResultObj->SetStringField(TEXT("image_name"), ImageName);
	ResultObj->SetStringField(TEXT("texture_path"), TexturePath);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddProgressBarToWidget(const TSharedPtr<FJsonObject>& Params)
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
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddButtonToWidgetV2(const TSharedPtr<FJsonObject>& Params)
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
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddSliderToWidget(const TSharedPtr<FJsonObject>& Params)
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
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddCheckBoxToWidget(const TSharedPtr<FJsonObject>& Params)
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
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddComboBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	FString ComboBoxName;
	if (!Params->TryGetStringField(TEXT("combobox_name"), ComboBoxName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'combobox_name' parameter"));
	}

	// Get optional parameters
	FString Path = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), Path);

	const TArray<TSharedPtr<FJsonValue>>* OptionsArray = nullptr;
	TArray<FString> Options;
	if (Params->TryGetArrayField(TEXT("options"), OptionsArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *OptionsArray)
		{
			Options.Add(Val->AsString());
		}
	}

	int32 SelectedIndex = 0;
	Params->TryGetNumberField(TEXT("selected_index"), SelectedIndex);

	int32 FontSize = 14;
	Params->TryGetNumberField(TEXT("font_size"), FontSize);

	FVector2D Size(200.0f, 40.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeJson = nullptr;
	if (Params->TryGetArrayField(TEXT("size"), SizeJson) && SizeJson->Num() >= 2)
	{
		Size.X = (*SizeJson)[0]->AsNumber();
		Size.Y = (*SizeJson)[1]->AsNumber();
	}

	FString AnchorStr = TEXT("Center");
	Params->TryGetStringField(TEXT("anchor"), AnchorStr);

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignJson = nullptr;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignJson) && AlignJson->Num() >= 2)
	{
		Alignment.X = (*AlignJson)[0]->AsNumber();
		Alignment.Y = (*AlignJson)[1]->AsNumber();
	}

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
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Tree not found"));
	}

	// Create ComboBox widget
	UComboBoxString* ComboBoxWidget = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), FName(*ComboBoxName));
	if (!ComboBoxWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create ComboBox widget"));
	}

	// Add options
	for (const FString& Option : Options)
	{
		ComboBoxWidget->AddOption(Option);
	}

	// Set selected index if valid
	if (SelectedIndex >= 0 && SelectedIndex < Options.Num())
	{
		ComboBoxWidget->SetSelectedIndex(SelectedIndex);
	}

	// Parse anchor presets
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

	// Add to Canvas Panel
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Root widget is not a Canvas Panel"));
	}

	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(ComboBoxWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	// Mark Blueprint as modified and compile
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("combobox_name"), ComboBoxName);
	ResultObj->SetNumberField(TEXT("option_count"), Options.Num());
	ResultObj->SetNumberField(TEXT("selected_index"), SelectedIndex);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddEditableTextToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	FString TextName;
	if (!Params->TryGetStringField(TEXT("text_name"), TextName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'text_name' parameter"));
	}

	// Get optional parameters
	FString Path = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), Path);

	FString HintText = TEXT("");
	Params->TryGetStringField(TEXT("hint_text"), HintText);

	bool bIsPassword = false;
	Params->TryGetBoolField(TEXT("is_password"), bIsPassword);

	bool bIsMultiline = false;
	Params->TryGetBoolField(TEXT("is_multiline"), bIsMultiline);

	int32 FontSize = 14;
	Params->TryGetNumberField(TEXT("font_size"), FontSize);

	FLinearColor TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	const TArray<TSharedPtr<FJsonValue>>* ColorJson = nullptr;
	if (Params->TryGetArrayField(TEXT("text_color"), ColorJson) && ColorJson->Num() >= 4)
	{
		TextColor.R = (*ColorJson)[0]->AsNumber();
		TextColor.G = (*ColorJson)[1]->AsNumber();
		TextColor.B = (*ColorJson)[2]->AsNumber();
		TextColor.A = (*ColorJson)[3]->AsNumber();
	}

	FVector2D Size(200.0f, 40.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeJson = nullptr;
	if (Params->TryGetArrayField(TEXT("size"), SizeJson) && SizeJson->Num() >= 2)
	{
		Size.X = (*SizeJson)[0]->AsNumber();
		Size.Y = (*SizeJson)[1]->AsNumber();
	}

	FString AnchorStr = TEXT("Center");
	Params->TryGetStringField(TEXT("anchor"), AnchorStr);

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignJson = nullptr;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignJson) && AlignJson->Num() >= 2)
	{
		Alignment.X = (*AlignJson)[0]->AsNumber();
		Alignment.Y = (*AlignJson)[1]->AsNumber();
	}

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
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Tree not found"));
	}

	UWidget* TextWidget = nullptr;

	// Create appropriate text widget based on multiline setting
	if (bIsMultiline)
	{
		UMultiLineEditableTextBox* MultiLineWidget = WidgetTree->ConstructWidget<UMultiLineEditableTextBox>(
			UMultiLineEditableTextBox::StaticClass(), FName(*TextName));
		if (!MultiLineWidget)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create MultiLineEditableTextBox widget"));
		}

		MultiLineWidget->SetHintText(FText::FromString(HintText));
		MultiLineWidget->SetIsReadOnly(false);

		TextWidget = MultiLineWidget;
	}
	else
	{
		UEditableTextBox* SingleLineWidget = WidgetTree->ConstructWidget<UEditableTextBox>(
			UEditableTextBox::StaticClass(), FName(*TextName));
		if (!SingleLineWidget)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create EditableTextBox widget"));
		}

		SingleLineWidget->SetHintText(FText::FromString(HintText));
		SingleLineWidget->SetIsPassword(bIsPassword);
		SingleLineWidget->SetIsReadOnly(false);

		TextWidget = SingleLineWidget;
	}

	// Parse anchor presets
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

	// Add to Canvas Panel
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Root widget is not a Canvas Panel"));
	}

	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(TextWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	// Mark Blueprint as modified and compile
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("text_name"), TextName);
	ResultObj->SetBoolField(TEXT("is_multiline"), bIsMultiline);
	ResultObj->SetBoolField(TEXT("is_password"), bIsPassword);
	ResultObj->SetStringField(TEXT("hint_text"), HintText);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddSpinBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	FString SpinBoxName;
	if (!Params->TryGetStringField(TEXT("spinbox_name"), SpinBoxName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'spinbox_name' parameter"));
	}

	// Get optional parameters
	FString Path = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), Path);

	double Value = 0.0;
	Params->TryGetNumberField(TEXT("value"), Value);

	double MinValue = 0.0;
	Params->TryGetNumberField(TEXT("min_value"), MinValue);

	double MaxValue = 100.0;
	Params->TryGetNumberField(TEXT("max_value"), MaxValue);

	double Delta = 1.0;
	Params->TryGetNumberField(TEXT("delta"), Delta);

	FVector2D Size(150.0f, 40.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeJson = nullptr;
	if (Params->TryGetArrayField(TEXT("size"), SizeJson) && SizeJson->Num() >= 2)
	{
		Size.X = (*SizeJson)[0]->AsNumber();
		Size.Y = (*SizeJson)[1]->AsNumber();
	}

	FString AnchorStr = TEXT("Center");
	Params->TryGetStringField(TEXT("anchor"), AnchorStr);

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignJson = nullptr;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignJson) && AlignJson->Num() >= 2)
	{
		Alignment.X = (*AlignJson)[0]->AsNumber();
		Alignment.Y = (*AlignJson)[1]->AsNumber();
	}

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
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Tree not found"));
	}

	// Create SpinBox widget
	USpinBox* SpinBoxWidget = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), FName(*SpinBoxName));
	if (!SpinBoxWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create SpinBox widget"));
	}

	// Configure SpinBox
	SpinBoxWidget->SetValue(Value);
	SpinBoxWidget->SetMinValue(MinValue);
	SpinBoxWidget->SetMaxValue(MaxValue);
	SpinBoxWidget->SetDelta(Delta);

	// Parse anchor presets
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

	// Add to Canvas Panel
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Root widget is not a Canvas Panel"));
	}

	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(SpinBoxWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	// Mark Blueprint as modified and compile
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("spinbox_name"), SpinBoxName);
	ResultObj->SetNumberField(TEXT("value"), Value);
	ResultObj->SetNumberField(TEXT("min_value"), MinValue);
	ResultObj->SetNumberField(TEXT("max_value"), MaxValue);
	ResultObj->SetNumberField(TEXT("delta"), Delta);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCommands::HandleAddScrollBoxToWidget(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	FString ScrollBoxName;
	if (!Params->TryGetStringField(TEXT("scrollbox_name"), ScrollBoxName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'scrollbox_name' parameter"));
	}

	// Get optional parameters
	FString Path = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), Path);

	FString OrientationStr = TEXT("Vertical");
	Params->TryGetStringField(TEXT("orientation"), OrientationStr);

	FString ScrollBarVisibilityStr = TEXT("Visible");
	Params->TryGetStringField(TEXT("scroll_bar_visibility"), ScrollBarVisibilityStr);

	FVector2D Size(300.0f, 200.0f);
	const TArray<TSharedPtr<FJsonValue>>* SizeJson = nullptr;
	if (Params->TryGetArrayField(TEXT("size"), SizeJson) && SizeJson->Num() >= 2)
	{
		Size.X = (*SizeJson)[0]->AsNumber();
		Size.Y = (*SizeJson)[1]->AsNumber();
	}

	FString AnchorStr = TEXT("Center");
	Params->TryGetStringField(TEXT("anchor"), AnchorStr);

	FVector2D Alignment(0.5f, 0.5f);
	const TArray<TSharedPtr<FJsonValue>>* AlignJson = nullptr;
	if (Params->TryGetArrayField(TEXT("alignment"), AlignJson) && AlignJson->Num() >= 2)
	{
		Alignment.X = (*AlignJson)[0]->AsNumber();
		Alignment.Y = (*AlignJson)[1]->AsNumber();
	}

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
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Tree not found"));
	}

	// Create ScrollBox widget
	UScrollBox* ScrollBoxWidget = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName(*ScrollBoxName));
	if (!ScrollBoxWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create ScrollBox widget"));
	}

	// Set orientation
	EOrientation Orientation = EOrientation::Orient_Vertical;
	if (OrientationStr == TEXT("Horizontal"))
	{
		Orientation = EOrientation::Orient_Horizontal;
	}
	ScrollBoxWidget->SetOrientation(Orientation);

	// Set scroll bar visibility
	ESlateVisibility ScrollBarVisibility = ESlateVisibility::Visible;
	if (ScrollBarVisibilityStr == TEXT("Hidden"))
	{
		ScrollBarVisibility = ESlateVisibility::Hidden;
	}
	else if (ScrollBarVisibilityStr == TEXT("Collapsed"))
	{
		ScrollBarVisibility = ESlateVisibility::Collapsed;
	}
	else if (ScrollBarVisibilityStr == TEXT("HitTestInvisible"))
	{
		ScrollBarVisibility = ESlateVisibility::HitTestInvisible;
	}
	else if (ScrollBarVisibilityStr == TEXT("SelfHitTestInvisible"))
	{
		ScrollBarVisibility = ESlateVisibility::SelfHitTestInvisible;
	}
	ScrollBoxWidget->SetScrollBarVisibility(ScrollBarVisibility);

	// Parse anchor presets
	FAnchors Anchors = ParseAnchorPreset(AnchorStr);

	// Add to Canvas Panel
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Root widget is not a Canvas Panel"));
	}

	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(ScrollBoxWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(Size);
	}

	// Mark Blueprint as modified and compile
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("scrollbox_name"), ScrollBoxName);
	ResultObj->SetStringField(TEXT("orientation"), OrientationStr);
	ResultObj->SetStringField(TEXT("scroll_bar_visibility"), ScrollBarVisibilityStr);
	return ResultObj;
}

FAnchors FSpirrowBridgeUMGWidgetCommands::ParseAnchorPreset(const FString& AnchorStr)
{
	if (AnchorStr == TEXT("TopLeft"))
	{
		return FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
	}
	else if (AnchorStr == TEXT("TopCenter"))
	{
		return FAnchors(0.5f, 0.0f, 0.5f, 0.0f);
	}
	else if (AnchorStr == TEXT("TopRight"))
	{
		return FAnchors(1.0f, 0.0f, 1.0f, 0.0f);
	}
	else if (AnchorStr == TEXT("MiddleLeft"))
	{
		return FAnchors(0.0f, 0.5f, 0.0f, 0.5f);
	}
	else if (AnchorStr == TEXT("Center"))
	{
		return FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
	}
	else if (AnchorStr == TEXT("MiddleRight"))
	{
		return FAnchors(1.0f, 0.5f, 1.0f, 0.5f);
	}
	else if (AnchorStr == TEXT("BottomLeft"))
	{
		return FAnchors(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else if (AnchorStr == TEXT("BottomCenter"))
	{
		return FAnchors(0.5f, 1.0f, 0.5f, 1.0f);
	}
	else if (AnchorStr == TEXT("BottomRight"))
	{
		return FAnchors(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Default: Center
	return FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
}
