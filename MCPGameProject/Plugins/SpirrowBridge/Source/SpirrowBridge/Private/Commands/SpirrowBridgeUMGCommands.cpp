#include "Commands/SpirrowBridgeUMGCommands.h"
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
// We'll create widgets using regular Factory classes
#include "Factories/Factory.h"
// Remove problematic includes that don't exist in UE 5.5
// #include "UMGEditorSubsystem.h"
// #include "WidgetBlueprintFactory.h"
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
// Phase 3: Animation support
#include "Animation/WidgetAnimation.h"
#include "MovieScene.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieSceneColorTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Sections/MovieSceneColorSection.h"
#include "Channels/MovieSceneFloatChannel.h"
// Phase 4-A: Interactive Widgets
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "K2Node_ComponentBoundEvent.h"

FSpirrowBridgeUMGCommands::FSpirrowBridgeUMGCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("create_umg_widget_blueprint"))
	{
		return HandleCreateUMGWidgetBlueprint(Params);
	}
	else if (CommandName == TEXT("add_text_block_to_widget"))
	{
		return HandleAddTextBlockToWidget(Params);
	}
	else if (CommandName == TEXT("add_widget_to_viewport"))
	{
		return HandleAddWidgetToViewport(Params);
	}
	else if (CommandName == TEXT("add_button_to_widget"))
	{
		return HandleAddButtonToWidget(Params);
	}
	else if (CommandName == TEXT("bind_widget_event"))
	{
		return HandleBindWidgetEvent(Params);
	}
	else if (CommandName == TEXT("set_text_block_binding"))
	{
		return HandleSetTextBlockBinding(Params);
	}
	else if (CommandName == TEXT("add_text_to_widget"))
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
	// Phase 1: Designer Operations
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
	else if (CommandName == TEXT("add_vertical_box_to_widget"))
	{
		return HandleAddVerticalBoxToWidget(Params);
	}
	else if (CommandName == TEXT("add_horizontal_box_to_widget"))
	{
		return HandleAddHorizontalBoxToWidget(Params);
	}
	else if (CommandName == TEXT("reparent_widget_element"))
	{
		return HandleReparentWidgetElement(Params);
	}
	else if (CommandName == TEXT("remove_widget_element"))
	{
		return HandleRemoveWidgetElement(Params);
	}
	// Phase 2: Variable & Function Operations
	else if (CommandName == TEXT("add_widget_variable"))
	{
		return HandleAddWidgetVariable(Params);
	}
	else if (CommandName == TEXT("set_widget_variable_default"))
	{
		return HandleSetWidgetVariableDefault(Params);
	}
	else if (CommandName == TEXT("add_widget_function"))
	{
		return HandleAddWidgetFunction(Params);
	}
	else if (CommandName == TEXT("add_widget_event"))
	{
		return HandleAddWidgetEvent(Params);
	}
	else if (CommandName == TEXT("bind_widget_to_variable"))
	{
		return HandleBindWidgetToVariable(Params);
	}
	// Phase 3: Animation Operations
	else if (CommandName == TEXT("create_widget_animation"))
	{
		return HandleCreateWidgetAnimation(Params);
	}
	else if (CommandName == TEXT("add_animation_track"))
	{
		return HandleAddAnimationTrack(Params);
	}
	else if (CommandName == TEXT("add_animation_keyframe"))
	{
		return HandleAddAnimationKeyframe(Params);
	}
	else if (CommandName == TEXT("get_widget_animations"))
	{
		return HandleGetWidgetAnimations(Params);
	}
	else if (CommandName == TEXT("add_widget_array_variable"))
	{
		return HandleAddWidgetArrayVariable(Params);
	}
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

	return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown UMG command: %s"), *CommandName));
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddTextBlockToWidget(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddButtonToWidget(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleBindWidgetEvent(const TSharedPtr<FJsonObject>& Params)
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

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing event_name parameter"));
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

	// Create the event graph if it doesn't exist
	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(WidgetBlueprint);
	if (!EventGraph)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to find or create event graph"));
		return Response;
	}

	// Find the widget in the blueprint
	UWidget* Widget = WidgetBlueprint->WidgetTree->FindWidget(*WidgetName);
	if (!Widget)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to find widget: %s"), *WidgetName));
		return Response;
	}

	// Create the event node (e.g., OnClicked for buttons)
	UK2Node_Event* EventNode = nullptr;
	
	// Find existing nodes first
	TArray<UK2Node_Event*> AllEventNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(WidgetBlueprint, AllEventNodes);
	
	for (UK2Node_Event* Node : AllEventNodes)
	{
		if (Node->CustomFunctionName == FName(*EventName) && Node->EventReference.GetMemberParentClass() == Widget->GetClass())
		{
			EventNode = Node;
			break;
		}
	}

	// If no existing node, create a new one
	if (!EventNode)
	{
		// Calculate position - place it below existing nodes
		float MaxHeight = 0.0f;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			MaxHeight = FMath::Max(MaxHeight, Node->NodePosY);
		}
		
		const FVector2D NodePos(200, MaxHeight + 200);

		// Call CreateNewBoundEventForClass, which returns void, so we can't capture the return value directly
		// We'll need to find the node after creating it
		FKismetEditorUtilities::CreateNewBoundEventForClass(
			Widget->GetClass(),
			FName(*EventName),
			WidgetBlueprint,
			nullptr  // We don't need a specific property binding
		);

		// Now find the newly created node
		TArray<UK2Node_Event*> UpdatedEventNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(WidgetBlueprint, UpdatedEventNodes);
		
		for (UK2Node_Event* Node : UpdatedEventNodes)
		{
			if (Node->CustomFunctionName == FName(*EventName) && Node->EventReference.GetMemberParentClass() == Widget->GetClass())
			{
				EventNode = Node;
				
				// Set position of the node
				EventNode->NodePosX = NodePos.X;
				EventNode->NodePosY = NodePos.Y;
				
				break;
			}
		}
	}

	if (!EventNode)
	{
		Response->SetStringField(TEXT("error"), TEXT("Failed to create event node"));
		return Response;
	}

	// Save the Widget Blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("event_name"), EventName);
	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params)
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

	FString BindingName;
	if (!Params->TryGetStringField(TEXT("binding_name"), BindingName))
	{
		Response->SetStringField(TEXT("error"), TEXT("Missing binding_name parameter"));
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

	// Create a variable for binding if it doesn't exist
	FBlueprintEditorUtils::AddMemberVariable(
		WidgetBlueprint,
		FName(*BindingName),
		FEdGraphPinType(UEdGraphSchema_K2::PC_Text, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType())
	);

	// Find the TextBlock widget
	UTextBlock* TextBlock = Cast<UTextBlock>(WidgetBlueprint->WidgetTree->FindWidget(FName(*WidgetName)));
	if (!TextBlock)
	{
		Response->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to find TextBlock widget: %s"), *WidgetName));
		return Response;
	}

	// Create binding function
	const FString FunctionName = FString::Printf(TEXT("Get%s"), *BindingName);
	UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
		WidgetBlueprint,
		FName(*FunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass()
	);

	if (FuncGraph)
	{
		// Add the function to the blueprint with proper template parameter
		// Template requires null for last parameter when not using a signature-source
		FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBlueprint, FuncGraph, false, nullptr);

		// Create entry node
		UK2Node_FunctionEntry* EntryNode = nullptr;
		
		// Create entry node - use the API that exists in UE 5.5
		EntryNode = NewObject<UK2Node_FunctionEntry>(FuncGraph);
		FuncGraph->AddNode(EntryNode, false, false);
		EntryNode->NodePosX = 0;
		EntryNode->NodePosY = 0;
		EntryNode->FunctionReference.SetExternalMember(FName(*FunctionName), WidgetBlueprint->GeneratedClass);
		EntryNode->AllocateDefaultPins();

		// Create get variable node
		UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
		GetVarNode->VariableReference.SetSelfMember(FName(*BindingName));
		FuncGraph->AddNode(GetVarNode, false, false);
		GetVarNode->NodePosX = 200;
		GetVarNode->NodePosY = 0;
		GetVarNode->AllocateDefaultPins();

		// Connect nodes
		UEdGraphPin* EntryThenPin = EntryNode->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* GetVarOutPin = GetVarNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
		if (EntryThenPin && GetVarOutPin)
		{
			EntryThenPin->MakeLinkTo(GetVarOutPin);
		}
	}

	// Save the Widget Blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
	UEditorAssetLibrary::SaveAsset(BlueprintPath, false);

	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("binding_name"), BindingName);
	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddTextToWidget(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddImageToWidget(const TSharedPtr<FJsonObject>& Params)
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

// Phase 1: Designer Operations

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

// ============================================================================
// Phase 2: Variable & Function Operations
// ============================================================================

// Helper function to setup pin types for variables and function parameters
bool FSpirrowBridgeUMGCommands::SetupPinType(const FString& TypeName, FEdGraphPinType& OutPinType)
{
	OutPinType.PinCategory = NAME_None;
	OutPinType.PinSubCategory = NAME_None;
	OutPinType.PinSubCategoryObject = nullptr;
	OutPinType.ContainerType = EPinContainerType::None;
	OutPinType.bIsReference = false;

	if (TypeName == TEXT("Boolean") || TypeName == TEXT("Bool"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (TypeName == TEXT("Integer") || TypeName == TEXT("Int"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (TypeName == TEXT("Float") || TypeName == TEXT("Double"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
	}
	else if (TypeName == TEXT("String"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (TypeName == TEXT("Name"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else if (TypeName == TEXT("Text"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
	}
	else if (TypeName == TEXT("Vector"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (TypeName == TEXT("Vector2D"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		OutPinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
	}
	else if (TypeName == TEXT("Rotator"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	}
	else if (TypeName == TEXT("Transform"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
	}
	else if (TypeName == TEXT("LinearColor") || TypeName == TEXT("Color"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
	}
	else if (TypeName == TEXT("TimerHandle"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		OutPinType.PinSubCategoryObject = FTimerHandle::StaticStruct();
	}
	else if (TypeName == TEXT("Texture2D"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
		OutPinType.PinSubCategoryObject = UTexture2D::StaticClass();
	}
	else if (TypeName == TEXT("Object"))
	{
		OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
		OutPinType.PinSubCategoryObject = UObject::StaticClass();
	}
	else
	{
		// Try to find custom class or struct
		UClass* FoundClass = FindObject<UClass>(nullptr, *TypeName);
		if (FoundClass)
		{
			OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			OutPinType.PinSubCategoryObject = FoundClass;
		}
		else
		{
			UScriptStruct* FoundStruct = FindObject<UScriptStruct>(nullptr, *TypeName);
			if (FoundStruct)
			{
				OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				OutPinType.PinSubCategoryObject = FoundStruct;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetVariable(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'widget_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FString VariableType;
	if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
	}

	// Get optional parameters
	FString Path = TEXT("/Game/UI");
	Params->TryGetStringField(TEXT("path"), Path);

	FString DefaultValue;
	bool bHasDefault = Params->TryGetStringField(TEXT("default_value"), DefaultValue);

	bool bIsExposed = false;
	Params->TryGetBoolField(TEXT("is_exposed"), bIsExposed);

	FString Category;
	Params->TryGetStringField(TEXT("category"), Category);

	// Load Widget Blueprint
	FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
	}

	// Setup Pin Type
	FEdGraphPinType PinType;
	if (!SetupPinType(VariableType, PinType))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
	}

	// Add variable to Blueprint
	FBlueprintEditorUtils::AddMemberVariable(WidgetBP, FName(*VariableName), PinType);

	// Find and configure the newly created variable
	FBPVariableDescription* NewVar = nullptr;
	for (FBPVariableDescription& Variable : WidgetBP->NewVariables)
	{
		if (Variable.VarName == FName(*VariableName))
		{
			NewVar = &Variable;
			break;
		}
	}

	if (NewVar)
	{
		// Set editor exposure
		if (bIsExposed)
		{
			NewVar->PropertyFlags |= CPF_Edit | CPF_BlueprintVisible;
		}

		// Set category
		if (!Category.IsEmpty())
		{
			NewVar->Category = FText::FromString(Category);
		}

		// Set default value
		if (bHasDefault)
		{
			NewVar->DefaultValue = DefaultValue;
		}
	}

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("variable_name"), VariableName);
	ResultObj->SetStringField(TEXT("variable_type"), VariableType);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleSetWidgetVariableDefault(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName, VariableName, DefaultValue, Path = TEXT("/Game/UI");

	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("variable_name"), VariableName) ||
		!Params->TryGetStringField(TEXT("default_value"), DefaultValue))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}
	Params->TryGetStringField(TEXT("path"), Path);

	// Load Widget Blueprint
	FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint '%s' not found"), *WidgetName));
	}

	// Find variable
	FBPVariableDescription* Variable = nullptr;
	for (FBPVariableDescription& Var : WidgetBP->NewVariables)
	{
		if (Var.VarName == FName(*VariableName))
		{
			Variable = &Var;
			break;
		}
	}

	if (!Variable)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Variable '%s' not found in Widget Blueprint"), *VariableName));
	}

	// Set default value
	Variable->DefaultValue = DefaultValue;

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("variable_name"), VariableName);
	ResultObj->SetStringField(TEXT("default_value"), DefaultValue);
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetFunction(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName, FunctionName, Path = TEXT("/Game/UI");
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}
	Params->TryGetStringField(TEXT("path"), Path);

	bool bIsPure = false;
	Params->TryGetBoolField(TEXT("is_pure"), bIsPure);

	// Load Widget Blueprint
	FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Blueprint not found"));
	}

	// Check if function already exists
	for (UEdGraph* Graph : WidgetBP->FunctionGraphs)
	{
		if (Graph->GetFName() == FName(*FunctionName))
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Function '%s' already exists"), *FunctionName));
		}
	}

	// Create function graph
	UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
		WidgetBP,
		FName(*FunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass()
	);

	if (!FuncGraph)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create function graph"));
	}

	// Add function to Blueprint - This automatically creates the Entry node
	FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);

	// Find the existing Entry node (created by AddFunctionGraph)
	UK2Node_FunctionEntry* EntryNode = nullptr;
	for (UEdGraphNode* Node : FuncGraph->Nodes)
	{
		EntryNode = Cast<UK2Node_FunctionEntry>(Node);
		if (EntryNode)
		{
			break;
		}
	}

	if (!EntryNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find function entry node"));
	}

	// Add input parameters to the existing Entry node
	const TArray<TSharedPtr<FJsonValue>>* InputsArray;
	if (Params->TryGetArrayField(TEXT("inputs"), InputsArray))
	{
		for (const TSharedPtr<FJsonValue>& InputValue : *InputsArray)
		{
			const TSharedPtr<FJsonObject>* InputObj;
			if (InputValue->TryGetObject(InputObj))
			{
				FString ParamName, ParamType;
				(*InputObj)->TryGetStringField(TEXT("name"), ParamName);
				(*InputObj)->TryGetStringField(TEXT("type"), ParamType);

				if (!ParamName.IsEmpty() && !ParamType.IsEmpty())
				{
					FEdGraphPinType PinType;
					if (SetupPinType(ParamType, PinType))
					{
						EntryNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
					}
				}
			}
		}
	}

	// Add output parameters if provided - Create Result node only if needed
	const TArray<TSharedPtr<FJsonValue>>* OutputsArray;
	if (Params->TryGetArrayField(TEXT("outputs"), OutputsArray) && OutputsArray->Num() > 0)
	{
		// Check if Result node already exists
		UK2Node_FunctionResult* ResultNode = nullptr;
		for (UEdGraphNode* Node : FuncGraph->Nodes)
		{
			ResultNode = Cast<UK2Node_FunctionResult>(Node);
			if (ResultNode)
			{
				break;
			}
		}

		// Create Result node only if it doesn't exist
		if (!ResultNode)
		{
			ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
			FuncGraph->AddNode(ResultNode, false, false);
			ResultNode->NodePosX = 400;
			ResultNode->NodePosY = 0;
			ResultNode->AllocateDefaultPins();
		}

		for (const TSharedPtr<FJsonValue>& OutputValue : *OutputsArray)
		{
			const TSharedPtr<FJsonObject>* OutputObj;
			if (OutputValue->TryGetObject(OutputObj))
			{
				FString ParamName, ParamType;
				(*OutputObj)->TryGetStringField(TEXT("name"), ParamName);
				(*OutputObj)->TryGetStringField(TEXT("type"), ParamType);

				if (!ParamName.IsEmpty() && !ParamType.IsEmpty())
				{
					FEdGraphPinType PinType;
					if (SetupPinType(ParamType, PinType))
					{
						ResultNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Input);
					}
				}
			}
		}
	}

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("function_name"), FunctionName);
	ResultObj->SetStringField(TEXT("graph_id"), FuncGraph->GraphGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetEvent(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName, EventName, Path = TEXT("/Game/UI");
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}
	Params->TryGetStringField(TEXT("path"), Path);

	// Load Widget Blueprint
	FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Blueprint not found"));
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

	// Create Custom Event node
	UK2Node_Event* EventNode = NewObject<UK2Node_Event>(EventGraph);
	EventGraph->AddNode(EventNode, false, false);
	EventNode->CustomFunctionName = FName(*EventName);
	EventNode->bIsEditable = true;
	EventNode->NodePosX = 0;
	EventNode->NodePosY = 0;
	EventNode->AllocateDefaultPins();

	// Add input parameters if provided
	const TArray<TSharedPtr<FJsonValue>>* InputsArray;
	if (Params->TryGetArrayField(TEXT("inputs"), InputsArray))
	{
		for (const TSharedPtr<FJsonValue>& InputValue : *InputsArray)
		{
			const TSharedPtr<FJsonObject>* InputObj;
			if (InputValue->TryGetObject(InputObj))
			{
				FString ParamName, ParamType;
				(*InputObj)->TryGetStringField(TEXT("name"), ParamName);
				(*InputObj)->TryGetStringField(TEXT("type"), ParamType);

				FEdGraphPinType PinType;
				if (SetupPinType(ParamType, PinType))
				{
					EventNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
				}
			}
		}
	}

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("event_name"), EventName);
	ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleBindWidgetToVariable(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName, ElementName, PropertyName, VariableName, Path = TEXT("/Game/UI");
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("element_name"), ElementName) ||
		!Params->TryGetStringField(TEXT("property_name"), PropertyName) ||
		!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}
	Params->TryGetStringField(TEXT("path"), Path);

	// Load Widget Blueprint
	FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget Blueprint not found"));
	}

	// Find widget element
	UWidget* Element = WidgetBP->WidgetTree->FindWidget(FName(*ElementName));
	if (!Element)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Widget element not found"));
	}

	// Generate binding function name
	FString FunctionName = FString::Printf(TEXT("Get%s%s"), *VariableName, *PropertyName);

	// Check if binding function already exists
	UEdGraph* FuncGraph = nullptr;
	for (UEdGraph* Graph : WidgetBP->FunctionGraphs)
	{
		if (Graph->GetFName() == FName(*FunctionName))
		{
			FuncGraph = Graph;
			break;
		}
	}

	// Create binding function if it doesn't exist
	if (!FuncGraph)
	{
		// Find the variable to determine return type
		FBPVariableDescription* Variable = nullptr;
		for (FBPVariableDescription& Var : WidgetBP->NewVariables)
		{
			if (Var.VarName == FName(*VariableName))
			{
				Variable = &Var;
				break;
			}
		}

		if (!Variable)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Variable '%s' not found in Widget Blueprint"), *VariableName));
		}

		// Create function graph
		FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
			WidgetBP,
			FName(*FunctionName),
			UEdGraph::StaticClass(),
			UEdGraphSchema_K2::StaticClass()
		);

		if (!FuncGraph)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create binding function"));
		}

		// Add function to Blueprint - This automatically creates the Entry node
		FBlueprintEditorUtils::AddFunctionGraph<UClass>(WidgetBP, FuncGraph, false, nullptr);

		// Find the existing Entry node (created by AddFunctionGraph)
		UK2Node_FunctionEntry* EntryNode = nullptr;
		for (UEdGraphNode* Node : FuncGraph->Nodes)
		{
			EntryNode = Cast<UK2Node_FunctionEntry>(Node);
			if (EntryNode)
			{
				break;
			}
		}

		if (!EntryNode)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find function entry node"));
		}

		// Create Variable Get node
		UK2Node_VariableGet* GetVarNode = NewObject<UK2Node_VariableGet>(FuncGraph);
		GetVarNode->VariableReference.SetSelfMember(FName(*VariableName));
		FuncGraph->AddNode(GetVarNode, false, false);
		GetVarNode->NodePosX = 200;
		GetVarNode->NodePosY = 0;
		GetVarNode->AllocateDefaultPins();

		// Check if Result node already exists
		UK2Node_FunctionResult* ResultNode = nullptr;
		for (UEdGraphNode* Node : FuncGraph->Nodes)
		{
			ResultNode = Cast<UK2Node_FunctionResult>(Node);
			if (ResultNode)
			{
				break;
			}
		}

		// Create Result node only if it doesn't exist
		if (!ResultNode)
		{
			ResultNode = NewObject<UK2Node_FunctionResult>(FuncGraph);
			FuncGraph->AddNode(ResultNode, false, false);
			ResultNode->NodePosX = 400;
			ResultNode->NodePosY = 0;
			ResultNode->AllocateDefaultPins();

			// Add return value pin
			FEdGraphPinType ReturnPinType = Variable->VarType;
			ResultNode->CreateUserDefinedPin(TEXT("ReturnValue"), ReturnPinType, EGPD_Input);
		}

		// Connect nodes: Entry exec -> (nothing for pure function)
		// GetVar output -> Result ReturnValue
		UEdGraphPin* GetVarOutputPin = GetVarNode->FindPin(FName(*VariableName), EGPD_Output);
		UEdGraphPin* ResultInputPin = ResultNode->FindPin(TEXT("ReturnValue"), EGPD_Input);

		if (GetVarOutputPin && ResultInputPin)
		{
			GetVarOutputPin->MakeLinkTo(ResultInputPin);
		}
	}

	// Note: Property binding setup is complex and depends on UMG internal APIs
	// For Phase 2, we create the binding function but leave full binding integration for Phase 3
	// The user can manually bind in the UMG editor or we'll implement full binding in Phase 3

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("binding_function"), FunctionName);
	ResultObj->SetStringField(TEXT("note"), TEXT("Binding function created. Manual binding in UMG editor may be required for Phase 2."));
	return ResultObj;
}

// ============================================================================
// Phase 3: Animation Operations
// ============================================================================

TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleCreateWidgetAnimation(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetName, AnimationName, Path = TEXT("/Game/UI");
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName) ||
		!Params->TryGetStringField(TEXT("animation_name"), AnimationName))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing required parameters"));
	}
	Params->TryGetStringField(TEXT("path"), Path);

	float Length = 1.0f;
	if (Params->HasField(TEXT("length")))
	{
		Length = static_cast<float>(Params->GetNumberField(TEXT("length")));
	}

	bool bLoop = false;
	Params->TryGetBoolField(TEXT("loop"), bLoop);

	// Load Widget Blueprint
	FString AssetPath = Path + TEXT("/") + WidgetName + TEXT(".") + WidgetName;
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint '%s' not found at path '%s'"), *WidgetName, *AssetPath));
	}

	// Check if animation already exists
	for (UWidgetAnimation* ExistingAnim : WidgetBP->Animations)
	{
		if (ExistingAnim && ExistingAnim->GetFName() == FName(*AnimationName))
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Animation '%s' already exists in Widget Blueprint"), *AnimationName));
		}
	}

	// Create new Widget Animation
	UWidgetAnimation* NewAnimation = NewObject<UWidgetAnimation>(
		WidgetBP,
		UWidgetAnimation::StaticClass(),
		FName(*AnimationName),
		RF_Transactional
	);

	if (!NewAnimation)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create Widget Animation"));
	}

	// Create MovieScene for the animation
	UMovieScene* MovieScene = NewObject<UMovieScene>(
		NewAnimation,
		UMovieScene::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	if (!MovieScene)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create MovieScene"));
	}

	NewAnimation->MovieScene = MovieScene;

	// Set playback range
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameNumber StartFrame = 0;
	FFrameNumber EndFrame = FFrameNumber(static_cast<int32>(Length * TickResolution.AsDecimal()));

	MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartFrame, EndFrame));
	MovieScene->SetWorkingRange(StartFrame.Value / TickResolution.AsDecimal(), EndFrame.Value / TickResolution.AsDecimal());
	MovieScene->SetViewRange(StartFrame.Value / TickResolution.AsDecimal(), EndFrame.Value / TickResolution.AsDecimal());

	// Set loop mode
	if (bLoop)
	{
		MovieScene->SetEvaluationType(EMovieSceneEvaluationType::WithSubFrames);
		// Note: Loop settings are typically controlled at playback time, not in the MovieScene itself
	}

	// Add animation to Widget Blueprint
	WidgetBP->Animations.Add(NewAnimation);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("widget_name"), WidgetName);
	ResultObj->SetStringField(TEXT("animation_name"), AnimationName);
	ResultObj->SetStringField(TEXT("animation_id"), NewAnimation->GetPathName());
	ResultObj->SetNumberField(TEXT("length"), Length);
	ResultObj->SetBoolField(TEXT("loop"), bLoop);

	return ResultObj;
}

// Helper: Find Float Track by Property Name
static UMovieSceneFloatTrack* FindFloatTrackByPropertyName(UMovieScene* MovieScene, const FGuid& BindingGuid, const FString& PropertyName)
{
	if (!MovieScene)
	{
		return nullptr;
	}

	// Find binding by GUID
	const FMovieSceneBinding* Binding = MovieScene->FindBinding(BindingGuid);
	if (!Binding)
	{
		return nullptr;
	}

	// Search through binding's tracks
	for (UMovieSceneTrack* Track : Binding->GetTracks())
	{
		if (UMovieSceneFloatTrack* FloatTrack = Cast<UMovieSceneFloatTrack>(Track))
		{
			// Match by track name or property path
			if (FloatTrack->GetTrackName().ToString() == PropertyName ||
				FloatTrack->GetPropertyPath().ToString() == PropertyName)
			{
				return FloatTrack;
			}
		}
	}

	return nullptr;
}

// Phase 3: Add Animation Track
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddAnimationTrack(const TSharedPtr<FJsonObject>& Params)
{
	// Get parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString TargetWidgetName = Params->GetStringField(TEXT("target_widget"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));
	FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

	// Load Widget Blueprint
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
	}

	// Find the animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBP->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}

	if (!Animation)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->GetMovieScene();
	if (!MovieScene)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("MovieScene not found in animation"));
	}

	// Find target widget
	UWidget* TargetWidget = nullptr;
	if (WidgetBP->WidgetTree)
	{
		TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*TargetWidgetName));
	}

	if (!TargetWidget)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Target widget not found: %s"), *TargetWidgetName));
	}

	// Find or create Possessable (binding target)
	FGuid BindingGuid;
	bool bFoundBinding = false;

	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == TargetWidgetName)
		{
			BindingGuid = Possessable.GetGuid();
			bFoundBinding = true;
			break;
		}
	}

	if (!bFoundBinding)
	{
		// Create new Possessable
		BindingGuid = MovieScene->AddPossessable(TargetWidgetName, TargetWidget->GetClass());

		// Add binding to Animation's AnimationBindings
		FWidgetAnimationBinding Binding;
		Binding.WidgetName = FName(*TargetWidgetName);
		Binding.SlotWidgetName = NAME_None;
		Binding.AnimationGuid = BindingGuid;
		Binding.bIsRootWidget = false;
		Animation->AnimationBindings.Add(Binding);
	}

	// Create track based on property
	FString TrackDisplayName;

	if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
	{
		// Float track (Opacity)
		UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (Track)
		{
			Track->SetPropertyNameAndPath(FName("RenderOpacity"), "RenderOpacity");

			// Add section
			UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
			if (Section)
			{
				Section->SetRange(TRange<FFrameNumber>::All());
				Track->AddSection(*Section);
			}

			TrackDisplayName = TEXT("RenderOpacity");
		}
	}
	else if (PropertyName == TEXT("ColorAndOpacity"))
	{
		// Color track
		UMovieSceneColorTrack* Track = MovieScene->AddTrack<UMovieSceneColorTrack>(BindingGuid);
		if (Track)
		{
			Track->SetPropertyNameAndPath(FName("ColorAndOpacity"), "ColorAndOpacity");

			UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Track->CreateNewSection());
			if (Section)
			{
				Section->SetRange(TRange<FFrameNumber>::All());
				Track->AddSection(*Section);
			}

			TrackDisplayName = TEXT("ColorAndOpacity");
		}
	}
	else if (PropertyName.StartsWith(TEXT("RenderTransform.")))
	{
		// RenderTransform property (Translation.X, Translation.Y, Scale.X, Scale.Y, Angle)
		UMovieSceneFloatTrack* Track = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create float track for RenderTransform"));
		}

		// Set property name and path
		Track->SetPropertyNameAndPath(FName(*PropertyName), PropertyName);

		// Create section
		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
		if (Section)
		{
			Section->SetRange(TRange<FFrameNumber>::All());
			Track->AddSection(*Section);
		}

		TrackDisplayName = PropertyName;
	}
	else
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported property: %s. Supported: Opacity, RenderOpacity, ColorAndOpacity, RenderTransform.*"), *PropertyName));
	}

	// Save
	WidgetBP->MarkPackageDirty();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Create response
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	Response->SetStringField(TEXT("target_widget"), TargetWidgetName);
	Response->SetStringField(TEXT("property_name"), TrackDisplayName);
	Response->SetStringField(TEXT("binding_guid"), BindingGuid.ToString());

	return Response;
}

// Phase 3: Add Animation Keyframe
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddAnimationKeyframe(const TSharedPtr<FJsonObject>& Params)
{
	// Get parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString AnimationName = Params->GetStringField(TEXT("animation_name"));
	FString TargetWidgetName = Params->GetStringField(TEXT("target_widget"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));
	double Time = Params->GetNumberField(TEXT("time"));
	FString Interpolation = Params->HasField(TEXT("interpolation")) ?
		Params->GetStringField(TEXT("interpolation")) : TEXT("Linear");
	FString Path = Params->HasField(TEXT("path")) ?
		Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

	// Load Widget Blueprint
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
	}

	// Find the animation
	UWidgetAnimation* Animation = nullptr;
	for (UWidgetAnimation* Anim : WidgetBP->Animations)
	{
		if (Anim && Anim->GetName() == AnimationName)
		{
			Animation = Anim;
			break;
		}
	}

	if (!Animation)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
	}

	UMovieScene* MovieScene = Animation->GetMovieScene();
	if (!MovieScene)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("MovieScene not found"));
	}

	// Find Possessable binding GUID
	FGuid BindingGuid;
	bool bFoundBinding = false;

	for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		if (Possessable.GetName() == TargetWidgetName)
		{
			BindingGuid = Possessable.GetGuid();
			bFoundBinding = true;
			break;
		}
	}

	if (!bFoundBinding)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("No binding found for widget: %s. Add a track first."), *TargetWidgetName));
	}

	// Calculate frame number
	FFrameRate TickResolution = MovieScene->GetTickResolution();
	FFrameNumber FrameNumber = (Time * TickResolution).FloorToFrame();

	// Determine interpolation type
	ERichCurveInterpMode InterpMode = RCIM_Linear;
	if (Interpolation == TEXT("Cubic"))
	{
		InterpMode = RCIM_Cubic;
	}
	else if (Interpolation == TEXT("Constant"))
	{
		InterpMode = RCIM_Constant;
	}

	// Add keyframe based on property
	if (PropertyName == TEXT("Opacity") || PropertyName == TEXT("RenderOpacity"))
	{
		// Get Float value
		double Value = Params->GetNumberField(TEXT("value"));

		// Find Float track
		UMovieSceneFloatTrack* Track = MovieScene->FindTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				TEXT("Float track not found. Add a track first using add_animation_track."));
		}

		// Get section
		if (Track->GetAllSections().Num() == 0)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in track"));
		}

		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections()[0]);
		if (!Section)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid section"));
		}

		// Get channel and add keyframe
		FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
		if (Channel)
		{
			FMovieSceneFloatValue KeyValue(Value);
			KeyValue.InterpMode = InterpMode;
			Channel->AddKeys({FrameNumber}, {KeyValue});
		}
	}
	else if (PropertyName == TEXT("ColorAndOpacity"))
	{
		// Get Color value [R, G, B, A]
		const TArray<TSharedPtr<FJsonValue>>* ColorArray;
		if (!Params->TryGetArrayField(TEXT("value"), ColorArray) || ColorArray->Num() < 4)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				TEXT("ColorAndOpacity requires [R, G, B, A] array"));
		}

		float R = (*ColorArray)[0]->AsNumber();
		float G = (*ColorArray)[1]->AsNumber();
		float B = (*ColorArray)[2]->AsNumber();
		float A = (*ColorArray)[3]->AsNumber();

		// Find Color track
		UMovieSceneColorTrack* Track = MovieScene->FindTrack<UMovieSceneColorTrack>(BindingGuid);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				TEXT("Color track not found. Add a track first."));
		}

		if (Track->GetAllSections().Num() == 0)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in color track"));
		}

		UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(Track->GetAllSections()[0]);
		if (!Section)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid color section"));
		}

		// Add keyframe to each channel (R, G, B, A)
		TArrayView<FMovieSceneFloatChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
		if (Channels.Num() >= 4)
		{
			FMovieSceneFloatValue RValue(R); RValue.InterpMode = InterpMode;
			FMovieSceneFloatValue GValue(G); GValue.InterpMode = InterpMode;
			FMovieSceneFloatValue BValue(B); BValue.InterpMode = InterpMode;
			FMovieSceneFloatValue AValue(A); AValue.InterpMode = InterpMode;

			Channels[0]->AddKeys({FrameNumber}, {RValue});
			Channels[1]->AddKeys({FrameNumber}, {GValue});
			Channels[2]->AddKeys({FrameNumber}, {BValue});
			Channels[3]->AddKeys({FrameNumber}, {AValue});
		}
	}
	else if (PropertyName.StartsWith(TEXT("RenderTransform.")))
	{
		// RenderTransform property (Translation.X, Translation.Y, Scale.X, Scale.Y, Angle)
		double Value = Params->GetNumberField(TEXT("value"));

		// Find Float track by property name
		UMovieSceneFloatTrack* Track = FindFloatTrackByPropertyName(MovieScene, BindingGuid, PropertyName);
		if (!Track)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Float track not found for property: %s. Add a track first."), *PropertyName));
		}

		// Get section
		if (Track->GetAllSections().Num() == 0)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("No section found in track"));
		}

		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(Track->GetAllSections()[0]);
		if (!Section)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Invalid section"));
		}

		// Get channel and add keyframe
		FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
		if (Channel)
		{
			FMovieSceneFloatValue KeyValue(Value);
			KeyValue.InterpMode = InterpMode;
			Channel->AddKeys({FrameNumber}, {KeyValue});
		}
	}
	else
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported property for keyframe: %s"), *PropertyName));
	}

	// Save
	WidgetBP->MarkPackageDirty();

	// Response
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("animation_name"), AnimationName);
	Response->SetStringField(TEXT("target_widget"), TargetWidgetName);
	Response->SetStringField(TEXT("property_name"), PropertyName);
	Response->SetNumberField(TEXT("time"), Time);
	Response->SetNumberField(TEXT("frame"), FrameNumber.Value);
	Response->SetStringField(TEXT("interpolation"), Interpolation);

	return Response;
}

// Phase 3: Get Widget Animations
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleGetWidgetAnimations(const TSharedPtr<FJsonObject>& Params)
{
	// Get parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString Path = Params->HasField(TEXT("path")) ?
		Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

	// Load Widget Blueprint
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
	}

	// Build animations array
	TArray<TSharedPtr<FJsonValue>> AnimationsArray;

	for (UWidgetAnimation* Animation : WidgetBP->Animations)
	{
		if (!Animation) continue;

		TSharedPtr<FJsonObject> AnimObj = MakeShareable(new FJsonObject());
		AnimObj->SetStringField(TEXT("name"), Animation->GetName());

		UMovieScene* MovieScene = Animation->GetMovieScene();
		if (MovieScene)
		{
			// Get animation length
			FFrameRate TickResolution = MovieScene->GetTickResolution();
			FFrameRate DisplayRate = MovieScene->GetDisplayRate();
			TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();

			double StartTime = TickResolution.AsSeconds(PlaybackRange.GetLowerBoundValue());
			double EndTime = TickResolution.AsSeconds(PlaybackRange.GetUpperBoundValue());
			double Length = EndTime - StartTime;

			AnimObj->SetNumberField(TEXT("length"), Length);

			// Check if looping (this info is typically stored externally, default to false)
			AnimObj->SetBoolField(TEXT("is_looping"), false);

			// Get tracks
			TArray<TSharedPtr<FJsonValue>> TracksArray;

			for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
			{
				const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
				FGuid BindingGuid = Possessable.GetGuid();
				FString TargetName = Possessable.GetName();

				// Check for Float tracks (Opacity)
				if (UMovieSceneFloatTrack* FloatTrack = MovieScene->FindTrack<UMovieSceneFloatTrack>(BindingGuid))
				{
					TSharedPtr<FJsonObject> TrackObj = MakeShareable(new FJsonObject());
					TrackObj->SetStringField(TEXT("target_widget"), TargetName);
					TrackObj->SetStringField(TEXT("property"), FloatTrack->GetTrackName().ToString());
					TrackObj->SetStringField(TEXT("type"), TEXT("Float"));

					int32 KeyframeCount = 0;
					if (FloatTrack->GetAllSections().Num() > 0)
					{
						UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(FloatTrack->GetAllSections()[0]);
						if (Section)
						{
							FMovieSceneFloatChannel* Channel = Section->GetChannelProxy().GetChannel<FMovieSceneFloatChannel>(0);
							if (Channel)
							{
								KeyframeCount = Channel->GetNumKeys();
							}
						}
					}
					TrackObj->SetNumberField(TEXT("keyframe_count"), KeyframeCount);

					TracksArray.Add(MakeShareable(new FJsonValueObject(TrackObj)));
				}

				// Check for Color tracks (ColorAndOpacity)
				if (UMovieSceneColorTrack* ColorTrack = MovieScene->FindTrack<UMovieSceneColorTrack>(BindingGuid))
				{
					TSharedPtr<FJsonObject> TrackObj = MakeShareable(new FJsonObject());
					TrackObj->SetStringField(TEXT("target_widget"), TargetName);
					TrackObj->SetStringField(TEXT("property"), ColorTrack->GetTrackName().ToString());
					TrackObj->SetStringField(TEXT("type"), TEXT("Color"));

					int32 KeyframeCount = 0;
					if (ColorTrack->GetAllSections().Num() > 0)
					{
						UMovieSceneColorSection* Section = Cast<UMovieSceneColorSection>(ColorTrack->GetAllSections()[0]);
						if (Section)
						{
							// Get keyframe count from first channel (R)
							TArrayView<FMovieSceneFloatChannel*> Channels = Section->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
							if (Channels.Num() > 0)
							{
								KeyframeCount = Channels[0]->GetNumKeys();
							}
						}
					}
					TrackObj->SetNumberField(TEXT("keyframe_count"), KeyframeCount);

					TracksArray.Add(MakeShareable(new FJsonValueObject(TrackObj)));
				}
			}

			AnimObj->SetNumberField(TEXT("track_count"), TracksArray.Num());
			AnimObj->SetArrayField(TEXT("tracks"), TracksArray);
		}
		else
		{
			AnimObj->SetNumberField(TEXT("length"), 0.0);
			AnimObj->SetBoolField(TEXT("is_looping"), false);
			AnimObj->SetNumberField(TEXT("track_count"), 0);
			AnimObj->SetArrayField(TEXT("tracks"), TArray<TSharedPtr<FJsonValue>>());
		}

		AnimationsArray.Add(MakeShareable(new FJsonValueObject(AnimObj)));
	}

	// Response
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetNumberField(TEXT("animation_count"), AnimationsArray.Num());
	Response->SetArrayField(TEXT("animations"), AnimationsArray);

	return Response;
}

// Phase 3: Add Widget Array Variable
TSharedPtr<FJsonObject> FSpirrowBridgeUMGCommands::HandleAddWidgetArrayVariable(const TSharedPtr<FJsonObject>& Params)
{
	// Get parameters
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString VariableName = Params->GetStringField(TEXT("variable_name"));
	FString ElementType = Params->GetStringField(TEXT("element_type"));
	bool bIsExposed = Params->HasField(TEXT("is_exposed")) ? Params->GetBoolField(TEXT("is_exposed")) : false;
	FString Category = Params->HasField(TEXT("category")) ? Params->GetStringField(TEXT("category")) : TEXT("");
	FString Path = Params->HasField(TEXT("path")) ? Params->GetStringField(TEXT("path")) : TEXT("/Game/UI");

	// Load Widget Blueprint
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *WidgetName, *WidgetName);
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget Blueprint not found: %s"), *AssetPath));
	}

	// Get Blueprint class
	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(WidgetBP->GeneratedClass);
	if (!BPClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get Blueprint generated class"));
	}

	// Check if variable already exists
	for (const FBPVariableDescription& Var : WidgetBP->NewVariables)
	{
		if (Var.VarName == *VariableName)
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Variable '%s' already exists"), *VariableName));
		}
	}

	// Set up the element pin type
	FEdGraphPinType ElementPinType;
	if (!SetupPinType(ElementType, ElementPinType))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unsupported element type: %s"), *ElementType));
	}

	// Create array pin type
	FEdGraphPinType ArrayPinType;
	ArrayPinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;  // Will be overwritten
	ArrayPinType.ContainerType = EPinContainerType::Array;

	// Copy element type info to array type
	ArrayPinType.PinCategory = ElementPinType.PinCategory;
	ArrayPinType.PinSubCategory = ElementPinType.PinSubCategory;
	ArrayPinType.PinSubCategoryObject = ElementPinType.PinSubCategoryObject;
	ArrayPinType.PinSubCategoryMemberReference = ElementPinType.PinSubCategoryMemberReference;

	// Create the variable description
	FBPVariableDescription NewVar;
	NewVar.VarName = *VariableName;
	NewVar.VarGuid = FGuid::NewGuid();
	NewVar.VarType = ArrayPinType;
	NewVar.PropertyFlags = CPF_Edit | CPF_BlueprintVisible | CPF_DisableEditOnInstance;

	if (bIsExposed)
	{
		NewVar.PropertyFlags |= CPF_ExposeOnSpawn;
	}

	if (!Category.IsEmpty())
	{
		NewVar.Category = FText::FromString(Category);
	}

	// Add to Blueprint
	WidgetBP->NewVariables.Add(NewVar);

	// Compile
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	// Mark dirty
	WidgetBP->MarkPackageDirty();

	// Response
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("variable_name"), VariableName);
	Response->SetStringField(TEXT("variable_type"), FString::Printf(TEXT("TArray<%s>"), *ElementType));
	Response->SetStringField(TEXT("element_type"), ElementType);
	Response->SetBoolField(TEXT("is_exposed"), bIsExposed);

	return Response;
}

// Phase 4-A: Add Button Widget V2
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

// Phase 4-A: Bind Widget Component Event
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

	// Ensure widget is bound as a variable first
	FObjectProperty* WidgetProperty = FindFProperty<FObjectProperty>(WidgetBP->SkeletonGeneratedClass, FName(*ComponentName));
	if (!WidgetProperty)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget '%s' must be bound as a variable first. Use bind_widget_to_variable tool."), *ComponentName));
	}

	// Create component bound event using FKismetEditorUtilities
	FKismetEditorUtilities::CreateNewBoundEventForComponent(
		WidgetComponent,
		EventPropertyName,
		WidgetBP,
		WidgetProperty
	);

	// Find the created event node in the EventGraph
	UEdGraphNode* EventNode = nullptr;
	FString ExpectedNodeName = ComponentName + TEXT("_") + EventType;
	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		if (UK2Node_ComponentBoundEvent* BoundEventNode = Cast<UK2Node_ComponentBoundEvent>(Node))
		{
			if (BoundEventNode->DelegatePropertyName == EventPropertyName &&
				BoundEventNode->ComponentPropertyName == FName(*ComponentName))
			{
				EventNode = BoundEventNode;
				break;
			}
		}
	}

	if (!EventNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to find created event node"));
	}

	// If we have a function, connect the event to call it
	if (FuncGraph && EventNode)
	{
		// Find the exec output pin of the event node
		UEdGraphPin* EventExecPin = nullptr;
		for (UEdGraphPin* Pin : EventNode->Pins)
		{
			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
			{
				EventExecPin = Pin;
				break;
			}
		}

		if (EventExecPin)
		{
			// Create call function node
			UK2Node_CallFunction* CallFuncNode = NewObject<UK2Node_CallFunction>(EventGraph);
			UFunction* TargetFunction = WidgetBP->GeneratedClass->FindFunctionByName(FName(*FunctionName));
			if (TargetFunction)
			{
				CallFuncNode->SetFromFunction(TargetFunction);
				EventGraph->AddNode(CallFuncNode, false, false);
				CallFuncNode->NodePosX = EventNode->NodePosX + 300;
				CallFuncNode->NodePosY = EventNode->NodePosY;
				CallFuncNode->AllocateDefaultPins();

				// Connect exec pins
				UEdGraphPin* FuncExecPin = CallFuncNode->FindPin(UEdGraphSchema_K2::PN_Execute);
				if (FuncExecPin)
				{
					EventExecPin->MakeLinkTo(FuncExecPin);
				}
			}
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
	if (EventNode)
	{
		ResultObj->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
	}
	return ResultObj;
}

// Phase 4-A: Add Slider Widget
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

// Phase 4-A: Add CheckBox Widget
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
