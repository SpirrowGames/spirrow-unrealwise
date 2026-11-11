// Function graph commands (Issue #21): add_function_graph, add_function_override,
// list_blueprint_graphs. They are members of FSpirrowBridgeBlueprintNodeCoreCommands
// and live in their own file only to keep SpirrowBridgeBlueprintNodeCoreCommands.cpp
// from growing further.

#include "Commands/SpirrowBridgeBlueprintNodeCoreCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"

namespace
{
    struct FRequestedPin
    {
        FName Name;
        FEdGraphPinType Type;
    };

    /** Parse [{"name": ..., "type": ...}, ...]. Returns an error response, or null on success. */
    TSharedPtr<FJsonObject> ParsePinList(const TSharedPtr<FJsonObject>& Params, const TCHAR* Field, TArray<FRequestedPin>& OutPins)
    {
        const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
        if (!Params->TryGetArrayField(Field, Array) || !Array)
        {
            return nullptr;
        }

        for (int32 Index = 0; Index < Array->Num(); ++Index)
        {
            const TSharedPtr<FJsonObject>* Entry = nullptr;
            FString Name, TypeName;
            if (!(*Array)[Index]->TryGetObject(Entry) || !Entry || !(*Entry)->TryGetStringField(TEXT("name"), Name)
                || !(*Entry)->TryGetStringField(TEXT("type"), TypeName) || Name.IsEmpty())
            {
                return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                    ESpirrowErrorCode::InvalidParams,
                    FString::Printf(TEXT("%s[%d] must be an object with 'name' and 'type'"), Field, Index));
            }

            FRequestedPin Pin;
            Pin.Name = FName(*Name);
            FString TypeError;
            if (!FSpirrowBridgeCommonUtils::ResolvePinTypeFromString(TypeName, Pin.Type, TypeError))
            {
                return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                    ESpirrowErrorCode::InvalidParamValue,
                    FString::Printf(TEXT("%s[%d] '%s': %s"), Field, Index, *Name, *TypeError));
            }
            for (const FRequestedPin& Existing : OutPins)
            {
                if (Existing.Name == Pin.Name)
                {
                    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                        ESpirrowErrorCode::InvalidParamValue,
                        FString::Printf(TEXT("%s has two entries named '%s'"), Field, *Name));
                }
            }
            OutPins.Add(Pin);
        }
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> DescribePins(const UEdGraphNode* Node, EEdGraphPinDirection Direction)
    {
        TArray<TSharedPtr<FJsonValue>> Out;
        if (!Node)
        {
            return Out;
        }
        for (const UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin || Pin->Direction != Direction || Pin->bHidden)
            {
                continue;
            }
            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("full_type"), FSpirrowBridgeCommonUtils::DescribePinType(Pin->PinType));
            Out.Add(MakeShared<FJsonValueObject>(PinObj));
        }
        return Out;
    }

    /** Common tail of add_function_graph / add_function_override. */
    TSharedPtr<FJsonObject> DescribeFunctionGraph(const UEdGraph* Graph)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), true);
        ResultObj->SetStringField(TEXT("graph_name"), Graph->GetName());

        TArray<UK2Node_FunctionEntry*> Entries;
        Graph->GetNodesOfClass(Entries);
        if (Entries.Num() > 0)
        {
            ResultObj->SetStringField(TEXT("entry_node_id"), Entries[0]->NodeGuid.ToString());
            // The entry node's output pins are the function's inputs.
            ResultObj->SetArrayField(TEXT("entry_pins"), DescribePins(Entries[0], EGPD_Output));
        }

        TArray<UK2Node_FunctionResult*> Results;
        Graph->GetNodesOfClass(Results);
        if (Results.Num() > 0)
        {
            ResultObj->SetStringField(TEXT("result_node_id"), Results[0]->NodeGuid.ToString());
            // The result node's input pins are the function's outputs.
            ResultObj->SetArrayField(TEXT("result_pins"), DescribePins(Results[0], EGPD_Input));
        }
        return ResultObj;
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleAddFunctionGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString FunctionName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("function_name"), FunctionName))
    {
        return Error;
    }

    // Parse and validate every pin before touching the Blueprint, so a bad
    // type cannot leave a half-built function behind.
    TArray<FRequestedPin> Inputs, Outputs;
    if (auto Error = ParsePinList(Params, TEXT("inputs"), Inputs))
    {
        return Error;
    }
    if (auto Error = ParsePinList(Params, TEXT("outputs"), Outputs))
    {
        return Error;
    }

    bool bIsPure = false;
    Params->TryGetBoolField(TEXT("is_pure"), bIsPure);

    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    if (FSpirrowBridgeCommonUtils::FindGraphByName(Blueprint, FunctionName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeAlreadyExists,
            FString::Printf(TEXT("Blueprint already has a graph named '%s'"), *FunctionName));
    }
    // A user function with the name of an inherited function would be an
    // override with the wrong signature. That is add_function_override's job.
    if (Blueprint->ParentClass && Blueprint->ParentClass->FindFunctionByName(FName(*FunctionName)))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            FString::Printf(TEXT("'%s' is already a function of the parent class %s. Use add_function_override to override it."),
                *FunctionName, *Blueprint->ParentClass->GetName()));
    }

    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, FName(*FunctionName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    if (!NewGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed, TEXT("Failed to create the function graph"));
    }
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, NewGraph, /*bIsUserCreated=*/ true, nullptr);

    TArray<UK2Node_FunctionEntry*> Entries;
    NewGraph->GetNodesOfClass(Entries);
    if (Entries.Num() == 0)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed, TEXT("The new function graph has no entry node"));
    }
    UK2Node_FunctionEntry* Entry = Entries[0];

    if (bIsPure)
    {
        Entry->AddExtraFlags(FUNC_BlueprintPure);
    }
    for (const FRequestedPin& Input : Inputs)
    {
        Entry->CreateUserDefinedPin(Input.Name, Input.Type, EGPD_Output, /*bUseUniqueName=*/ false);
    }
    if (Outputs.Num() > 0)
    {
        if (UK2Node_FunctionResult* Result = FBlueprintEditorUtils::FindOrCreateFunctionResultNode(Entry))
        {
            for (const FRequestedPin& Output : Outputs)
            {
                Result->CreateUserDefinedPin(Output.Name, Output.Type, EGPD_Input, /*bUseUniqueName=*/ false);
            }
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = DescribeFunctionGraph(NewGraph);
    ResultObj->SetBoolField(TEXT("is_override"), false);
    ResultObj->SetBoolField(TEXT("is_pure"), bIsPure);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleAddFunctionOverride(const TSharedPtr<FJsonObject>& Params)
{
    FString FunctionName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("function_name"), FunctionName))
    {
        return Error;
    }

    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    // Same lookup the editor's "Override Function" menu uses: parent classes
    // and implemented interfaces.
    UFunction* OverrideFunction = nullptr;
    UClass* OverrideClass = FBlueprintEditorUtils::GetOverrideFunctionClass(Blueprint, FName(*FunctionName), &OverrideFunction);

    // The caller may have used the editor name ("CanJump") instead of the
    // function name ("CanJumpInternal"). The editor name can also be the real
    // name of a different, non-overridable function (ACharacter::CanJump), so
    // the lookup by DisplayName also runs when the direct hit cannot be overridden.
    if ((!OverrideFunction || !UEdGraphSchema_K2::CanKismetOverrideFunction(OverrideFunction)) && Blueprint->ParentClass)
    {
        for (TFieldIterator<UFunction> It(Blueprint->ParentClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
        {
            if (It->HasAnyFunctionFlags(FUNC_BlueprintEvent)
                && It->GetMetaData(TEXT("DisplayName")).Equals(FunctionName, ESearchCase::IgnoreCase))
            {
                UFunction* ByDisplayName = nullptr;
                UClass* ByDisplayNameClass = FBlueprintEditorUtils::GetOverrideFunctionClass(Blueprint, It->GetFName(), &ByDisplayName);
                if (ByDisplayName && ByDisplayNameClass)
                {
                    OverrideFunction = ByDisplayName;
                    OverrideClass = ByDisplayNameClass;
                }
                break;
            }
        }
    }

    if (!OverrideFunction || !OverrideClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::FunctionNotFound,
            FString::Printf(TEXT("'%s' is not a function of the parent classes or interfaces of this Blueprint"), *FunctionName));
    }
    if (!UEdGraphSchema_K2::CanKismetOverrideFunction(OverrideFunction))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            FString::Printf(TEXT("'%s' cannot be overridden in a Blueprint (it is not a BlueprintImplementableEvent / BlueprintNativeEvent)"),
                *OverrideFunction->GetName()));
    }
    if (UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(OverrideFunction))
    {
        // The editor overrides these with an event node in the event graph,
        // and so does add_blueprint_event_node.
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidOperation,
            FString::Printf(TEXT("'%s' has no return value or out parameters, so it is overridden with an event node. Use add_blueprint_event_node."),
                *OverrideFunction->GetName()));
    }

    const FName GraphName = OverrideFunction->GetFName();
    if (FSpirrowBridgeCommonUtils::FindGraphByName(Blueprint, GraphName.ToString()))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeAlreadyExists,
            FString::Printf(TEXT("'%s' is already overridden in this Blueprint"), *GraphName.ToString()));
    }

    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, GraphName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    if (!NewGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed, TEXT("Failed to create the function graph"));
    }
    // bIsUserCreated=false + the signature class: the entry and result nodes
    // take the parent function's signature and are not editable.
    FBlueprintEditorUtils::AddFunctionGraph(Blueprint, NewGraph, /*bIsUserCreated=*/ false, OverrideClass);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = DescribeFunctionGraph(NewGraph);
    ResultObj->SetBoolField(TEXT("is_override"), true);
    ResultObj->SetStringField(TEXT("parent_class"), OverrideClass->GetName());
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeBlueprintNodeCoreCommands::HandleListBlueprintGraphs(const TSharedPtr<FJsonObject>& Params)
{
    UBlueprint* Blueprint = nullptr;
    if (auto Error = FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, Blueprint))
    {
        return Error;
    }

    TArray<TSharedPtr<FJsonValue>> Graphs;
    auto AddGraphs = [&Graphs](const TArray<TObjectPtr<UEdGraph>>& Source, const TCHAR* Type)
    {
        for (const UEdGraph* Graph : Source)
        {
            if (!Graph)
            {
                continue;
            }
            TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
            GraphObj->SetStringField(TEXT("graph_name"), Graph->GetName());
            GraphObj->SetStringField(TEXT("graph_type"), Type);
            GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
            Graphs.Add(MakeShared<FJsonValueObject>(GraphObj));
        }
    };
    AddGraphs(Blueprint->UbergraphPages, TEXT("event"));
    AddGraphs(Blueprint->FunctionGraphs, TEXT("function"));
    AddGraphs(Blueprint->MacroGraphs, TEXT("macro"));

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("graphs"), Graphs);
    ResultObj->SetNumberField(TEXT("count"), Graphs.Num());
    return ResultObj;
}
