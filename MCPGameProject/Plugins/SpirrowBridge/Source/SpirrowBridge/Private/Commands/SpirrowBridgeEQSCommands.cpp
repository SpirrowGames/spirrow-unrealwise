#include "Commands/SpirrowBridgeEQSCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// Asset includes
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

// EQS includes
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

// Generator includes
#include "EnvironmentQuery/Generators/EnvQueryGenerator_SimpleGrid.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_Donut.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_OnCircle.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ActorsOfClass.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_CurrentLocation.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_PathingGrid.h"

// Test includes
#include "EnvironmentQuery/Tests/EnvQueryTest_Distance.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Trace.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Dot.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Pathfinding.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_PathfindingBatch.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_GameplayTags.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Overlap.h"

// Context includes
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Item.h"

FSpirrowBridgeEQSCommands::FSpirrowBridgeEQSCommands()
{
}

FSpirrowBridgeEQSCommands::~FSpirrowBridgeEQSCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeEQSCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_eqs_query"))
	{
		return HandleCreateEQSQuery(Params);
	}
	else if (CommandType == TEXT("add_eqs_generator"))
	{
		return HandleAddEQSGenerator(Params);
	}
	else if (CommandType == TEXT("add_eqs_test"))
	{
		return HandleAddEQSTest(Params);
	}
	else if (CommandType == TEXT("set_eqs_test_property"))
	{
		return HandleSetEQSTestProperty(Params);
	}
	else if (CommandType == TEXT("list_eqs_assets"))
	{
		return HandleListEQSAssets(Params);
	}

	return FSpirrowBridgeCommonUtils::CreateErrorResponse(
		ESpirrowErrorCode::UnknownCommand,
		FString::Printf(TEXT("Unknown EQS command: %s"), *CommandType)
	);
}

TSharedPtr<FJsonObject> FSpirrowBridgeEQSCommands::HandleCreateEQSQuery(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString QueryName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), QueryName))
	{
		return Error;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/AI/EQS"));

	// Construct full path
	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *QueryName);
	FString FullAssetPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *QueryName);

	// Check if asset already exists
	if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetAlreadyExists,
			FString::Printf(TEXT("EQS Query already exists: %s"), *FullAssetPath)
		);
	}

	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create package for EQS Query")
		);
	}

	// Create the EQS Query asset
	UEnvQuery* NewQuery = NewObject<UEnvQuery>(Package, FName(*QueryName), RF_Public | RF_Standalone);
	if (!NewQuery)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create EQS Query asset")
		);
	}

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(NewQuery);
	NewQuery->MarkPackageDirty();

	// Save the package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewQuery, *PackageFileName, SaveArgs);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("name"), QueryName);
	Response->SetStringField(TEXT("asset_path"), FullAssetPath);
	Response->SetNumberField(TEXT("option_count"), 0);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEQSCommands::HandleAddEQSGenerator(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString QueryName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("query_name"), QueryName))
	{
		return Error;
	}

	FString GeneratorType;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("generator_type"), GeneratorType))
	{
		return Error;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/AI/EQS"));

	// Find the EQS Query
	UEnvQuery* Query = FindEQSQueryAsset(QueryName, Path);
	if (!Query)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("EQS Query not found: %s at %s"), *QueryName, *Path)
		);
	}

	// Get the generator class
	UClass* GeneratorClass = GetGeneratorClass(GeneratorType);
	if (!GeneratorClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParamValue,
			FString::Printf(TEXT("Invalid generator_type: %s. Valid types: SimpleGrid, Donut, OnCircle, ActorsOfClass, CurrentLocation, PathingGrid"), *GeneratorType)
		);
	}

	// Create the generator
	UEnvQueryGenerator* Generator = NewObject<UEnvQueryGenerator>(Query, GeneratorClass);
	if (!Generator)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::OperationFailed,
			TEXT("Failed to create EQS Generator")
		);
	}

	// Configure generator based on type
	if (GeneratorType.Equals(TEXT("SimpleGrid"), ESearchCase::IgnoreCase))
	{
		UEnvQueryGenerator_SimpleGrid* GridGen = Cast<UEnvQueryGenerator_SimpleGrid>(Generator);
		if (GridGen)
		{
			double GridSize = 1000.0;
			double SpaceBetween = 100.0;
			FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("grid_size"), GridSize, 1000.0);
			FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("space_between"), SpaceBetween, 100.0);
			GridGen->GridSize.DefaultValue = static_cast<float>(GridSize);
			GridGen->SpaceBetween.DefaultValue = static_cast<float>(SpaceBetween);
		}
	}
	else if (GeneratorType.Equals(TEXT("Donut"), ESearchCase::IgnoreCase))
	{
		UEnvQueryGenerator_Donut* DonutGen = Cast<UEnvQueryGenerator_Donut>(Generator);
		if (DonutGen)
		{
			double InnerRadius = 300.0;
			double OuterRadius = 1000.0;
			FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("inner_radius"), InnerRadius, 300.0);
			FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("outer_radius"), OuterRadius, 1000.0);
			DonutGen->InnerRadius.DefaultValue = static_cast<float>(InnerRadius);
			DonutGen->OuterRadius.DefaultValue = static_cast<float>(OuterRadius);
		}
	}
	else if (GeneratorType.Equals(TEXT("OnCircle"), ESearchCase::IgnoreCase))
	{
		UEnvQueryGenerator_OnCircle* CircleGen = Cast<UEnvQueryGenerator_OnCircle>(Generator);
		if (CircleGen)
		{
			double CircleRadius = 500.0;
			double NumberOfPoints = 8.0;
			FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("circle_radius"), CircleRadius, 500.0);
			FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("number_of_points"), NumberOfPoints, 8.0);
			CircleGen->CircleRadius.DefaultValue = static_cast<float>(CircleRadius);
			CircleGen->NumberOfPoints.DefaultValue = static_cast<int32>(NumberOfPoints);
		}
	}
	else if (GeneratorType.Equals(TEXT("ActorsOfClass"), ESearchCase::IgnoreCase))
	{
		UEnvQueryGenerator_ActorsOfClass* ActorsGen = Cast<UEnvQueryGenerator_ActorsOfClass>(Generator);
		if (ActorsGen)
		{
			FString SearchedActorClass;
			FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("searched_actor_class"), SearchedActorClass, TEXT(""));
			if (!SearchedActorClass.IsEmpty())
			{
				UClass* ActorClass = FindObject<UClass>(nullptr, *SearchedActorClass);
				if (ActorClass)
				{
					ActorsGen->SearchedActorClass = ActorClass;
				}
			}
		}
	}

	// Add the generator as a new option (UEnvQueryOption)
	UEnvQueryOption* NewOption = NewObject<UEnvQueryOption>(Query);
	NewOption->Generator = Generator;
	Query->GetOptionsMutable().Add(NewOption);

	// Mark as dirty and save
	Query->MarkPackageDirty();

	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *QueryName);
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Query->GetOutermost(), Query, *PackageFileName, SaveArgs);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("query_name"), QueryName);
	Response->SetStringField(TEXT("generator_type"), GeneratorType);
	Response->SetNumberField(TEXT("generator_index"), Query->GetOptions().Num() - 1);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEQSCommands::HandleAddEQSTest(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString QueryName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("query_name"), QueryName))
	{
		return Error;
	}

	FString TestType;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("test_type"), TestType))
	{
		return Error;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/AI/EQS"));
	double GeneratorIndexDouble = 0.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("generator_index"), GeneratorIndexDouble, 0.0);
	int32 GeneratorIndex = static_cast<int32>(GeneratorIndexDouble);

	// Find the EQS Query
	UEnvQuery* Query = FindEQSQueryAsset(QueryName, Path);
	if (!Query)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("EQS Query not found: %s at %s"), *QueryName, *Path)
		);
	}

	// Validate generator index
	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	if (GeneratorIndex < 0 || GeneratorIndex >= Options.Num())
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParamValue,
			FString::Printf(TEXT("Invalid generator_index: %d. Query has %d generators"), GeneratorIndex, Options.Num())
		);
	}

	// Get the test class
	UClass* TestClass = GetTestClass(TestType);
	if (!TestClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParamValue,
			FString::Printf(TEXT("Invalid test_type: %s. Valid types: Distance, Trace, Dot, Pathfinding, GameplayTags, Overlap"), *TestType)
		);
	}

	// Create the test
	UEnvQueryTest* Test = NewObject<UEnvQueryTest>(Query, TestClass);
	if (!Test)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::OperationFailed,
			TEXT("Failed to create EQS Test")
		);
	}

	// Configure common test settings
	FString TestPurposeStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("test_purpose"), TestPurposeStr, TEXT("Score"));
	Test->TestPurpose = GetTestPurpose(TestPurposeStr);

	FString ScoringEquationStr;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("scoring_equation"), ScoringEquationStr, TEXT("Linear"));
	Test->ScoringEquation = GetScoringEquation(ScoringEquationStr);

	double ScoringFactor = 1.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("scoring_factor"), ScoringFactor, 1.0);
	Test->ScoringFactor.DefaultValue = static_cast<float>(ScoringFactor);

	// Configure type-specific settings
	if (TestType.Equals(TEXT("Distance"), ESearchCase::IgnoreCase))
	{
		UEnvQueryTest_Distance* DistTest = Cast<UEnvQueryTest_Distance>(Test);
		if (DistTest)
		{
			// Distance test configuration can be extended here
		}
	}
	else if (TestType.Equals(TEXT("Trace"), ESearchCase::IgnoreCase))
	{
		UEnvQueryTest_Trace* TraceTest = Cast<UEnvQueryTest_Trace>(Test);
		if (TraceTest)
		{
			// Trace test configuration can be extended here
		}
	}
	else if (TestType.Equals(TEXT("Dot"), ESearchCase::IgnoreCase))
	{
		UEnvQueryTest_Dot* DotTest = Cast<UEnvQueryTest_Dot>(Test);
		if (DotTest)
		{
			// Dot test configuration can be extended here
		}
	}

	// Add test to the generator option
	UEnvQueryOption* Option = Query->GetOptionsMutable()[GeneratorIndex];
	Option->Tests.Add(Test);

	// Mark as dirty and save
	Query->MarkPackageDirty();

	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *QueryName);
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Query->GetOutermost(), Query, *PackageFileName, SaveArgs);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("query_name"), QueryName);
	Response->SetStringField(TEXT("test_type"), TestType);
	Response->SetNumberField(TEXT("generator_index"), GeneratorIndex);
	Response->SetNumberField(TEXT("test_index"), Option->Tests.Num() - 1);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEQSCommands::HandleSetEQSTestProperty(const TSharedPtr<FJsonObject>& Params)
{
	// Validate required parameters
	FString QueryName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("query_name"), QueryName))
	{
		return Error;
	}

	FString PropertyName;
	if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
	{
		return Error;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/AI/EQS"));
	double GeneratorIndexDouble = 0.0;
	double TestIndexDouble = 0.0;
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("generator_index"), GeneratorIndexDouble, 0.0);
	FSpirrowBridgeCommonUtils::GetOptionalNumber(Params, TEXT("test_index"), TestIndexDouble, 0.0);
	int32 GeneratorIndex = static_cast<int32>(GeneratorIndexDouble);
	int32 TestIndex = static_cast<int32>(TestIndexDouble);

	// Find the EQS Query
	UEnvQuery* Query = FindEQSQueryAsset(QueryName, Path);
	if (!Query)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("EQS Query not found: %s at %s"), *QueryName, *Path)
		);
	}

	// Validate indices
	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	if (GeneratorIndex < 0 || GeneratorIndex >= Options.Num())
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParamValue,
			FString::Printf(TEXT("Invalid generator_index: %d"), GeneratorIndex)
		);
	}

	UEnvQueryOption* Option = Options[GeneratorIndex];
	if (TestIndex < 0 || TestIndex >= Option->Tests.Num())
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParamValue,
			FString::Printf(TEXT("Invalid test_index: %d"), TestIndex)
		);
	}

	UEnvQueryTest* Test = Option->Tests[TestIndex];
	if (!Test)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::OperationFailed,
			TEXT("Test not found at specified index")
		);
	}

	// Get property value from params
	TSharedPtr<FJsonValue> PropertyValue = Params->TryGetField(TEXT("property_value"));
	if (!PropertyValue.IsValid())
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::MissingRequiredParam,
			TEXT("Missing required parameter: property_value")
		);
	}

	// Set the property using reflection
	FString ErrorMessage;
	bool bSuccess = FSpirrowBridgeCommonUtils::SetObjectProperty(Test, PropertyName, PropertyValue, ErrorMessage);
	if (!bSuccess)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::PropertySetFailed,
			FString::Printf(TEXT("Failed to set property '%s' on test: %s"), *PropertyName, *ErrorMessage)
		);
	}

	// Mark as dirty and save
	Query->MarkPackageDirty();

	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *QueryName);
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Query->GetOutermost(), Query, *PackageFileName, SaveArgs);

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetStringField(TEXT("query_name"), QueryName);
	Response->SetStringField(TEXT("property_name"), PropertyName);
	Response->SetNumberField(TEXT("generator_index"), GeneratorIndex);
	Response->SetNumberField(TEXT("test_index"), TestIndex);

	return Response;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEQSCommands::HandleListEQSAssets(const TSharedPtr<FJsonObject>& Params)
{
	FString PathFilter;
	FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path_filter"), PathFilter, TEXT(""));

	// Get asset registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Find all EQS Query assets
	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(UEnvQuery::StaticClass()->GetClassPathName(), AssetDataList);

	TArray<TSharedPtr<FJsonValue>> QueriesArray;

	for (const FAssetData& AssetData : AssetDataList)
	{
		FString AssetPath = AssetData.GetObjectPathString();

		// Apply path filter if specified
		if (!PathFilter.IsEmpty() && !AssetPath.Contains(PathFilter))
		{
			continue;
		}

		TSharedPtr<FJsonObject> QueryJson = MakeShareable(new FJsonObject);
		QueryJson->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		QueryJson->SetStringField(TEXT("path"), AssetData.PackagePath.ToString());
		QueryJson->SetStringField(TEXT("asset_path"), AssetPath);

		// Try to load and get more details
		UEnvQuery* Query = Cast<UEnvQuery>(AssetData.GetAsset());
		if (Query)
		{
			const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
			QueryJson->SetNumberField(TEXT("generator_count"), Options.Num());

			int32 TotalTests = 0;
			for (UEnvQueryOption* Option : Options)
			{
				if (Option)
				{
					TotalTests += Option->Tests.Num();
				}
			}
			QueryJson->SetNumberField(TEXT("test_count"), TotalTests);
		}

		QueriesArray.Add(MakeShareable(new FJsonValueObject(QueryJson)));
	}

	// Build response
	TSharedPtr<FJsonObject> Response = FSpirrowBridgeCommonUtils::CreateSuccessResponse();
	Response->SetArrayField(TEXT("queries"), QueriesArray);
	Response->SetNumberField(TEXT("total_count"), QueriesArray.Num());

	return Response;
}

// ===== Helper Functions =====

UEnvQuery* FSpirrowBridgeEQSCommands::FindEQSQueryAsset(const FString& Name, const FString& Path)
{
	FString FullPath = FString::Printf(TEXT("%s/%s.%s"), *Path, *Name, *Name);
	return Cast<UEnvQuery>(UEditorAssetLibrary::LoadAsset(FullPath));
}

UClass* FSpirrowBridgeEQSCommands::GetGeneratorClass(const FString& GeneratorType)
{
	if (GeneratorType.Equals(TEXT("SimpleGrid"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryGenerator_SimpleGrid::StaticClass();
	}
	else if (GeneratorType.Equals(TEXT("Donut"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryGenerator_Donut::StaticClass();
	}
	else if (GeneratorType.Equals(TEXT("OnCircle"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryGenerator_OnCircle::StaticClass();
	}
	else if (GeneratorType.Equals(TEXT("ActorsOfClass"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryGenerator_ActorsOfClass::StaticClass();
	}
	else if (GeneratorType.Equals(TEXT("CurrentLocation"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryGenerator_CurrentLocation::StaticClass();
	}
	else if (GeneratorType.Equals(TEXT("PathingGrid"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryGenerator_PathingGrid::StaticClass();
	}

	return nullptr;
}

UClass* FSpirrowBridgeEQSCommands::GetTestClass(const FString& TestType)
{
	if (TestType.Equals(TEXT("Distance"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryTest_Distance::StaticClass();
	}
	else if (TestType.Equals(TEXT("Trace"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryTest_Trace::StaticClass();
	}
	else if (TestType.Equals(TEXT("Dot"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryTest_Dot::StaticClass();
	}
	else if (TestType.Equals(TEXT("Pathfinding"), ESearchCase::IgnoreCase) ||
	         TestType.Equals(TEXT("PathExists"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryTest_Pathfinding::StaticClass();
	}
	else if (TestType.Equals(TEXT("GameplayTags"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryTest_GameplayTags::StaticClass();
	}
	else if (TestType.Equals(TEXT("Overlap"), ESearchCase::IgnoreCase))
	{
		return UEnvQueryTest_Overlap::StaticClass();
	}

	return nullptr;
}

EEnvTestScoreEquation::Type FSpirrowBridgeEQSCommands::GetScoringEquation(const FString& EquationString)
{
	if (EquationString.Equals(TEXT("Linear"), ESearchCase::IgnoreCase))
	{
		return EEnvTestScoreEquation::Linear;
	}
	else if (EquationString.Equals(TEXT("Square"), ESearchCase::IgnoreCase))
	{
		return EEnvTestScoreEquation::Square;
	}
	else if (EquationString.Equals(TEXT("InverseLinear"), ESearchCase::IgnoreCase))
	{
		return EEnvTestScoreEquation::InverseLinear;
	}
	else if (EquationString.Equals(TEXT("Constant"), ESearchCase::IgnoreCase))
	{
		return EEnvTestScoreEquation::Constant;
	}
	else if (EquationString.Equals(TEXT("SquareRoot"), ESearchCase::IgnoreCase))
	{
		return EEnvTestScoreEquation::SquareRoot;
	}

	return EEnvTestScoreEquation::Linear;
}

EEnvTestPurpose::Type FSpirrowBridgeEQSCommands::GetTestPurpose(const FString& PurposeString)
{
	if (PurposeString.Equals(TEXT("Score"), ESearchCase::IgnoreCase))
	{
		return EEnvTestPurpose::Score;
	}
	else if (PurposeString.Equals(TEXT("Filter"), ESearchCase::IgnoreCase))
	{
		return EEnvTestPurpose::Filter;
	}
	else if (PurposeString.Equals(TEXT("FilterAndScore"), ESearchCase::IgnoreCase) ||
	         PurposeString.Equals(TEXT("ScoreAndFilter"), ESearchCase::IgnoreCase))
	{
		return EEnvTestPurpose::FilterAndScore;
	}

	return EEnvTestPurpose::Score;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEQSCommands::EQSQueryToJson(UEnvQuery* Query)
{
	if (!Query)
	{
		return nullptr;
	}

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();

	TSharedPtr<FJsonObject> QueryJson = MakeShareable(new FJsonObject);
	QueryJson->SetStringField(TEXT("name"), Query->GetName());
	QueryJson->SetNumberField(TEXT("option_count"), Options.Num());

	TArray<TSharedPtr<FJsonValue>> OptionsArray;
	for (int32 i = 0; i < Options.Num(); i++)
	{
		UEnvQueryOption* Option = Options[i];
		if (Option && Option->Generator)
		{
			TSharedPtr<FJsonObject> OptionJson = MakeShareable(new FJsonObject);
			OptionJson->SetNumberField(TEXT("index"), i);
			OptionJson->SetStringField(TEXT("generator_class"), Option->Generator->GetClass()->GetName());
			OptionJson->SetNumberField(TEXT("test_count"), Option->Tests.Num());

			TArray<TSharedPtr<FJsonValue>> TestsArray;
			for (int32 j = 0; j < Option->Tests.Num(); j++)
			{
				UEnvQueryTest* Test = Option->Tests[j];
				if (Test)
				{
					TSharedPtr<FJsonObject> TestJson = MakeShareable(new FJsonObject);
					TestJson->SetNumberField(TEXT("index"), j);
					TestJson->SetStringField(TEXT("test_class"), Test->GetClass()->GetName());
					TestsArray.Add(MakeShareable(new FJsonValueObject(TestJson)));
				}
			}
			OptionJson->SetArrayField(TEXT("tests"), TestsArray);

			OptionsArray.Add(MakeShareable(new FJsonValueObject(OptionJson)));
		}
	}
	QueryJson->SetArrayField(TEXT("options"), OptionsArray);

	return QueryJson;
}
