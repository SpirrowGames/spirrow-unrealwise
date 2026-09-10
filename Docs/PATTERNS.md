---
id: spirrow-unrealwise:patterns
title: PATTERNS 実装パターン集
product: spirrow-unrealwise
type: reference
status: active
version: 1.0
created: 2026-01-03
last_verified: 2026-09-10
supersedes: []
related: []
keywords: [実装パターン, リファレンス]
legacy_drive_id: [1Na_8Mv4KRaaJkfMHVYHw9zLHsiTUTInHskEPHBGk7FM]
---

# SpirrowBridge 実装パターン集

このドキュメントは、SpirrowBridge開発時に使用する標準的な実装パターンをまとめたものです。
新機能追加やリファクタリング時に参照することで、一貫性のあるコードを効率的に書くことができます。

> **最終更新**: 2026-01-03  
> **バージョン**: Phase E

---

## 目次

1. [ルーターパターン](#1-ルーターパターン)
2. [エラーハンドリングパターン](#2-エラーハンドリングパターン)
3. [コマンドハンドラパターン](#3-コマンドハンドラパターン)
4. [テストパターン](#4-テストパターン)
5. [ファイル分割ガイドライン](#5-ファイル分割ガイドライン)

---

## 1. ルーターパターン

大きなファイル（60KB超）を分割する際に使用するパターン。

### いつ使うか

- ファイルサイズが60KBを超えた場合
- 機能的に明確なグループ分けが可能な場合

### 構成

```
SpirrowBridgeXxxCommands.cpp (ルーター, ~2KB)
├── SpirrowBridgeXxxCoreCommands.cpp (実装)
├── SpirrowBridgeXxxSubCommands.cpp (実装)
└── SpirrowBridgeXxxOtherCommands.cpp (実装)
```

### ルーターファイルの実装例

```cpp
// SpirrowBridgeXxxCommands.h
#pragma once
#include "XxxCoreCommands.h"
#include "XxxSubCommands.h"

class FSpirrowBridgeXxxCommands
{
public:
    static TSharedPtr<FJsonObject> HandleCommand(
        const FString& CommandType,
        const TSharedPtr<FJsonObject>& Params);

private:
    static FSpirrowBridgeXxxCoreCommands CoreCommands;
    static FSpirrowBridgeXxxSubCommands SubCommands;
};
```

```cpp
// SpirrowBridgeXxxCommands.cpp
#include "SpirrowBridgeXxxCommands.h"

FSpirrowBridgeXxxCoreCommands FSpirrowBridgeXxxCommands::CoreCommands;
FSpirrowBridgeXxxSubCommands FSpirrowBridgeXxxCommands::SubCommands;

TSharedPtr<FJsonObject> FSpirrowBridgeXxxCommands::HandleCommand(
    const FString& CommandType,
    const TSharedPtr<FJsonObject>& Params)
{
    // Core系コマンド
    if (CommandType == TEXT("create_xxx") ||
        CommandType == TEXT("delete_xxx"))
    {
        return CoreCommands.HandleCommand(CommandType, Params);
    }
    // Sub系コマンド
    else if (CommandType == TEXT("add_xxx") ||
             CommandType == TEXT("set_xxx"))
    {
        return SubCommands.HandleCommand(CommandType, Params);
    }
    
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::UnknownCommand,
        FString::Printf(TEXT("Unknown command: %s"), *CommandType));
}
```

### 分割クラスの実装例

```cpp
// SpirrowBridgeXxxCoreCommands.h
#pragma once

class FSpirrowBridgeXxxCoreCommands
{
public:
    TSharedPtr<FJsonObject> HandleCommand(
        const FString& CommandType,
        const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateXxx(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteXxx(const TSharedPtr<FJsonObject>& Params);
};
```

---

## 2. エラーハンドリングパターン

全Commandsファイルで統一されたエラーハンドリング方式。

### エラーコード体系

| 範囲 | カテゴリ | 例 |
|------|---------|-----|
| 0 | 成功 | Success |
| 1000-1099 | 一般エラー | UnknownCommand, InvalidParameter |
| 1100-1199 | アセットエラー | AssetNotFound, AssetCreationFailed |
| 1200-1299 | Blueprintエラー | BlueprintNotFound, NodeCreationFailed |
| 1300-1399 | Widgetエラー | WidgetNotFound, WidgetElementNotFound |
| 1400-1499 | Actorエラー | ActorNotFound, ComponentNotFound |
| 1500-1599 | GASエラー | GameplayTagInvalid |
| 1600-1699 | Configエラー | ConfigKeyNotFound, FileWriteFailed |

### CreateErrorResponse の3形式

```cpp
// 形式1: メッセージのみ（後方互換）
return FSpirrowBridgeCommonUtils::CreateErrorResponse(
    TEXT("Something went wrong"));

// 形式2: エラーコード + メッセージ（推奨）
return FSpirrowBridgeCommonUtils::CreateErrorResponse(
    ESpirrowErrorCode::BlueprintNotFound,
    FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));

// 形式3: エラーコード + メッセージ + 詳細情報
TSharedPtr<FJsonObject> Details = MakeShareable(new FJsonObject());
Details->SetStringField(TEXT("blueprint_name"), BlueprintName);
Details->SetStringField(TEXT("path"), Path);
return FSpirrowBridgeCommonUtils::CreateErrorResponse(
    ESpirrowErrorCode::BlueprintNotFound,
    TEXT("Blueprint not found"),
    Details);
```

### パラメータ検証パターン

```cpp
TSharedPtr<FJsonObject> HandleXxx(const TSharedPtr<FJsonObject>& Params)
{
    // 必須パラメータの検証
    FString Name;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), Name))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: name"));
    }
    
    // オプショナルパラメータの取得
    FString Path = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), TEXT("/Game/Default"));
    
    // 処理...
}
```

---

## 3. コマンドハンドラパターン

個々のコマンド処理関数の標準的な構造。

### 基本構造

```cpp
TSharedPtr<FJsonObject> HandleXxxCommand(const TSharedPtr<FJsonObject>& Params)
{
    // ===== 1. パラメータ検証 =====
    FString RequiredParam;
    if (!FSpirrowBridgeCommonUtils::ValidateRequiredString(
        Params, TEXT("required_param"), RequiredParam))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing required parameter: required_param"));
    }
    
    FString OptionalParam = FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("optional_param"), TEXT("default_value"));
    
    // ===== 2. リソース検証 =====
    UBlueprint* Blueprint = FSpirrowBridgeCommonUtils::FindBlueprintByName(
        RequiredParam, OptionalParam);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::BlueprintNotFound,
            FString::Printf(TEXT("Blueprint not found: %s at %s"), 
                *RequiredParam, *OptionalParam));
    }
    
    // ===== 3. メイン処理 =====
    // GameThread での実行が必要な場合
    bool bSuccess = false;
    FString ErrorMessage;
    
    if (IsInGameThread())
    {
        bSuccess = DoActualWork(Blueprint, ErrorMessage);
    }
    else
    {
        FEvent* DoneEvent = FPlatformProcess::GetSynchEventFromPool(true);
        AsyncTask(ENamedThreads::GameThread, [&]()
        {
            bSuccess = DoActualWork(Blueprint, ErrorMessage);
            DoneEvent->Trigger();
        });
        DoneEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(DoneEvent);
    }
    
    // ===== 4. レスポンス作成 =====
    if (!bSuccess)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::OperationFailed,
            ErrorMessage);
    }
    
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), RequiredParam);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Result);
}
```

### GameThread 処理の簡略版

```cpp
// 短い処理の場合
if (!IsInGameThread())
{
    TSharedPtr<FJsonObject> Result;
    FEvent* DoneEvent = FPlatformProcess::GetSynchEventFromPool(true);
    AsyncTask(ENamedThreads::GameThread, [&]()
    {
        Result = HandleXxxCommand(Params);  // 再帰呼び出し
        DoneEvent->Trigger();
    });
    DoneEvent->Wait();
    FPlatformProcess::ReturnSynchEventToPool(DoneEvent);
    return Result;
}

// ここからはGameThread保証
// ...処理...
```

---

## 4. テストパターン

### スモークテストの実行

```bash
cd Python
python tests/smoke_test.py
```

### 新コマンドのテスト追加

```python
# tests/test_xxx.py
import pytest
from tests.test_framework import MCPTestClient

class TestXxxCommands:
    @pytest.fixture(autouse=True)
    def setup(self, mcp_client: MCPTestClient):
        self.client = mcp_client
    
    def test_create_xxx_success(self):
        """正常系: Xxx作成"""
        result = self.client.call_tool("create_xxx", {
            "name": "Test_Xxx",
            "path": "/Game/Test"
        })
        
        assert result.get("success") == True
        assert "name" in result
    
    def test_create_xxx_missing_param(self):
        """異常系: 必須パラメータ不足"""
        result = self.client.call_tool("create_xxx", {})
        
        assert result.get("success") == False
        assert result.get("error_code") == 1003  # MissingRequiredParam
    
    def test_create_xxx_not_found(self):
        """異常系: リソースが見つからない"""
        result = self.client.call_tool("create_xxx", {
            "name": "NonExistent",
            "path": "/Game/Invalid"
        })
        
        assert result.get("success") == False
        assert result.get("error_code") == 1100  # AssetNotFound
```

### テスト実行オプション

```bash
# 全テスト
python tests/run_tests.py

# カテゴリ指定
python tests/run_tests.py -m umg
python tests/run_tests.py -m blueprint

# 特定テストファイル
pytest tests/test_xxx.py -v

# 特定テスト関数
pytest tests/test_xxx.py::TestXxxCommands::test_create_xxx_success -v
```

---

## 5. ファイル分割ガイドライン

### 分割の判断基準

| ファイルサイズ | アクション |
|--------------|-----------|
| < 40KB | そのまま |
| 40-60KB | 監視対象、機能追加時に分割検討 |
| > 60KB | 分割推奨 |
| > 80KB | 分割必須 |

### 分割の手順

1. **機能グループの特定**
   - Core: 作成/削除/基本操作
   - Sub1: 特定機能グループA
   - Sub2: 特定機能グループB

2. **ヘッダーファイル作成**
   - 分割先クラスのヘッダーを作成
   - 必要なインクルードを移動

3. **実装ファイル作成**
   - 関数を分割先に移動
   - 依存関係を解決

4. **ルーター作成**
   - 元のファイルをルーターに変換
   - HandleCommand でルーティング

5. **SpirrowBridge.cpp 更新**
   - 新しいルーティングが必要な場合のみ
   - 既存ルーターを使う場合は不要

6. **ビルド & テスト**
   ```bash
   # Unreal Editor でビルド
   # または
   # UnrealBuildTool でビルド
   
   # テスト
   python tests/smoke_test.py
   ```

### 現在の分割状況

| ファイル | 状態 | サイズ |
|---------|------|--------|
| BlueprintCommands | ✅ 3分割済み | Router + 23/26/21 KB |
| BlueprintNodeCommands | ✅ 3分割済み | Router + 24/14/21 KB |
| UMGWidgetCommands | ✅ 3分割済み | Router + 7/17/30 KB |
| UMGLayoutCommands | 単独 | 32 KB |
| UMGAnimationCommands | 単独 | 23 KB |
| UMGVariableCommands | 単独 | 40 KB |
| GASCommands | 📋 分割候補 | 55 KB |
| CommonUtils | 📋 分割候補 | 47 KB |

---

## 6. Python側ツール追加パターン

### 新しいMCPツールの追加

```python
# Python/tools/xxx_tools.py

from mcp.server.fastmcp import FastMCP

def register_xxx_tools(mcp: FastMCP, send_command):
    """Xxx関連ツールを登録"""
    
    @mcp.tool()
    async def create_xxx(
        name: str,
        path: str = "/Game/Default",
        option: bool = False
    ) -> dict:
        """
        Create a new Xxx.
        
        Args:
            name: Name of the Xxx
            path: Content browser path (default: /Game/Default)
            option: Optional flag
            
        Returns:
            Dict containing created Xxx details
        """
        return await send_command("create_xxx", {
            "name": name,
            "path": path,
            "option": option
        })
```

### ツール登録

```python
# Python/server.py
from tools.xxx_tools import register_xxx_tools

# 登録
register_xxx_tools(mcp, send_command)
```

---

## クイックリファレンス

### よく使うエラーコード

```cpp
ESpirrowErrorCode::Success              // 0
ESpirrowErrorCode::UnknownCommand       // 1001
ESpirrowErrorCode::MissingRequiredParam // 1003
ESpirrowErrorCode::InvalidParameter     // 1006
ESpirrowErrorCode::OperationFailed      // 1007
ESpirrowErrorCode::BlueprintNotFound    // 1200
ESpirrowErrorCode::NodeCreationFailed   // 1204
ESpirrowErrorCode::WidgetNotFound       // 1300
ESpirrowErrorCode::ActorNotFound        // 1400
```

### よく使うユーティリティ関数

```cpp
// パラメータ検証
ValidateRequiredString(Params, "key", OutValue)
GetOptionalString(Params, "key", DefaultValue)
GetOptionalNumber(Params, "key", DefaultValue)
GetOptionalBool(Params, "key", DefaultValue)

// リソース検索
FindBlueprintByName(Name, Path)
ValidateBlueprint(Params, OutBlueprint)
ValidateWidgetBlueprint(Params, OutWidget)

// レスポンス作成
CreateSuccessResponse(ResultObject)
CreateErrorResponse(ErrorCode, Message)
CreateErrorResponse(ErrorCode, Message, Details)
```

---

## 更新履歴

| 日付 | 内容 |
|------|------|
| 2026-01-03 | 初版作成 (Phase E) |
