#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// Blackboard includes
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"

// Asset management includes
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"

// ===== Helper Functions =====

UBlackboardData* FSpirrowBridgeAICommands::FindBlackboardAsset(const FString& Name, const FString& Path)
{
	FString FullPath = Path / Name + TEXT(".") + Name;
	return Cast<UBlackboardData>(UEditorAssetLibrary::LoadAsset(FullPath));
}

UClass* FSpirrowBridgeAICommands::GetBlackboardKeyTypeClass(const FString& TypeString)
{
	if (TypeString == TEXT("Bool")) return UBlackboardKeyType_Bool::StaticClass();
	if (TypeString == TEXT("Int")) return UBlackboardKeyType_Int::StaticClass();
	if (TypeString == TEXT("Float")) return UBlackboardKeyType_Float::StaticClass();
	if (TypeString == TEXT("String")) return UBlackboardKeyType_String::StaticClass();
	if (TypeString == TEXT("Name")) return UBlackboardKeyType_Name::StaticClass();
	if (TypeString == TEXT("Vector")) return UBlackboardKeyType_Vector::StaticClass();
	if (TypeString == TEXT("Rotator")) return UBlackboardKeyType_Rotator::StaticClass();
	if (TypeString == TEXT("Object")) return UBlackboardKeyType_Object::StaticClass();
	if (TypeString == TEXT("Class")) return UBlackboardKeyType_Class::StaticClass();
	if (TypeString == TEXT("Enum")) return UBlackboardKeyType_Enum::StaticClass();
	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::BlackboardKeyToJson(const FBlackboardEntry& Entry)
{
	TSharedPtr<FJsonObject> KeyJson = MakeShareable(new FJsonObject());
	KeyJson->SetStringField(TEXT("name"), Entry.EntryName.ToString());

	if (Entry.KeyType)
	{
		FString TypeName = Entry.KeyType->GetClass()->GetName();
		// Remove "BlackboardKeyType_" prefix
		TypeName.RemoveFromStart(TEXT("BlackboardKeyType_"));
		KeyJson->SetStringField(TEXT("type"), TypeName);

		// Add base_class for Object/Class types
		if (UBlackboardKeyType_Object* ObjectType = Cast<UBlackboardKeyType_Object>(Entry.KeyType))
		{
			if (ObjectType->BaseClass)
			{
				KeyJson->SetStringField(TEXT("base_class"), ObjectType->BaseClass->GetName());
			}
		}
		else if (UBlackboardKeyType_Class* ClassType = Cast<UBlackboardKeyType_Class>(Entry.KeyType))
		{
			if (ClassType->BaseClass)
			{
				KeyJson->SetStringField(TEXT("base_class"), ClassType->BaseClass->GetName());
			}
		}
	}

	KeyJson->SetBoolField(TEXT("instance_synced"), Entry.bInstanceSynced);

	return KeyJson;
}

// ===== Blackboard Commands Implementation =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleCreateBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString Name;
	TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name);
	if (NameError)
	{
		return NameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));

	// パッケージ作成
	FString PackagePath = Path / Name;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create package"));
	}

	// Blackboard Data Asset作成
	UBlackboardData* BlackboardData = NewObject<UBlackboardData>(
		Package,
		*Name,
		RF_Public | RF_Standalone);

	if (!BlackboardData)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetCreationFailed,
			TEXT("Failed to create Blackboard Data Asset"));
	}

	// 保存
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BlackboardData);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, BlackboardData, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_path"), PackagePath);
	Result->SetStringField(TEXT("name"), Name);
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleAddBlackboardKey(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString BlackboardName;
	TSharedPtr<FJsonObject> BlackboardNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName);
	if (BlackboardNameError)
	{
		return BlackboardNameError;
	}

	FString KeyName;
	TSharedPtr<FJsonObject> KeyNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_name"), KeyName);
	if (KeyNameError)
	{
		return KeyNameError;
	}

	FString KeyType;
	TSharedPtr<FJsonObject> KeyTypeError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_type"), KeyType);
	if (KeyTypeError)
	{
		return KeyTypeError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));
	bool bInstanceSynced;
	FSpirrowBridgeCommonUtils::GetOptionalBool(
		Params, TEXT("instance_synced"), bInstanceSynced, false);
	FString BaseClass;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("base_class"), BaseClass, TEXT(""));

	// Blackboard検索
	UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, Path);
	if (!Blackboard)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *Path));
	}

	// キータイプ取得
	UClass* KeyTypeClass = GetBlackboardKeyTypeClass(KeyType);
	if (!KeyTypeClass)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Invalid key type: %s"), *KeyType));
	}

	// 重複チェック
	for (const FBlackboardEntry& Entry : Blackboard->Keys)
	{
		if (Entry.EntryName == FName(*KeyName))
		{
			return FSpirrowBridgeCommonUtils::CreateErrorResponse(
				ESpirrowErrorCode::InvalidOperation,
				FString::Printf(TEXT("Key already exists: %s"), *KeyName));
		}
	}

	// キー追加
	FBlackboardEntry NewEntry;
	NewEntry.EntryName = FName(*KeyName);
	NewEntry.KeyType = NewObject<UBlackboardKeyType>(Blackboard, KeyTypeClass);
	NewEntry.bInstanceSynced = bInstanceSynced;

	// Object/Classタイプの場合、BaseClassを設定
	if (!BaseClass.IsEmpty())
	{
		if (UBlackboardKeyType_Object* ObjectType = Cast<UBlackboardKeyType_Object>(NewEntry.KeyType))
		{
			UClass* FoundClass = FindObject<UClass>(nullptr, *BaseClass);
			if (FoundClass)
			{
				ObjectType->BaseClass = FoundClass;
			}
		}
		else if (UBlackboardKeyType_Class* ClassType = Cast<UBlackboardKeyType_Class>(NewEntry.KeyType))
		{
			UClass* FoundClass = FindObject<UClass>(nullptr, *BaseClass);
			if (FoundClass)
			{
				ClassType->BaseClass = FoundClass;
			}
		}
	}

	Blackboard->Keys.Add(NewEntry);

	// 保存
	Blackboard->MarkPackageDirty();
	UPackage* Package = Blackboard->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blackboard, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
	Result->SetStringField(TEXT("key_name"), KeyName);
	Result->SetStringField(TEXT("key_type"), KeyType);
	Result->SetNumberField(TEXT("total_keys"), Blackboard->Keys.Num());
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleRemoveBlackboardKey(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString BlackboardName;
	TSharedPtr<FJsonObject> BlackboardNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName);
	if (BlackboardNameError)
	{
		return BlackboardNameError;
	}

	FString KeyName;
	TSharedPtr<FJsonObject> KeyNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_name"), KeyName);
	if (KeyNameError)
	{
		return KeyNameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));

	// Blackboard検索
	UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, Path);
	if (!Blackboard)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *Path));
	}

	// キー検索と削除
	int32 RemovedIndex = INDEX_NONE;
	for (int32 i = 0; i < Blackboard->Keys.Num(); ++i)
	{
		if (Blackboard->Keys[i].EntryName == FName(*KeyName))
		{
			RemovedIndex = i;
			Blackboard->Keys.RemoveAt(i);
			break;
		}
	}

	if (RemovedIndex == INDEX_NONE)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::InvalidParameter,
			FString::Printf(TEXT("Key not found: %s"), *KeyName));
	}

	// 保存
	Blackboard->MarkPackageDirty();
	UPackage* Package = Blackboard->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Blackboard, *PackageFileName, SaveArgs);

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
	Result->SetStringField(TEXT("removed_key"), KeyName);
	Result->SetNumberField(TEXT("remaining_keys"), Blackboard->Keys.Num());
	return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleListBlackboardKeys(
	const TSharedPtr<FJsonObject>& Params)
{
	// パラメータ検証
	FString BlackboardName;
	TSharedPtr<FJsonObject> BlackboardNameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName);
	if (BlackboardNameError)
	{
		return BlackboardNameError;
	}

	FString Path;
	FSpirrowBridgeCommonUtils::GetOptionalString(
		Params, TEXT("path"), Path, TEXT("/Game/AI/Blackboards"));

	// Blackboard検索
	UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, Path);
	if (!Blackboard)
	{
		return FSpirrowBridgeCommonUtils::CreateErrorResponse(
			ESpirrowErrorCode::AssetNotFound,
			FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *Path));
	}

	// キー一覧取得
	TArray<TSharedPtr<FJsonValue>> KeysArray;
	for (const FBlackboardEntry& Entry : Blackboard->Keys)
	{
		KeysArray.Add(MakeShareable(new FJsonValueObject(BlackboardKeyToJson(Entry))));
	}

	// レスポンス作成
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
	Result->SetArrayField(TEXT("keys"), KeysArray);
	Result->SetNumberField(TEXT("count"), Blackboard->Keys.Num());
	return Result;
}
