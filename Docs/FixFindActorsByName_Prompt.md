# 機能名: find_actors_by_name レスポンス表示問題の修正

**ステータス: ✅ 実装完了（2026-01-01）**

## 概要

`find_actors_by_name`ツールのレスポンスがClaude.aiのMCPインターフェースで表示されない問題を修正する。

## 現状の問題

1. ツール実行は成功している（エラーなし）
2. しかしレスポンスが表示されない（"Tool ran without output or errors"）
3. `get_actors_in_level`（`List[Dict]`を返す）は正常に表示される
4. `find_actors_by_name`（`Dict[str, Any]`を返す）は表示されない

## 原因仮説

MCPのツール結果表示において、`Dict`型の戻り値がClaude.ai側で正しく処理されていない可能性がある。

または、C++側のレスポンスに`success`フィールドがないため、Python側の処理で問題が発生している可能性がある。

## 修正案

### 案1: C++側に`success`フィールドを追加

**ファイル:** `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeEditorCommands.cpp`

**修正箇所:** `HandleFindActorsByName`関数

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FSpirrowBridgeCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);  // 追加
    ResultObj->SetStringField(TEXT("pattern"), Pattern);  // 追加
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    ResultObj->SetNumberField(TEXT("count"), MatchingActors.Num());  // 追加
    
    return ResultObj;
}
```

### 案2: Python側のレスポンス形式を変更

**ファイル:** `Python/tools/editor_tools.py`

`find_actors_by_name`の戻り値を`Dict`ではなく、`get_actors_in_level`と同様に`List[Dict]`にする。

```python
@mcp.tool()
def find_actors_by_name(ctx: Context, pattern: str) -> List[Dict[str, Any]]:
    """Find actors by name pattern.
    
    Args:
        ctx: The MCP context
        pattern: Name pattern to search for (partial match)
        
    Returns:
        List of matching actors
    """
    from unreal_mcp_server import get_unreal_connection
    
    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.warning("Failed to connect to Unreal Engine")
            return []
            
        response = unreal.send_command("find_actors_by_name", {
            "pattern": pattern
        })
        
        if not response:
            return []
        
        # Log for debugging
        logger.info(f"find_actors_by_name response: {response}")
        
        # Handle different response formats
        actors = []
        if "result" in response and "actors" in response["result"]:
            actors = response["result"]["actors"]
        elif "actors" in response:
            actors = response["actors"]
            
        return actors
        
    except Exception as e:
        logger.error(f"Error finding actors: {e}")
        return []
```

## 推奨

**案1（C++側修正）を推奨**

理由：
- 一貫性のあるAPIレスポンス形式（`success`, `count`などのメタ情報を含む）
- Python側の型定義（`Dict[str, Any]`）はそのまま維持
- 他の類似ツールとの整合性

## テスト手順

1. C++側を修正してリビルド
2. UEエディタを再起動
3. 以下のテストを実行：

```python
# 存在するパターン
find_actors_by_name(pattern="Light")
find_actors_by_name(pattern="Floor")
find_actors_by_name(pattern="Test")

# 存在しないパターン
find_actors_by_name(pattern="NONEXISTENT")
```

期待される結果：
```json
{
  "success": true,
  "pattern": "Light",
  "actors": [...],
  "count": 2
}
```
