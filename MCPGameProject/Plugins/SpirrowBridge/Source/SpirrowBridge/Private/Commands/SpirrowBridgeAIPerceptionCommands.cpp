#include "Commands/SpirrowBridgeAIPerceptionCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// Actor includes
#include "GameFramework/Actor.h"

// Blueprint includes
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"

// AI Perception includes
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISenseConfig.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"

FSpirrowBridgeAIPerceptionCommands::FSpirrowBridgeAIPerceptionCommands()
{
}

FSpirrowBridgeAIPerceptionCommands::~FSpirrowBridgeAIPerceptionCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeAIPerceptionCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("add_ai_perception_component"))
	{
		return HandleAddAIPerceptionComponent(Params);
	}
	else if (CommandType == TEXT("configure_sight_sense"))
	{
		return HandleConfigureSightSense(Params);
	}
	else if (CommandType == TEXT("configure_hearing_sense"))
	{
		return HandleConfigureHearingSense(Params);
	}
	else if (CommandType == TEXT("configure_damage_sense"))
	{
		return HandleConfigureDamageSense(Params);
	}
	else if (CommandType == TEXT("set_perception_dominant_sense"))
	{
		return HandleSetPerceptionDominantSense(Params);
	}
	else if (CommandType == TEXT("add_perception_stimuli_source"))
	{
		return HandleAddPerceptionStimuliSource(Params);
	}

	return FSpirrowBridgeCommonUtils::CreateErrorResponse(
		ESpirrowErrorCode::UnknownCommand,
		FString::Printf(TEXT("Unknown AIPerception command: %s"), *CommandType)
	);
}

TSharedPtr<FJsonObject> FSpirrowBridgeAIPerceptionCommands::HandleAddAIPerceptionComponent(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
	{
		return Error;
	}

	FString ComponentName;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("component_name"), ComponentName, TEXT("AIPerceptionComponent"));
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

	// Find the Blueprint
	FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));

	if (!Blueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::BlueprintNotFound,
			FString::Printf(TEXT("Blueprint not found: %s"), *FullPath)
		);
	}

	// Check if component already exists
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->GetVariableName() == FName(*ComponentName))
			{
				return FSpirrowBridgeCommonUtils::CreateErrorResponse(
					ESpirrowErrorCode::InvalidOperation,
					FString::Printf(TEXT("Component '%s' already exists in Blueprint"), *ComponentName)
				);
			}
		}
	}

	// Create SCS node for AIPerceptionComponent
	USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(UAIPerceptionComponent::StaticClass(), FName(*ComponentName));
	if (!NewNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::ComponentCreationFailed,
			TEXT("Failed to create AIPerceptionComponent node")
		);
	}

	// Set the variable name
	NewNode->SetVariableName(FName(*ComponentName));

	// Add to root or default scene root
	Blueprint->SimpleConstructionScript->AddNode(NewNode);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Save the asset
	UEditorAssetLibrary::SaveAsset(FullPath, false);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("blueprint_name"), BlueprintName);
	Response->SetStringField(TEXT("component_name"), ComponentName);
	Response->SetStringField(TEXT("component_type"), TEXT("AIPerceptionComponent"));

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAIPerceptionCommands::HandleConfigureSightSense(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
	{
		return Error;
	}

	FString ComponentName;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("component_name"), ComponentName, TEXT("AIPerceptionComponent"));
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

	// Get optional sense parameters with defaults
	double SightRadius = 3000.0;
	double LoseSightRadius = 3500.0;
	double PeripheralVisionAngle = 90.0;
	double AutoSuccessRange = 500.0;
	double MaxAge = 5.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("sight_radius"), SightRadius, 3000.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("lose_sight_radius"), LoseSightRadius, 3500.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("peripheral_vision_angle"), PeripheralVisionAngle, 90.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("auto_success_range"), AutoSuccessRange, 500.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("max_age"), MaxAge, 5.0);

	// Parse detection by affiliation
	bool bDetectEnemies = true;
	bool bDetectNeutrals = true;
	bool bDetectFriendlies = false;

	if (Params->HasField(TEXT("detection_by_affiliation")))
	{
		TSharedPtr<FJsonObject> AffiliationJson = Params->GetObjectField(TEXT("detection_by_affiliation"));
		ParseDetectionAffiliation(AffiliationJson, bDetectEnemies, bDetectNeutrals, bDetectFriendlies);
	}

	// Find the Blueprint
	FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));

	if (!Blueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::BlueprintNotFound,
			FString::Printf(TEXT("Blueprint not found: %s"), *FullPath)
		);
	}

	// Find the AIPerceptionComponent
	UAIPerceptionComponent* PerceptionComp = FindPerceptionComponent(Blueprint, ComponentName);
	if (!PerceptionComp)
	{
		// Try to find it in the CDO
		UClass* GeneratedClass = Blueprint->GeneratedClass;
		if (GeneratedClass)
		{
			AActor* CDO = Cast<AActor>(GeneratedClass->GetDefaultObject());
			if (CDO)
			{
				PerceptionComp = CDO->FindComponentByClass<UAIPerceptionComponent>();
			}
		}
	}

	// If still not found, try to configure via SCS node template
	USCS_Node* PerceptionNode = nullptr;
	if (!PerceptionComp && Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->ComponentClass && Node->ComponentClass->IsChildOf(UAIPerceptionComponent::StaticClass()))
			{
				if (Node->GetVariableName() == FName(*ComponentName) || ComponentName == TEXT("AIPerceptionComponent"))
				{
					PerceptionNode = Node;
					PerceptionComp = Cast<UAIPerceptionComponent>(Node->ComponentTemplate);
					break;
				}
			}
		}
	}

	if (!PerceptionComp)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::ComponentNotFound,
			FString::Printf(TEXT("AIPerceptionComponent '%s' not found in Blueprint"), *ComponentName)
		);
	}

	// Create and configure sight sense config
	UAISenseConfig_Sight* SightConfig = NewObject<UAISenseConfig_Sight>(PerceptionComp, UAISenseConfig_Sight::StaticClass());
	SightConfig->SightRadius = static_cast<float>(SightRadius);
	SightConfig->LoseSightRadius = static_cast<float>(LoseSightRadius);
	SightConfig->PeripheralVisionAngleDegrees = static_cast<float>(PeripheralVisionAngle);
	SightConfig->DetectionByAffiliation.bDetectEnemies = bDetectEnemies;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = bDetectNeutrals;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	SightConfig->SetMaxAge(static_cast<float>(MaxAge));
	SightConfig->AutoSuccessRangeFromLastSeenLocation = static_cast<float>(AutoSuccessRange);

	// Configure the sense
	PerceptionComp->ConfigureSense(*SightConfig);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Save the asset
	UEditorAssetLibrary::SaveAsset(FullPath, false);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("sense_type"), TEXT("Sight"));
	Response->SetNumberField(TEXT("sight_radius"), SightRadius);
	Response->SetNumberField(TEXT("lose_sight_radius"), LoseSightRadius);
	Response->SetNumberField(TEXT("peripheral_vision_angle"), PeripheralVisionAngle);
	Response->SetNumberField(TEXT("auto_success_range"), AutoSuccessRange);
	Response->SetNumberField(TEXT("max_age"), MaxAge);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAIPerceptionCommands::HandleConfigureHearingSense(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
	{
		return Error;
	}

	FString ComponentName;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("component_name"), ComponentName, TEXT("AIPerceptionComponent"));
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

	// Get optional sense parameters with defaults
	double HearingRange = 3000.0;
	double MaxAge = 5.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("hearing_range"), HearingRange, 3000.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("max_age"), MaxAge, 5.0);

	// Parse detection by affiliation
	bool bDetectEnemies = true;
	bool bDetectNeutrals = true;
	bool bDetectFriendlies = false;

	if (Params->HasField(TEXT("detection_by_affiliation")))
	{
		TSharedPtr<FJsonObject> AffiliationJson = Params->GetObjectField(TEXT("detection_by_affiliation"));
		ParseDetectionAffiliation(AffiliationJson, bDetectEnemies, bDetectNeutrals, bDetectFriendlies);
	}

	// Find the Blueprint
	FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));

	if (!Blueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::BlueprintNotFound,
			FString::Printf(TEXT("Blueprint not found: %s"), *FullPath)
		);
	}

	// Find the AIPerceptionComponent via SCS
	UAIPerceptionComponent* PerceptionComp = nullptr;
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->ComponentClass && Node->ComponentClass->IsChildOf(UAIPerceptionComponent::StaticClass()))
			{
				if (Node->GetVariableName() == FName(*ComponentName) || ComponentName == TEXT("AIPerceptionComponent"))
				{
					PerceptionComp = Cast<UAIPerceptionComponent>(Node->ComponentTemplate);
					break;
				}
			}
		}
	}

	if (!PerceptionComp)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::ComponentNotFound,
			FString::Printf(TEXT("AIPerceptionComponent '%s' not found in Blueprint"), *ComponentName)
		);
	}

	// Create and configure hearing sense config
	UAISenseConfig_Hearing* HearingConfig = NewObject<UAISenseConfig_Hearing>(PerceptionComp, UAISenseConfig_Hearing::StaticClass());
	HearingConfig->HearingRange = static_cast<float>(HearingRange);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = bDetectEnemies;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = bDetectNeutrals;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	HearingConfig->SetMaxAge(static_cast<float>(MaxAge));

	// Configure the sense
	PerceptionComp->ConfigureSense(*HearingConfig);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Save the asset
	UEditorAssetLibrary::SaveAsset(FullPath, false);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("sense_type"), TEXT("Hearing"));
	Response->SetNumberField(TEXT("hearing_range"), HearingRange);
	Response->SetNumberField(TEXT("max_age"), MaxAge);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAIPerceptionCommands::HandleConfigureDamageSense(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
	{
		return Error;
	}

	FString ComponentName;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("component_name"), ComponentName, TEXT("AIPerceptionComponent"));
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

	// Get optional sense parameters with defaults
	double MaxAge = 5.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("max_age"), MaxAge, 5.0);

	// Find the Blueprint
	FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));

	if (!Blueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::BlueprintNotFound,
			FString::Printf(TEXT("Blueprint not found: %s"), *FullPath)
		);
	}

	// Find the AIPerceptionComponent via SCS
	UAIPerceptionComponent* PerceptionComp = nullptr;
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->ComponentClass && Node->ComponentClass->IsChildOf(UAIPerceptionComponent::StaticClass()))
			{
				if (Node->GetVariableName() == FName(*ComponentName) || ComponentName == TEXT("AIPerceptionComponent"))
				{
					PerceptionComp = Cast<UAIPerceptionComponent>(Node->ComponentTemplate);
					break;
				}
			}
		}
	}

	if (!PerceptionComp)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::ComponentNotFound,
			FString::Printf(TEXT("AIPerceptionComponent '%s' not found in Blueprint"), *ComponentName)
		);
	}

	// Create and configure damage sense config
	UAISenseConfig_Damage* DamageConfig = NewObject<UAISenseConfig_Damage>(PerceptionComp, UAISenseConfig_Damage::StaticClass());
	DamageConfig->SetMaxAge(static_cast<float>(MaxAge));

	// Configure the sense
	PerceptionComp->ConfigureSense(*DamageConfig);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Save the asset
	UEditorAssetLibrary::SaveAsset(FullPath, false);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("sense_type"), TEXT("Damage"));
	Response->SetNumberField(TEXT("max_age"), MaxAge);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAIPerceptionCommands::HandleSetPerceptionDominantSense(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
	{
		return Error;
	}

	FString SenseType;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("sense_type"), SenseType))
	{
		return Error;
	}

	FString ComponentName;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("component_name"), ComponentName, TEXT("AIPerceptionComponent"));
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

	// Find the Blueprint
	FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));

	if (!Blueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::BlueprintNotFound,
			FString::Printf(TEXT("Blueprint not found: %s"), *FullPath)
		);
	}

	// Find the AIPerceptionComponent via SCS
	UAIPerceptionComponent* PerceptionComp = nullptr;
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->ComponentClass && Node->ComponentClass->IsChildOf(UAIPerceptionComponent::StaticClass()))
			{
				if (Node->GetVariableName() == FName(*ComponentName) || ComponentName == TEXT("AIPerceptionComponent"))
				{
					PerceptionComp = Cast<UAIPerceptionComponent>(Node->ComponentTemplate);
					break;
				}
			}
		}
	}

	if (!PerceptionComp)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::ComponentNotFound,
			FString::Printf(TEXT("AIPerceptionComponent '%s' not found in Blueprint"), *ComponentName)
		);
	}

	// Get the sense class
	UClass* SenseClass = GetSenseClass(SenseType);
	if (!SenseClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParamValue,
			FString::Printf(TEXT("Invalid sense_type: %s. Valid types: Sight, Hearing, Damage"), *SenseType)
		);
	}

	// Set dominant sense
	PerceptionComp->SetDominantSense(SenseClass);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Save the asset
	UEditorAssetLibrary::SaveAsset(FullPath, false);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("dominant_sense"), SenseType);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAIPerceptionCommands::HandleAddPerceptionStimuliSource(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString BlueprintName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
	{
		return Error;
	}

	FString ComponentName;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("component_name"), ComponentName, TEXT("AIPerceptionStimuliSourceComponent"));
	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));
	bool bAutoRegister = true;
	FSpirrowBridgeCommonUtils::GetOptionalBool(Params, TEXT("auto_register"), bAutoRegister, true);

	// Get register_as_source_for array
	TArray<FString> RegisterForSenses;
	if (Params->HasField(TEXT("register_as_source_for")))
	{
		const TArray<TSharedPtr<FJsonValue>>* SensesArray;
		if (Params->TryGetArrayField(TEXT("register_as_source_for"), SensesArray))
		{
			for (const TSharedPtr<FJsonValue>& Value : *SensesArray)
			{
				RegisterForSenses.Add(Value->AsString());
			}
		}
	}

	// Default to Sight if no senses specified
	if (RegisterForSenses.Num() == 0)
	{
		RegisterForSenses.Add(TEXT("Sight"));
	}

	// Find the Blueprint
	FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *BlueprintName, *BlueprintName);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullPath));

	if (!Blueprint)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::BlueprintNotFound,
			FString::Printf(TEXT("Blueprint not found: %s"), *FullPath)
		);
	}

	// Check if component already exists
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->GetVariableName() == FName(*ComponentName))
			{
				return FSpirrowBridgeCommonUtils::CreateErrorResponse(
					ESpirrowErrorCode::InvalidOperation,
					FString::Printf(TEXT("Component '%s' already exists in Blueprint"), *ComponentName)
				);
			}
		}
	}

	// Create SCS node for AIPerceptionStimuliSourceComponent
	USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(UAIPerceptionStimuliSourceComponent::StaticClass(), FName(*ComponentName));
	if (!NewNode)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::ComponentCreationFailed,
			TEXT("Failed to create AIPerceptionStimuliSourceComponent node")
		);
	}

	// Set the variable name
	NewNode->SetVariableName(FName(*ComponentName));

	// Get the component template and configure it
	UAIPerceptionStimuliSourceComponent* StimuliComp = Cast<UAIPerceptionStimuliSourceComponent>(NewNode->ComponentTemplate);
	if (StimuliComp)
	{
		StimuliComp->bAutoRegister = bAutoRegister;

		// Register for specified senses
		for (const FString& SenseType : RegisterForSenses)
		{
			UClass* SenseClass = GetSenseClass(SenseType);
			if (SenseClass)
			{
				StimuliComp->RegisterForSense(TSubclassOf<UAISense>(SenseClass));
			}
		}
	}

	// Add to root
	Blueprint->SimpleConstructionScript->AddNode(NewNode);

	// Mark Blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// Save the asset
	UEditorAssetLibrary::SaveAsset(FullPath, false);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("blueprint_name"), BlueprintName);
	Response->SetStringField(TEXT("component_name"), ComponentName);
	Response->SetBoolField(TEXT("auto_register"), bAutoRegister);

	TArray<TSharedPtr<FJsonValue>> SensesJsonArray;
	for (const FString& Sense : RegisterForSenses)
	{
		SensesJsonArray.Add(MakeShareable(new FJsonValueString(Sense)));
	}
	Response->SetArrayField(TEXT("registered_senses"), SensesJsonArray);

	return Response;
}

// ===== Helper Functions =====

UAIPerceptionComponent* FSpirrowBridgeAIPerceptionCommands::FindPerceptionComponent(UBlueprint* Blueprint, const FString& ComponentName)
{
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return nullptr;
	}

	for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->ComponentClass && Node->ComponentClass->IsChildOf(UAIPerceptionComponent::StaticClass()))
		{
			if (Node->GetVariableName() == FName(*ComponentName))
			{
				return Cast<UAIPerceptionComponent>(Node->ComponentTemplate);
			}
		}
	}

	return nullptr;
}

UClass* FSpirrowBridgeAIPerceptionCommands::GetSenseClass(const FString& SenseType)
{
	if (SenseType.Equals(TEXT("Sight"), ESearchCase::IgnoreCase))
	{
		return UAISense_Sight::StaticClass();
	}
	else if (SenseType.Equals(TEXT("Hearing"), ESearchCase::IgnoreCase))
	{
		return UAISense_Hearing::StaticClass();
	}
	else if (SenseType.Equals(TEXT("Damage"), ESearchCase::IgnoreCase))
	{
		return UAISense_Damage::StaticClass();
	}

	return nullptr;
}

void FSpirrowBridgeAIPerceptionCommands::ParseDetectionAffiliation(
	const TSharedPtr<FJsonObject>& AffiliationJson,
	bool& OutDetectEnemies,
	bool& OutDetectNeutrals,
	bool& OutDetectFriendlies)
{
	if (!AffiliationJson.IsValid())
	{
		return;
	}

	if (AffiliationJson->HasField(TEXT("enemies")))
	{
		OutDetectEnemies = AffiliationJson->GetBoolField(TEXT("enemies"));
	}
	if (AffiliationJson->HasField(TEXT("neutrals")))
	{
		OutDetectNeutrals = AffiliationJson->GetBoolField(TEXT("neutrals"));
	}
	if (AffiliationJson->HasField(TEXT("friendlies")))
	{
		OutDetectFriendlies = AffiliationJson->GetBoolField(TEXT("friendlies"));
	}
}
