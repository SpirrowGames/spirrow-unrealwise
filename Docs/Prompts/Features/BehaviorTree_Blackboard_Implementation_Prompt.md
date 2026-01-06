# BehaviorTree / Blackboard 操作ツール実装

## 概要

AI開発に必須のBehaviorTree（行動木）とBlackboard（知識ベース）をMCP経由で操作するツール群を実装する。

## 目的

- Blackboard Data Assetの作成・キー管理
- BehaviorTree Assetの作成・Blackboard紐付け
- AI関連アセットの一覧取得

---

## アーキテクチャ

```
[MCP Tools - ai_tools.py]
├── create_blackboard()              # Blackboard Data Asset作成
├── add_blackboard_key()             # Blackboardにキー追加
├── remove_blackboard_key()          # Blackboardからキー削除
├── list_blackboard_keys()           # Blackboardのキー一覧取得
├── create_behavior_tree()           # BehaviorTree Asset作成
├── set_behavior_tree_blackboard()   # BTにBlackboard設定
├── get_behavior_tree_structure()    # BT構造取得（将来拡張用）
└── list_ai_assets()                 # AI関連アセット一覧

[C++ Commands - SpirrowBridgeAICommands]
├── HandleCreateBlackboard()
├── HandleAddBlackboardKey()
├── HandleRemoveBlackboardKey()
├── HandleListBlackboardKeys()
├── HandleCreateBehaviorTree()
├── HandleSetBehaviorTreeBlackboard()
├── HandleGetBehaviorTreeStructure()
└── HandleListAIAssets()
```

---

## ファイル構成

```
spirrow-unrealwise/
├── Python/
│   └── tools/
│       └── ai_tools.py                    # 新規作成
└── MCPGameProject/
    └── Plugins/SpirrowBridge/
        └── Source/SpirrowBridge/
            ├── Public/Commands/
            │   └── SpirrowBridgeAICommands.h      # 新規作成
            └── Private/Commands/
                └── SpirrowBridgeAICommands.cpp    # 新規作成
```

---

## Part 1: Blackboardキータイプ対応表

| UE型 | MCPパラメータ | 説明 |
|------|---------------|------|
| `UBlackboardKeyType_Bool` | `"Bool"` | 真偽値 |
| `UBlackboardKeyType_Int` | `"Int"` | 整数 |
| `UBlackboardKeyType_Float` | `"Float"` | 浮動小数点 |
| `UBlackboardKeyType_String` | `"String"` | 文字列 |
| `UBlackboardKeyType_Name` | `"Name"` | FName |
| `UBlackboardKeyType_Vector` | `"Vector"` | 3Dベクトル |
| `UBlackboardKeyType_Rotator` | `"Rotator"` | 回転 |
| `UBlackboardKeyType_Object` | `"Object"` | UObject参照 |
| `UBlackboardKeyType_Class` | `"Class"` | クラス参照 |
| `UBlackboardKeyType_Enum` | `"Enum"` | 列挙型 |

### Object/Classタイプの追加パラメータ

```json
{
  "key_name": "TargetActor",
  "key_type": "Object",
  "base_class": "Actor"  // オプション: 許可するベースクラス
}
```

---

## Part 2: C++ ヘッダーファイル

### SpirrowBridgeAICommands.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles AI-related commands for SpirrowBridge.
 * Includes BehaviorTree and Blackboard operations.
 */
class SPIRROWBRIDGE_API FSpirrowBridgeAICommands
{
public:
    FSpirrowBridgeAICommands();
    ~FSpirrowBridgeAICommands();

    /**
     * Main command handler that routes to specific handlers.
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // ===== Blackboard Commands =====
    
    /**
     * Create a new Blackboard Data Asset.
     */
    TSharedPtr<FJsonObject> HandleCreateBlackboard(const TSharedPtr<FJsonObject>& Params);

    /**
     * Add a key to an existing Blackboard.
     */
    TSharedPtr<FJsonObject> HandleAddBlackboardKey(const TSharedPtr<FJsonObject>& Params);

    /**
     * Remove a key from a Blackboard.
     */
    TSharedPtr<FJsonObject> HandleRemoveBlackboardKey(const TSharedPtr<FJsonObject>& Params);

    /**
     * List all keys in a Blackboard.
     */
    TSharedPtr<FJsonObject> HandleListBlackboardKeys(const TSharedPtr<FJsonObject>& Params);

    // ===== BehaviorTree Commands =====

    /**
     * Create a new BehaviorTree Asset.
     */
    TSharedPtr<FJsonObject> HandleCreateBehaviorTree(const TSharedPtr<FJsonObject>& Params);

    /**
     * Set the Blackboard asset for a BehaviorTree.
     */
    TSharedPtr<FJsonObject> HandleSetBehaviorTreeBlackboard(const TSharedPtr<FJsonObject>& Params);

    /**
     * Get the structure of a BehaviorTree (nodes, connections).
     */
    TSharedPtr<FJsonObject> HandleGetBehaviorTreeStructure(const TSharedPtr<FJsonObject>& Params);

    // ===== Utility Commands =====

    /**
     * List AI-related assets in the project.
     */
    TSharedPtr<FJsonObject> HandleListAIAssets(const TSharedPtr<FJsonObject>& Params);

    // ===== Helper Functions =====

    /**
     * Find a Blackboard asset by name and path.
     */
    class UBlackboardData* FindBlackboardAsset(const FString& Name, const FString& Path);

    /**
     * Find a BehaviorTree asset by name and path.
     */
    class UBehaviorTree* FindBehaviorTreeAsset(const FString& Name, const FString& Path);

    /**
     * Get the UClass for a Blackboard key type string.
     */
    UClass* GetBlackboardKeyTypeClass(const FString& TypeString);

    /**
     * Convert a Blackboard key to JSON representation.
     */
    TSharedPtr<FJsonObject> BlackboardKeyToJson(const struct FBlackboardEntry& Entry);
};
```

---

## Part 3: C++ 実装ファイル

### SpirrowBridgeAICommands.cpp

```cpp
#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// AI Module includes
#include "BehaviorTree/BehaviorTree.h"
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

// Asset creation includes
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Factories/BlackboardDataFactory.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorAssetLibrary.h"

FSpirrowBridgeAICommands::FSpirrowBridgeAICommands()
{
}

FSpirrowBridgeAICommands::~FSpirrowBridgeAICommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleCommand(
    const FString& CommandType, 
    const TSharedPtr<FJsonObject>& Params)
{
    // Blackboard commands
    if (CommandType == TEXT("create_blackboard"))
    {
        return HandleCreateBlackboard(Params);
    }
    else if (CommandType == TEXT("add_blackboard_key"))
    {
        return HandleAddBlackboardKey(Params);
    }
    else if (CommandType == TEXT("remove_blackboard_key"))
    {
        return HandleRemoveBlackboardKey(Params);
    }
    else if (CommandType == TEXT("list_blackboard_keys"))
    {
        return HandleListBlackboardKeys(Params);
    }
    // BehaviorTree commands
    else if (CommandType == TEXT("create_behavior_tree"))
    {
        return HandleCreateBehaviorTree(Params);
    }
    else if (CommandType == TEXT("set_behavior_tree_blackboard"))
    {
        return HandleSetBehaviorTreeBlackboard(Params);
    }
    else if (CommandType == TEXT("get_behavior_tree_structure"))
    {
        return HandleGetBehaviorTreeStructure(Params);
    }
    // Utility commands
    else if (CommandType == TEXT("list_ai_assets"))
    {
        return HandleListAIAssets(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::UnknownCommand,
        FString::Printf(TEXT("Unknown AI command: %s"), *CommandType));
}

// ===== Helper Functions =====

UBlackboardData* FSpirrowBridgeAICommands::FindBlackboardAsset(const FString& Name, const FString& Path)
{
    FString FullPath = Path / Name + TEXT(".") + Name;
    return Cast<UBlackboardData>(UEditorAssetLibrary::LoadAsset(FullPath));
}

UBehaviorTree* FSpirrowBridgeAICommands::FindBehaviorTreeAsset(const FString& Name, const FString& Path)
{
    FString FullPath = Path / Name + TEXT(".") + Name;
    return Cast<UBehaviorTree>(UEditorAssetLibrary::LoadAsset(FullPath));
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
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: name"));
    }

    FString Path = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), TEXT("/Game/AI/Blackboards"));

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
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: blackboard_name"));
    }

    FString KeyName;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_name"), KeyName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: key_name"));
    }

    FString KeyType;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_type"), KeyType))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: key_type"));
    }

    FString Path = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), TEXT("/Game/AI/Blackboards"));
    bool bInstanceSynced = FSpirrowBridgeCommonUtils::GetOptionalBool(
        Params, TEXT("instance_synced"), false);
    FString BaseClass = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("base_class"), TEXT(""));

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
            UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *BaseClass);
            if (FoundClass)
            {
                ObjectType->BaseClass = FoundClass;
            }
        }
        else if (UBlackboardKeyType_Class* ClassType = Cast<UBlackboardKeyType_Class>(NewEntry.KeyType))
        {
            UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *BaseClass);
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
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: blackboard_name"));
    }

    FString KeyName;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("key_name"), KeyName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: key_name"));
    }

    FString Path = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), TEXT("/Game/AI/Blackboards"));

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
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: blackboard_name"));
    }

    FString Path = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), TEXT("/Game/AI/Blackboards"));

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

// ===== BehaviorTree Commands Implementation =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleCreateBehaviorTree(
    const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ検証
    FString Name;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: name"));
    }

    FString Path = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), TEXT("/Game/AI/BehaviorTrees"));
    FString BlackboardName = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("blackboard_name"), TEXT(""));
    FString BlackboardPath = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("blackboard_path"), TEXT("/Game/AI/Blackboards"));

    // パッケージ作成
    FString PackagePath = Path / Name;
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create package"));
    }

    // BehaviorTree Asset作成
    UBehaviorTree* BehaviorTree = NewObject<UBehaviorTree>(
        Package, 
        *Name, 
        RF_Public | RF_Standalone);

    if (!BehaviorTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetCreationFailed,
            TEXT("Failed to create BehaviorTree Asset"));
    }

    // Blackboard設定（指定されている場合）
    if (!BlackboardName.IsEmpty())
    {
        UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, BlackboardPath);
        if (Blackboard)
        {
            BehaviorTree->BlackboardAsset = Blackboard;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Blackboard not found: %s - BehaviorTree created without Blackboard"), *BlackboardName);
        }
    }

    // 保存
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(BehaviorTree);

    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

    // レスポンス作成
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("asset_path"), PackagePath);
    Result->SetStringField(TEXT("name"), Name);
    Result->SetBoolField(TEXT("has_blackboard"), BehaviorTree->BlackboardAsset != nullptr);
    if (BehaviorTree->BlackboardAsset)
    {
        Result->SetStringField(TEXT("blackboard_name"), BehaviorTree->BlackboardAsset->GetName());
    }
    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleSetBehaviorTreeBlackboard(
    const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ検証
    FString BehaviorTreeName;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("behavior_tree_name"), BehaviorTreeName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: behavior_tree_name"));
    }

    FString BlackboardName;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blackboard_name"), BlackboardName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: blackboard_name"));
    }

    FString BtPath = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("behavior_tree_path"), TEXT("/Game/AI/BehaviorTrees"));
    FString BbPath = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("blackboard_path"), TEXT("/Game/AI/Blackboards"));

    // BehaviorTree検索
    UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, BtPath);
    if (!BehaviorTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *BtPath));
    }

    // Blackboard検索
    UBlackboardData* Blackboard = FindBlackboardAsset(BlackboardName, BbPath);
    if (!Blackboard)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("Blackboard not found: %s at %s"), *BlackboardName, *BbPath));
    }

    // Blackboard設定
    BehaviorTree->BlackboardAsset = Blackboard;

    // 保存
    BehaviorTree->MarkPackageDirty();
    UPackage* Package = BehaviorTree->GetOutermost();
    FString PackageFileName = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, BehaviorTree, *PackageFileName, SaveArgs);

    // レスポンス作成
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
    Result->SetStringField(TEXT("blackboard_name"), BlackboardName);
    return Result;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleGetBehaviorTreeStructure(
    const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ検証
    FString Name;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: name"));
    }

    FString Path = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), TEXT("/Game/AI/BehaviorTrees"));

    // BehaviorTree検索
    UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(Name, Path);
    if (!BehaviorTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *Name, *Path));
    }

    // 基本情報
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), Name);
    
    // Blackboard情報
    if (BehaviorTree->BlackboardAsset)
    {
        Result->SetStringField(TEXT("blackboard_name"), BehaviorTree->BlackboardAsset->GetName());
        Result->SetNumberField(TEXT("blackboard_key_count"), BehaviorTree->BlackboardAsset->Keys.Num());
    }
    else
    {
        Result->SetStringField(TEXT("blackboard_name"), TEXT("None"));
    }

    // Root Node情報（将来的にノード構造も取得可能に）
    Result->SetBoolField(TEXT("has_root_node"), BehaviorTree->RootNode != nullptr);

    return Result;
}

// ===== Utility Commands Implementation =====

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleListAIAssets(
    const TSharedPtr<FJsonObject>& Params)
{
    FString AssetType = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("asset_type"), TEXT("all"));
    FString PathFilter = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path_filter"), TEXT(""));

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<TSharedPtr<FJsonValue>> BehaviorTrees;
    TArray<TSharedPtr<FJsonValue>> Blackboards;

    // BehaviorTree検索
    if (AssetType == TEXT("all") || AssetType == TEXT("behavior_tree"))
    {
        TArray<FAssetData> BTAssets;
        AssetRegistry.GetAssetsByClass(UBehaviorTree::StaticClass()->GetClassPathName(), BTAssets);

        for (const FAssetData& Asset : BTAssets)
        {
            FString AssetPath = Asset.GetObjectPathString();
            if (PathFilter.IsEmpty() || AssetPath.Contains(PathFilter))
            {
                TSharedPtr<FJsonObject> AssetJson = MakeShareable(new FJsonObject());
                AssetJson->SetStringField(TEXT("name"), Asset.AssetName.ToString());
                AssetJson->SetStringField(TEXT("path"), Asset.PackagePath.ToString());
                BehaviorTrees.Add(MakeShareable(new FJsonValueObject(AssetJson)));
            }
        }
    }

    // Blackboard検索
    if (AssetType == TEXT("all") || AssetType == TEXT("blackboard"))
    {
        TArray<FAssetData> BBAssets;
        AssetRegistry.GetAssetsByClass(UBlackboardData::StaticClass()->GetClassPathName(), BBAssets);

        for (const FAssetData& Asset : BBAssets)
        {
            FString AssetPath = Asset.GetObjectPathString();
            if (PathFilter.IsEmpty() || AssetPath.Contains(PathFilter))
            {
                TSharedPtr<FJsonObject> AssetJson = MakeShareable(new FJsonObject());
                AssetJson->SetStringField(TEXT("name"), Asset.AssetName.ToString());
                AssetJson->SetStringField(TEXT("path"), Asset.PackagePath.ToString());
                Blackboards.Add(MakeShareable(new FJsonValueObject(AssetJson)));
            }
        }
    }

    // レスポンス作成
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetArrayField(TEXT("behavior_trees"), BehaviorTrees);
    Result->SetArrayField(TEXT("blackboards"), Blackboards);
    Result->SetNumberField(TEXT("total_behavior_trees"), BehaviorTrees.Num());
    Result->SetNumberField(TEXT("total_blackboards"), Blackboards.Num());
    return Result;
}
```

---

## Part 4: Python MCP ツール

### Python/tools/ai_tools.py

```python
"""
AI Tools for Unreal MCP - BehaviorTree and Blackboard operations.

This module provides tools for working with Unreal Engine's AI systems.
"""

import logging
from typing import Dict, Any, List, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")


def register_ai_tools(mcp: FastMCP):
    """Register AI tools with the MCP server."""

    # ===== Blackboard Tools =====

    @mcp.tool()
    def create_blackboard(
        ctx: Context,
        name: str,
        path: str = "/Game/AI/Blackboards"
    ) -> Dict[str, Any]:
        """
        Create a new Blackboard Data Asset.

        Args:
            name: Name of the Blackboard (e.g., "BB_Enemy", "BB_Player")
            path: Content browser path for the asset

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - asset_path: Path to the created asset
            - name: Name of the created Blackboard

        Example:
            create_blackboard(
                name="BB_Enemy",
                path="/Game/AI/Blackboards"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {"name": name, "path": path}
            logger.info(f"Creating Blackboard: {name}")
            response = unreal.send_command("create_blackboard", params)

            if response and response.get("success"):
                logger.info(f"Created Blackboard: {response.get('asset_path')}")
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error creating Blackboard: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def add_blackboard_key(
        ctx: Context,
        blackboard_name: str,
        key_name: str,
        key_type: str,
        path: str = "/Game/AI/Blackboards",
        instance_synced: bool = False,
        base_class: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a key to an existing Blackboard.

        Args:
            blackboard_name: Name of the target Blackboard
            key_name: Name of the key to add (e.g., "TargetActor", "PatrolIndex")
            key_type: Type of the key:
                - "Bool": Boolean value
                - "Int": Integer value
                - "Float": Float value
                - "String": String value
                - "Name": FName value
                - "Vector": 3D Vector
                - "Rotator": Rotation
                - "Object": UObject reference
                - "Class": UClass reference
                - "Enum": Enumeration value
            path: Content browser path where the Blackboard is located
            instance_synced: Whether to sync this key across instances
            base_class: For Object/Class types, the allowed base class (e.g., "Actor", "Pawn")

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - blackboard_name: Name of the Blackboard
            - key_name: Name of the added key
            - total_keys: Total number of keys after addition

        Examples:
            # Add a target actor reference
            add_blackboard_key(
                blackboard_name="BB_Enemy",
                key_name="TargetActor",
                key_type="Object",
                base_class="Actor"
            )

            # Add a patrol index
            add_blackboard_key(
                blackboard_name="BB_Enemy",
                key_name="PatrolIndex",
                key_type="Int"
            )

            # Add a destination vector
            add_blackboard_key(
                blackboard_name="BB_Enemy",
                key_name="MoveToLocation",
                key_type="Vector"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blackboard_name": blackboard_name,
                "key_name": key_name,
                "key_type": key_type,
                "path": path,
                "instance_synced": instance_synced
            }
            if base_class:
                params["base_class"] = base_class

            logger.info(f"Adding key '{key_name}' ({key_type}) to Blackboard '{blackboard_name}'")
            response = unreal.send_command("add_blackboard_key", params)

            if response and response.get("success"):
                logger.info(f"Added key: {key_name}, total keys: {response.get('total_keys')}")
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error adding Blackboard key: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def remove_blackboard_key(
        ctx: Context,
        blackboard_name: str,
        key_name: str,
        path: str = "/Game/AI/Blackboards"
    ) -> Dict[str, Any]:
        """
        Remove a key from a Blackboard.

        Args:
            blackboard_name: Name of the target Blackboard
            key_name: Name of the key to remove
            path: Content browser path where the Blackboard is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - blackboard_name: Name of the Blackboard
            - removed_key: Name of the removed key
            - remaining_keys: Number of remaining keys

        Example:
            remove_blackboard_key(
                blackboard_name="BB_Enemy",
                key_name="OldKey"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blackboard_name": blackboard_name,
                "key_name": key_name,
                "path": path
            }

            logger.info(f"Removing key '{key_name}' from Blackboard '{blackboard_name}'")
            response = unreal.send_command("remove_blackboard_key", params)
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error removing Blackboard key: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def list_blackboard_keys(
        ctx: Context,
        blackboard_name: str,
        path: str = "/Game/AI/Blackboards"
    ) -> Dict[str, Any]:
        """
        List all keys in a Blackboard.

        Args:
            blackboard_name: Name of the target Blackboard
            path: Content browser path where the Blackboard is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - blackboard_name: Name of the Blackboard
            - keys: List of key objects with name, type, and properties
            - count: Total number of keys

        Example:
            list_blackboard_keys(blackboard_name="BB_Enemy")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blackboard_name": blackboard_name,
                "path": path
            }

            logger.info(f"Listing keys for Blackboard '{blackboard_name}'")
            response = unreal.send_command("list_blackboard_keys", params)
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error listing Blackboard keys: {e}")
            return {"success": False, "error": str(e)}

    # ===== BehaviorTree Tools =====

    @mcp.tool()
    def create_behavior_tree(
        ctx: Context,
        name: str,
        path: str = "/Game/AI/BehaviorTrees",
        blackboard_name: Optional[str] = None,
        blackboard_path: str = "/Game/AI/Blackboards"
    ) -> Dict[str, Any]:
        """
        Create a new BehaviorTree Asset.

        Args:
            name: Name of the BehaviorTree (e.g., "BT_Enemy", "BT_Patrol")
            path: Content browser path for the asset
            blackboard_name: Optional Blackboard to link (e.g., "BB_Enemy")
            blackboard_path: Path where the Blackboard is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - asset_path: Path to the created asset
            - name: Name of the created BehaviorTree
            - has_blackboard: Whether a Blackboard was linked
            - blackboard_name: Name of the linked Blackboard (if any)

        Examples:
            # Create BT without Blackboard
            create_behavior_tree(name="BT_SimplePatrol")

            # Create BT with linked Blackboard
            create_behavior_tree(
                name="BT_Enemy",
                blackboard_name="BB_Enemy",
                path="/Game/AI/BehaviorTrees"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "name": name,
                "path": path,
                "blackboard_name": blackboard_name or "",
                "blackboard_path": blackboard_path
            }

            logger.info(f"Creating BehaviorTree: {name}")
            response = unreal.send_command("create_behavior_tree", params)

            if response and response.get("success"):
                logger.info(f"Created BehaviorTree: {response.get('asset_path')}")
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error creating BehaviorTree: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def set_behavior_tree_blackboard(
        ctx: Context,
        behavior_tree_name: str,
        blackboard_name: str,
        behavior_tree_path: str = "/Game/AI/BehaviorTrees",
        blackboard_path: str = "/Game/AI/Blackboards"
    ) -> Dict[str, Any]:
        """
        Set the Blackboard asset for an existing BehaviorTree.

        Args:
            behavior_tree_name: Name of the target BehaviorTree
            blackboard_name: Name of the Blackboard to set
            behavior_tree_path: Path where the BehaviorTree is located
            blackboard_path: Path where the Blackboard is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - behavior_tree_name: Name of the BehaviorTree
            - blackboard_name: Name of the linked Blackboard

        Example:
            set_behavior_tree_blackboard(
                behavior_tree_name="BT_Enemy",
                blackboard_name="BB_Enemy"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "behavior_tree_name": behavior_tree_name,
                "blackboard_name": blackboard_name,
                "behavior_tree_path": behavior_tree_path,
                "blackboard_path": blackboard_path
            }

            logger.info(f"Setting Blackboard '{blackboard_name}' for BT '{behavior_tree_name}'")
            response = unreal.send_command("set_behavior_tree_blackboard", params)
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error setting BehaviorTree Blackboard: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def get_behavior_tree_structure(
        ctx: Context,
        name: str,
        path: str = "/Game/AI/BehaviorTrees"
    ) -> Dict[str, Any]:
        """
        Get the structure of a BehaviorTree.

        Args:
            name: Name of the BehaviorTree
            path: Content browser path where the BehaviorTree is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - name: Name of the BehaviorTree
            - blackboard_name: Name of the linked Blackboard
            - blackboard_key_count: Number of Blackboard keys
            - has_root_node: Whether the tree has a root node

        Example:
            get_behavior_tree_structure(name="BT_Enemy")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {"name": name, "path": path}

            logger.info(f"Getting structure for BehaviorTree '{name}'")
            response = unreal.send_command("get_behavior_tree_structure", params)
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error getting BehaviorTree structure: {e}")
            return {"success": False, "error": str(e)}

    # ===== Utility Tools =====

    @mcp.tool()
    def list_ai_assets(
        ctx: Context,
        asset_type: str = "all",
        path_filter: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        List AI-related assets (BehaviorTrees, Blackboards) in the project.

        Args:
            asset_type: Filter by asset type:
                - "all": All AI assets (default)
                - "behavior_tree": BehaviorTree assets only
                - "blackboard": Blackboard Data assets only
            path_filter: Filter by content path (e.g., "/Game/AI/")

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - behavior_trees: List of BehaviorTree assets
            - blackboards: List of Blackboard assets
            - total_behavior_trees: Count of BehaviorTrees
            - total_blackboards: Count of Blackboards

        Example:
            list_ai_assets(asset_type="all", path_filter="/Game/AI/")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "asset_type": asset_type,
                "path_filter": path_filter or ""
            }

            logger.info(f"Listing AI assets (type={asset_type})")
            response = unreal.send_command("list_ai_assets", params)
            
            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error listing AI assets: {e}")
            return {"success": False, "error": str(e)}
```

---

## Part 5: SpirrowBridge.cpp への統合

### SpirrowBridge.h に追加

```cpp
// Forward declaration
class FSpirrowBridgeAICommands;

// メンバー変数追加
TSharedPtr<FSpirrowBridgeAICommands> AICommands;
```

### SpirrowBridge.cpp に追加

```cpp
// ヘッダー追加
#include "Commands/SpirrowBridgeAICommands.h"

// コンストラクタに追加
AICommands = MakeShared<FSpirrowBridgeAICommands>();

// デストラクタに追加
AICommands.Reset();

// ExecuteCommand() のルーティングに追加（GAS Commandsの後など適切な場所に）
// AI Commands
else if (CommandType == TEXT("create_blackboard") ||
         CommandType == TEXT("add_blackboard_key") ||
         CommandType == TEXT("remove_blackboard_key") ||
         CommandType == TEXT("list_blackboard_keys") ||
         CommandType == TEXT("create_behavior_tree") ||
         CommandType == TEXT("set_behavior_tree_blackboard") ||
         CommandType == TEXT("get_behavior_tree_structure") ||
         CommandType == TEXT("list_ai_assets"))
{
    ResultJson = AICommands->HandleCommand(CommandType, Params);
}
```

---

## Part 6: unreal_mcp_server.py への統合

```python
# import 追加
from tools.ai_tools import register_ai_tools

# 登録（既存の register_xxx_tools の後に追加）
register_ai_tools(mcp)
```

---

## Part 7: Build.cs への AIModule 追加

### SpirrowBridge.Build.cs

```csharp
// PublicDependencyModuleNames に追加
"AIModule",
"GameplayTasks"
```

---

## テスト手順

1. **SpirrowBridge プラグインをビルド**
2. **MCP サーバー再起動**
3. **基本テスト実行**:

```python
# Blackboard作成
create_blackboard(name="BB_TestEnemy", path="/Game/AI/Blackboards")

# キー追加
add_blackboard_key(
    blackboard_name="BB_TestEnemy",
    key_name="TargetActor",
    key_type="Object",
    base_class="Actor"
)

add_blackboard_key(
    blackboard_name="BB_TestEnemy",
    key_name="PatrolIndex",
    key_type="Int"
)

add_blackboard_key(
    blackboard_name="BB_TestEnemy",
    key_name="MoveToLocation",
    key_type="Vector"
)

# キー一覧確認
list_blackboard_keys(blackboard_name="BB_TestEnemy")

# BehaviorTree作成（Blackboard連携）
create_behavior_tree(
    name="BT_TestEnemy",
    blackboard_name="BB_TestEnemy",
    path="/Game/AI/BehaviorTrees"
)

# BT構造確認
get_behavior_tree_structure(name="BT_TestEnemy")

# AI アセット一覧
list_ai_assets(path_filter="/Game/AI/")
```

---

## エラーコード追加（SpirrowBridgeCommonUtils.h）

```cpp
// AI関連エラーコード (1700-1799)
namespace ESpirrowErrorCode
{
    // ... 既存のコード ...
    
    // AI (1700-1799)
    constexpr int32 BlackboardNotFound = 1700;
    constexpr int32 BehaviorTreeNotFound = 1701;
    constexpr int32 BlackboardKeyExists = 1702;
    constexpr int32 BlackboardKeyNotFound = 1703;
    constexpr int32 InvalidBlackboardKeyType = 1704;
}
```

---

## 注意事項

1. **AIModule 依存**: Build.cs に `AIModule` と `GameplayTasks` の追加が必須
2. **UE5.5+ 対応**: UE5.5 以降の API を使用
3. **Blackboard キータイプ**: `UBlackboardKeyType_*` クラスは AIModule に含まれる
4. **BT ノード操作は将来拡張**: 今回はアセット作成・Blackboard連携のみ。ノード追加/接続は Phase 2 で検討

---

## 将来の拡張（Phase 2候補）

- `add_behavior_tree_node()` - BTにノード追加
- `connect_behavior_tree_nodes()` - ノード接続
- `add_bt_task_node()` / `add_bt_decorator_node()` / `add_bt_service_node()` - 各種ノード追加
- `create_ai_controller()` - AIController Blueprint作成
- `set_ai_controller_behavior_tree()` - AIControllerにBT設定
