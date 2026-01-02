#include "Commands/SpirrowBridgeUMGVariableCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

FSpirrowBridgeUMGVariableCommands::FSpirrowBridgeUMGVariableCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("add_widget_variable"))
	{
		return HandleAddWidgetVariable(Params);
	}
	else if (CommandName == TEXT("add_widget_array_variable"))
	{
		return HandleAddWidgetArrayVariable(Params);
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
	else if (CommandName == TEXT("bind_widget_event"))
	{
		return HandleBindWidgetEvent(Params);
	}
	else if (CommandName == TEXT("set_text_block_binding"))
	{
		return HandleSetTextBlockBinding(Params);
	}
	else if (CommandName == TEXT("bind_widget_component_event"))
	{
		return HandleBindWidgetComponentEvent(Params);
	}

	return nullptr;
}

bool FSpirrowBridgeUMGVariableCommands::SetupPinType(const FString& TypeName, FEdGraphPinType& OutPinType)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleAddWidgetVariable(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleSetWidgetVariableDefault(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleAddWidgetFunction(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleAddWidgetEvent(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleBindWidgetToVariable(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleBindWidgetEvent(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleSetTextBlockBinding(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleAddWidgetArrayVariable(const TSharedPtr<FJsonObject>& Params)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGVariableCommands::HandleBindWidgetComponentEvent(const TSharedPtr<FJsonObject>& Params)
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
