#include "Commands/SpirrowBridgeUMGWidgetCoreCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

FSpirrowBridgeUMGWidgetCoreCommands::FSpirrowBridgeUMGWidgetCoreCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCoreCommands::HandleCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandName == TEXT("create_umg_widget_blueprint"))
	{
		return HandleCreateUMGWidgetBlueprint(Params);
	}
	else if (CommandName == TEXT("add_widget_to_viewport"))
	{
		return HandleAddWidgetToViewport(Params);
	}

	// Not handled by this class
	return nullptr;
}

FAnchors FSpirrowBridgeUMGWidgetCoreCommands::ParseAnchorPreset(const FString& AnchorStr)
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

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCoreCommands::HandleCreateUMGWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters - support both "widget_name" and "name" for compatibility
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("widget_name"), BlueprintName))
	{
		// Try alternative parameter name
		if (auto Error2 = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), BlueprintName))
		{
			return Error; // Return original error
		}
	}

	// Get optional parameters
	FString PackagePath, ParentClassName;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), PackagePath, TEXT("/Game/UI"));
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("parent_class"), ParentClassName, TEXT("UserWidget"));

	// Ensure path ends without trailing slash
	if (PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath = PackagePath.LeftChop(1);
	}

	// Resolve parent class. Accepts:
	//   - Bare shortcut "UserWidget"
	//   - C++ class path "/Script/Module.Class"
	//   - Blueprint class path "/Game/Path.Asset_C" or asset path "/Game/Path.Asset"
	//   - Bare class name (falls back to legacy /Script/UMG.<Name> lookup)
	// Resolution mirrors FSpirrowBridgeCommonUtils::SetObjectProperty's FClassProperty branch.
	UClass* ParentClass = nullptr;
	if (ParentClassName == TEXT("UserWidget"))
	{
		ParentClass = UUserWidget::StaticClass();
	}
	else
	{
		// 1. Direct class path load (handles /Script/Module.Class and /Game/Path.Asset_C)
		ParentClass = LoadClass<UObject>(nullptr, *ParentClassName);

		// 2. Try asset load, extract Blueprint->GeneratedClass (handles /Game/Path.Asset form)
		if (!ParentClass)
		{
			UObject* Loaded = UEditorAssetLibrary::LoadAsset(ParentClassName);
			if (UBlueprint* BP = Cast<UBlueprint>(Loaded))
			{
				ParentClass = BP->GeneratedClass;
			}
			else if (UClass* DirectClass = Cast<UClass>(Loaded))
			{
				ParentClass = DirectClass;
			}
		}

		// 3. Suffix-completion: "/Game/UI/BP_Foo" -> "/Game/UI/BP_Foo.BP_Foo"
		if (!ParentClass && !ParentClassName.Contains(TEXT(".")))
		{
			FString AssetName = FPackageName::GetShortName(ParentClassName);
			FString AltPath = ParentClassName + TEXT(".") + AssetName;
			UObject* Loaded = UEditorAssetLibrary::LoadAsset(AltPath);
			if (UBlueprint* BP = Cast<UBlueprint>(Loaded))
			{
				ParentClass = BP->GeneratedClass;
			}
		}

		// 4. Legacy /Script/UMG.<Name> backwards-compat fallback
		if (!ParentClass)
		{
			FString ScriptPath = FString::Printf(TEXT("/Script/UMG.%s"), *ParentClassName);
			ParentClass = LoadClass<UObject>(nullptr, *ScriptPath);
		}

		// 5. Unresolved -> hard error (no silent fallback)
		if (!ParentClass)
		{
			TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
			Details->SetStringField(TEXT("parent_class"), ParentClassName);
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::ClassNotFound,
				FString::Printf(TEXT("Parent class '%s' not found (tried direct load, asset load, and /Script/UMG.%s)"),
					*ParentClassName, *ParentClassName),
				Details);
		}
	}

	// Validate that resolved class descends from UUserWidget
	if (!ParentClass->IsChildOf(UUserWidget::StaticClass()))
	{
		TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
		Details->SetStringField(TEXT("parent_class"), ParentClassName);
		Details->SetStringField(TEXT("resolved_class"), ParentClass->GetPathName());
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParamValue,
			FString::Printf(TEXT("Parent class '%s' does not descend from UUserWidget (resolved: %s)"),
				*ParentClassName, *ParentClass->GetPathName()),
			Details);
	}

	// Create the full asset path
	FString AssetName = BlueprintName;
	FString FullPath = PackagePath + TEXT("/") + AssetName;

	// Check if asset already exists
	if (UEditorAssetLibrary::DoesAssetExist(FullPath))
	{
		TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
		Details->SetStringField(TEXT("widget_name"), BlueprintName);
		Details->SetStringField(TEXT("path"), FullPath);
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetAlreadyExists,
			FString::Printf(TEXT("Widget Blueprint '%s' already exists at '%s'"), *BlueprintName, *FullPath),
			Details);
	}

	// Create package
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create package"));
	}

	// Create Widget Blueprint
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		Package,
		FName(*AssetName),
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		FName("CreateUMGWidget")
	);

	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewBlueprint);
	if (!WidgetBlueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			FString::Printf(TEXT("Failed to create Widget Blueprint '%s'"), *BlueprintName));
	}

	// Add default Canvas Panel
	if (!WidgetBlueprint->WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		WidgetBlueprint->WidgetTree->RootWidget = RootCanvas;
	}

	// Mark and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(WidgetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

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
	ResultObj->SetStringField(TEXT("parent_class"), ParentClass->GetPathName());
	return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeUMGWidgetCoreCommands::HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
	{
		return Error;
	}

	// Find the Widget Blueprint (legacy path)
	FString FullPath = TEXT("/Game/Widgets/") + BlueprintName;
	UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));
	if (!WidgetBlueprint)
	{
		TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
		Details->SetStringField(TEXT("blueprint_name"), BlueprintName);
		Details->SetStringField(TEXT("path"), TEXT("/Game/Widgets"));
		Details->SetStringField(TEXT("full_path"), FullPath);
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetNotFound,
			FString::Printf(TEXT("Widget Blueprint '%s' not found"), *BlueprintName),
			Details);
	}

	// Get optional Z-order parameter
	double ZOrder;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("z_order"), ZOrder, 0.0);

	UClass* WidgetClass = WidgetBlueprint->GeneratedClass;
	if (!WidgetClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::WidgetCreationFailed,
			TEXT("Failed to get widget class"));
	}

	// Create success response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetStringField(TEXT("class_path"), WidgetClass->GetPathName());
	ResultObj->SetNumberField(TEXT("z_order"), static_cast<int32>(ZOrder));
	ResultObj->SetStringField(TEXT("note"), TEXT("Widget class ready. Use CreateWidget and AddToViewport nodes in Blueprint to display in game."));
	return ResultObj;
}
