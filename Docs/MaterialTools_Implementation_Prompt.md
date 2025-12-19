# マテリアル作成ツール実装

## 概要

シンプルなマテリアルを MCP 経由で作成できるツール。
組み込みテンプレート（JSON）とユーザー定義テンプレート（ChromaDB）の2層構造。

## アーキテクチャ

```
[MCP Tools]
├── create_material_from_template()  # テンプレートから作成
├── create_simple_material()         # 詳細設定で作成
├── list_material_templates()        # テンプレート一覧
├── save_material_template()         # ユーザー定義保存
├── delete_material_template()       # ユーザー定義削除
└── get_material_template()          # テンプレート取得

[Templates]
├── templates/materials/*.json       # 組み込み（builtin）
└── ChromaDB (category="material_template")  # ユーザー定義
```

## ファイル構成

```
spirrow-unrealwise/
├── Python/
│   └── tools/
│       └── material_tools.py        # 新規作成
├── templates/
│   └── materials/
│       ├── solid.json
│       ├── translucent.json
│       ├── unlit.json
│       ├── unlit_translucent.json
│       └── emissive.json
└── MCPGameProject/
    └── Plugins/SpirrowBridge/
        └── Source/SpirrowBridge/
            ├── Public/Commands/
            │   └── SpirrowBridgeMaterialCommands.h   # 新規作成
            └── Private/Commands/
                └── SpirrowBridgeMaterialCommands.cpp # 新規作成
```

---

## Part 1: 組み込みテンプレート（JSON）

### templates/materials/solid.json

```json
{
  "name": "solid",
  "description": "Basic opaque solid color material",
  "shading_model": "DefaultLit",
  "blend_mode": "Opaque",
  "two_sided": false,
  "parameters": {
    "base_color": [1.0, 1.0, 1.0]
  }
}
```

### templates/materials/translucent.json

```json
{
  "name": "translucent",
  "description": "Translucent material with adjustable opacity",
  "shading_model": "DefaultLit",
  "blend_mode": "Translucent",
  "two_sided": false,
  "parameters": {
    "base_color": [1.0, 1.0, 1.0],
    "opacity": 0.5
  }
}
```

### templates/materials/unlit.json

```json
{
  "name": "unlit",
  "description": "Unlit opaque material (no lighting)",
  "shading_model": "Unlit",
  "blend_mode": "Opaque",
  "two_sided": false,
  "parameters": {
    "emissive_color": [1.0, 1.0, 1.0]
  }
}
```

### templates/materials/unlit_translucent.json

```json
{
  "name": "unlit_translucent",
  "description": "Unlit translucent material - good for effects, detection spheres",
  "shading_model": "Unlit",
  "blend_mode": "Translucent",
  "two_sided": true,
  "parameters": {
    "emissive_color": [1.0, 1.0, 1.0],
    "opacity": 0.3
  }
}
```

### templates/materials/emissive.json

```json
{
  "name": "emissive",
  "description": "Emissive material with glow effect",
  "shading_model": "DefaultLit",
  "blend_mode": "Opaque",
  "two_sided": false,
  "parameters": {
    "base_color": [0.0, 0.0, 0.0],
    "emissive_color": [1.0, 1.0, 1.0],
    "emissive_strength": 1.0
  }
}
```

---

## Part 2: Python MCP ツール

### Python/tools/material_tools.py

```python
"""
Material creation and template management tools for SpirrowUnrealWise MCP.
"""

import json
import os
from pathlib import Path
from typing import Dict, Any, Optional, List
from mcp.server.fastmcp import Context

# テンプレートディレクトリのパス
TEMPLATES_DIR = Path(__file__).parent.parent.parent / "templates" / "materials"


def get_unreal_connection():
    """Get the Unreal Engine connection."""
    from tools.editor_tools import get_unreal_connection as get_conn
    return get_conn()


def send_command(command_type: str, params: dict) -> dict:
    """Send command to Unreal Engine."""
    from tools.editor_tools import send_command as send_cmd
    return send_cmd(command_type, params)


def load_builtin_templates() -> Dict[str, dict]:
    """Load all builtin templates from JSON files."""
    templates = {}
    if TEMPLATES_DIR.exists():
        for json_file in TEMPLATES_DIR.glob("*.json"):
            try:
                with open(json_file, "r", encoding="utf-8") as f:
                    template = json.load(f)
                    templates[template["name"]] = template
            except Exception as e:
                print(f"Error loading template {json_file}: {e}")
    return templates


def get_builtin_template(name: str) -> Optional[dict]:
    """Get a specific builtin template by name."""
    template_file = TEMPLATES_DIR / f"{name}.json"
    if template_file.exists():
        with open(template_file, "r", encoding="utf-8") as f:
            return json.load(f)
    return None


def register_material_tools(mcp):
    """Register all material tools with the MCP server."""

    @mcp.tool()
    async def list_material_templates(
        ctx: Context,
        source: str = "all"
    ) -> Dict[str, Any]:
        """
        List available material templates.

        Args:
            source: Filter by source - "builtin", "user", or "all" (default)

        Returns:
            Dict containing list of templates with their descriptions
        """
        result = {
            "builtin": [],
            "user": []
        }

        # Load builtin templates
        if source in ["builtin", "all"]:
            builtin = load_builtin_templates()
            for name, template in builtin.items():
                result["builtin"].append({
                    "name": name,
                    "description": template.get("description", ""),
                    "blend_mode": template.get("blend_mode", "Opaque"),
                    "shading_model": template.get("shading_model", "DefaultLit"),
                    "two_sided": template.get("two_sided", False)
                })

        # Load user templates from RAG
        if source in ["user", "all"]:
            try:
                from tools.rag_tools import search_knowledge_internal
                rag_results = search_knowledge_internal(
                    query="",
                    category="material_template",
                    n_results=100
                )
                if rag_results and "documents" in rag_results:
                    for doc in rag_results["documents"]:
                        try:
                            template = json.loads(doc)
                            result["user"].append({
                                "name": template.get("name", "unknown"),
                                "description": template.get("description", ""),
                                "blend_mode": template.get("blend_mode", "Opaque"),
                                "shading_model": template.get("shading_model", "DefaultLit"),
                                "two_sided": template.get("two_sided", False)
                            })
                        except json.JSONDecodeError:
                            pass
            except Exception as e:
                result["user_error"] = str(e)

        result["total_builtin"] = len(result["builtin"])
        result["total_user"] = len(result["user"])
        return result

    @mcp.tool()
    async def get_material_template(
        ctx: Context,
        name: str
    ) -> Dict[str, Any]:
        """
        Get a material template by name.

        Args:
            name: Template name (checks builtin first, then user-defined)

        Returns:
            Dict containing template definition or error
        """
        # Check builtin first
        builtin = get_builtin_template(name)
        if builtin:
            return {
                "success": True,
                "source": "builtin",
                "template": builtin
            }

        # Check user-defined in RAG
        try:
            from tools.rag_tools import search_knowledge_internal
            rag_results = search_knowledge_internal(
                query=name,
                category="material_template",
                n_results=10
            )
            if rag_results and "documents" in rag_results:
                for doc in rag_results["documents"]:
                    try:
                        template = json.loads(doc)
                        if template.get("name") == name:
                            return {
                                "success": True,
                                "source": "user",
                                "template": template
                            }
                    except json.JSONDecodeError:
                        pass
        except Exception as e:
            return {"success": False, "error": f"RAG search failed: {e}"}

        return {"success": False, "error": f"Template not found: {name}"}

    @mcp.tool()
    async def save_material_template(
        ctx: Context,
        name: str,
        description: str = "",
        shading_model: str = "DefaultLit",
        blend_mode: str = "Opaque",
        two_sided: bool = False,
        base_color: List[float] = None,
        opacity: float = 1.0,
        emissive_color: List[float] = None,
        emissive_strength: float = 1.0,
        tags: str = ""
    ) -> Dict[str, Any]:
        """
        Save a user-defined material template to RAG.

        Args:
            name: Template name (must be unique)
            description: Description of the template
            shading_model: DefaultLit or Unlit
            blend_mode: Opaque, Translucent, or Masked
            two_sided: Whether to render both sides
            base_color: [R, G, B] values 0.0-1.0
            opacity: Opacity value 0.0-1.0 (for Translucent)
            emissive_color: [R, G, B] emissive color
            emissive_strength: Emissive intensity multiplier
            tags: Comma-separated tags for search

        Returns:
            Dict containing success status
        """
        template = {
            "name": name,
            "description": description,
            "shading_model": shading_model,
            "blend_mode": blend_mode,
            "two_sided": two_sided,
            "parameters": {}
        }

        if base_color:
            template["parameters"]["base_color"] = base_color
        if blend_mode == "Translucent":
            template["parameters"]["opacity"] = opacity
        if emissive_color:
            template["parameters"]["emissive_color"] = emissive_color
            template["parameters"]["emissive_strength"] = emissive_strength

        try:
            from tools.rag_tools import add_knowledge_internal
            result = add_knowledge_internal(
                document=json.dumps(template),
                category="material_template",
                tags=tags,
                doc_id=f"material_template_{name}"
            )
            return {
                "success": True,
                "name": name,
                "message": f"Template '{name}' saved successfully",
                "rag_result": result
            }
        except Exception as e:
            return {"success": False, "error": str(e)}

    @mcp.tool()
    async def delete_material_template(
        ctx: Context,
        name: str
    ) -> Dict[str, Any]:
        """
        Delete a user-defined material template.

        Args:
            name: Template name to delete

        Returns:
            Dict containing success status
        """
        # Cannot delete builtin templates
        if get_builtin_template(name):
            return {
                "success": False,
                "error": f"Cannot delete builtin template: {name}"
            }

        try:
            from tools.rag_tools import delete_knowledge_internal
            result = delete_knowledge_internal(doc_id=f"material_template_{name}")
            return {
                "success": True,
                "name": name,
                "message": f"Template '{name}' deleted successfully",
                "rag_result": result
            }
        except Exception as e:
            return {"success": False, "error": str(e)}

    @mcp.tool()
    async def create_material_from_template(
        ctx: Context,
        name: str,
        template: str,
        path: str = "/Game/Materials",
        color: List[float] = None,
        opacity: float = None,
        two_sided: bool = None
    ) -> Dict[str, Any]:
        """
        Create a material from a template.

        Args:
            name: Name for the new material (e.g., "M_DetectionSphere")
            template: Template name (builtin or user-defined)
            path: Content browser path for the material
            color: Override [R, G, B] color (0.0-1.0)
            opacity: Override opacity (0.0-1.0)
            two_sided: Override two-sided setting

        Returns:
            Dict containing created material path or error

        Example:
            create_material_from_template(
                name="M_DetectionSphere",
                template="unlit_translucent",
                path="/Game/TrapxTrap/Materials",
                color=[0.2, 0.5, 1.0],
                opacity=0.3,
                two_sided=True
            )
        """
        # Get template
        template_result = await get_material_template(ctx, template)
        if not template_result.get("success"):
            return template_result

        template_data = template_result["template"]

        # Build material parameters with overrides
        params = {
            "name": name,
            "path": path,
            "shading_model": template_data.get("shading_model", "DefaultLit"),
            "blend_mode": template_data.get("blend_mode", "Opaque"),
            "two_sided": two_sided if two_sided is not None else template_data.get("two_sided", False)
        }

        # Handle color/emissive based on shading model
        template_params = template_data.get("parameters", {})
        
        if params["shading_model"] == "Unlit":
            # Unlit uses emissive_color
            params["emissive_color"] = color if color else template_params.get("emissive_color", [1.0, 1.0, 1.0])
        else:
            # DefaultLit uses base_color
            params["base_color"] = color if color else template_params.get("base_color", [1.0, 1.0, 1.0])
            if template_params.get("emissive_color"):
                params["emissive_color"] = template_params["emissive_color"]
                params["emissive_strength"] = template_params.get("emissive_strength", 1.0)

        # Opacity
        if params["blend_mode"] == "Translucent":
            params["opacity"] = opacity if opacity is not None else template_params.get("opacity", 0.5)

        # Send to Unreal
        return send_command("create_simple_material", params)

    @mcp.tool()
    async def create_simple_material(
        ctx: Context,
        name: str,
        path: str = "/Game/Materials",
        shading_model: str = "DefaultLit",
        blend_mode: str = "Opaque",
        two_sided: bool = False,
        base_color: List[float] = None,
        opacity: float = 1.0,
        emissive_color: List[float] = None,
        emissive_strength: float = 1.0
    ) -> Dict[str, Any]:
        """
        Create a simple material with detailed settings.

        Args:
            name: Material name (e.g., "M_MyMaterial")
            path: Content browser path (default: "/Game/Materials")
            shading_model: "DefaultLit" or "Unlit"
            blend_mode: "Opaque", "Translucent", or "Masked"
            two_sided: Whether to render both sides of faces
            base_color: [R, G, B] base color (0.0-1.0), used for DefaultLit
            opacity: Opacity value (0.0-1.0), only for Translucent blend mode
            emissive_color: [R, G, B] emissive/glow color (0.0-1.0)
            emissive_strength: Emissive intensity multiplier

        Returns:
            Dict containing created material path or error

        Example:
            create_simple_material(
                name="M_GlowingRed",
                blend_mode="Translucent",
                shading_model="Unlit",
                emissive_color=[1.0, 0.0, 0.0],
                opacity=0.5,
                two_sided=True
            )
        """
        params = {
            "name": name,
            "path": path,
            "shading_model": shading_model,
            "blend_mode": blend_mode,
            "two_sided": two_sided,
            "opacity": opacity
        }

        if base_color:
            params["base_color"] = base_color
        if emissive_color:
            params["emissive_color"] = emissive_color
            params["emissive_strength"] = emissive_strength

        return send_command("create_simple_material", params)
```

---

## Part 3: Unreal C++ コマンドハンドラ

### SpirrowBridgeMaterialCommands.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FSpirrowBridgeMaterialCommands
{
public:
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateSimpleMaterial(const TSharedPtr<FJsonObject>& Params);
};
```

### SpirrowBridgeMaterialCommands.cpp

```cpp
#include "Commands/SpirrowBridgeMaterialCommands.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Factories/MaterialFactoryNew.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"

TSharedPtr<FJsonObject> FSpirrowBridgeMaterialCommands::HandleCommand(
    const FString& CommandType,
    const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_simple_material"))
    {
        return HandleCreateSimpleMaterial(Params);
    }

    // Unknown command
    TSharedPtr<FJsonObject> ErrorResponse = MakeShareable(new FJsonObject());
    ErrorResponse->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown material command: %s"), *CommandType));
    return ErrorResponse;
}

TSharedPtr<FJsonObject> FSpirrowBridgeMaterialCommands::HandleCreateSimpleMaterial(
    const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());

    // Get parameters
    FString Name = Params->GetStringField(TEXT("name"));
    FString Path = Params->GetStringField(TEXT("path"));
    FString ShadingModelStr = Params->GetStringField(TEXT("shading_model"));
    FString BlendModeStr = Params->GetStringField(TEXT("blend_mode"));
    bool bTwoSided = Params->GetBoolField(TEXT("two_sided"));
    float Opacity = Params->GetNumberField(TEXT("opacity"));

    // Parse colors
    FLinearColor BaseColor = FLinearColor::White;
    FLinearColor EmissiveColor = FLinearColor::Black;
    float EmissiveStrength = 1.0f;

    const TArray<TSharedPtr<FJsonValue>>* BaseColorArray;
    if (Params->TryGetArrayField(TEXT("base_color"), BaseColorArray) && BaseColorArray->Num() >= 3)
    {
        BaseColor = FLinearColor(
            (*BaseColorArray)[0]->AsNumber(),
            (*BaseColorArray)[1]->AsNumber(),
            (*BaseColorArray)[2]->AsNumber()
        );
    }

    const TArray<TSharedPtr<FJsonValue>>* EmissiveColorArray;
    if (Params->TryGetArrayField(TEXT("emissive_color"), EmissiveColorArray) && EmissiveColorArray->Num() >= 3)
    {
        EmissiveColor = FLinearColor(
            (*EmissiveColorArray)[0]->AsNumber(),
            (*EmissiveColorArray)[1]->AsNumber(),
            (*EmissiveColorArray)[2]->AsNumber()
        );
        EmissiveStrength = Params->GetNumberField(TEXT("emissive_strength"));
    }

    // Create package
    FString PackagePath = Path / Name;
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to create package"));
        return Response;
    }

    // Create material
    UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
    UMaterial* Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
        UMaterial::StaticClass(),
        Package,
        *Name,
        RF_Public | RF_Standalone,
        nullptr,
        GWarn
    ));

    if (!Material)
    {
        Response->SetBoolField(TEXT("success"), false);
        Response->SetStringField(TEXT("error"), TEXT("Failed to create material"));
        return Response;
    }

    // Set blend mode
    if (BlendModeStr == TEXT("Translucent"))
    {
        Material->BlendMode = BLEND_Translucent;
    }
    else if (BlendModeStr == TEXT("Masked"))
    {
        Material->BlendMode = BLEND_Masked;
    }
    else
    {
        Material->BlendMode = BLEND_Opaque;
    }

    // Set shading model
    if (ShadingModelStr == TEXT("Unlit"))
    {
        Material->SetShadingModel(MSM_Unlit);
    }

    // Set two sided
    Material->TwoSided = bTwoSided;

    // Create material expressions
    int32 NodePosX = -300;
    int32 NodePosY = 0;

    // For Unlit: use Emissive Color
    // For DefaultLit: use Base Color
    if (ShadingModelStr == TEXT("Unlit"))
    {
        // Emissive Color node
        UMaterialExpressionConstant3Vector* EmissiveNode = NewObject<UMaterialExpressionConstant3Vector>(Material);
        EmissiveNode->Constant = EmissiveColor;
        EmissiveNode->MaterialExpressionEditorX = NodePosX;
        EmissiveNode->MaterialExpressionEditorY = NodePosY;
        Material->GetExpressionCollection().AddExpression(EmissiveNode);
        Material->GetEditorOnlyData()->EmissiveColor.Connect(0, EmissiveNode);
    }
    else
    {
        // Base Color node
        UMaterialExpressionConstant3Vector* BaseColorNode = NewObject<UMaterialExpressionConstant3Vector>(Material);
        BaseColorNode->Constant = BaseColor;
        BaseColorNode->MaterialExpressionEditorX = NodePosX;
        BaseColorNode->MaterialExpressionEditorY = NodePosY;
        Material->GetExpressionCollection().AddExpression(BaseColorNode);
        Material->GetEditorOnlyData()->BaseColor.Connect(0, BaseColorNode);

        // Emissive if specified
        if (EmissiveColor != FLinearColor::Black)
        {
            UMaterialExpressionConstant3Vector* EmissiveNode = NewObject<UMaterialExpressionConstant3Vector>(Material);
            EmissiveNode->Constant = EmissiveColor * EmissiveStrength;
            EmissiveNode->MaterialExpressionEditorX = NodePosX;
            EmissiveNode->MaterialExpressionEditorY = NodePosY + 150;
            Material->GetExpressionCollection().AddExpression(EmissiveNode);
            Material->GetEditorOnlyData()->EmissiveColor.Connect(0, EmissiveNode);
        }
    }

    // Opacity for Translucent
    if (Material->BlendMode == BLEND_Translucent)
    {
        UMaterialExpressionConstant* OpacityNode = NewObject<UMaterialExpressionConstant>(Material);
        OpacityNode->R = Opacity;
        OpacityNode->MaterialExpressionEditorX = NodePosX;
        OpacityNode->MaterialExpressionEditorY = NodePosY + 300;
        Material->GetExpressionCollection().AddExpression(OpacityNode);
        Material->GetEditorOnlyData()->Opacity.Connect(0, OpacityNode);
    }

    // Compile and save
    Material->PreEditChange(nullptr);
    Material->PostEditChange();

    // Mark package dirty and save
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Material);

    // Save the package
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Material, *PackageFileName, SaveArgs);

    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("asset_path"), PackagePath);
    Response->SetStringField(TEXT("name"), Name);
    Response->SetStringField(TEXT("blend_mode"), BlendModeStr);
    Response->SetStringField(TEXT("shading_model"), ShadingModelStr);
    Response->SetBoolField(TEXT("two_sided"), bTwoSided);

    return Response;
}
```

---

## Part 4: SpirrowBridge.cpp への統合

### SpirrowBridge.cpp に追加

```cpp
// ヘッダー追加
#include "Commands/SpirrowBridgeMaterialCommands.h"

// メンバー変数追加（クラス内）
TSharedPtr<FSpirrowBridgeMaterialCommands> MaterialCommands;

// Initialize() に追加
MaterialCommands = MakeShareable(new FSpirrowBridgeMaterialCommands());

// ExecuteCommand() のルーティングに追加
else if (CommandType == TEXT("create_simple_material"))
{
    ResultJson = MaterialCommands->HandleCommand(CommandType, Params);
}
```

---

## Part 5: server.py への統合

### Python/server.py に追加

```python
# import 追加
from tools.material_tools import register_material_tools

# 登録（既存の register_xxx_tools の後に追加）
register_material_tools(mcp)
```

---

## Part 6: RAG tools 内部関数追加

### Python/tools/rag_tools.py に追加

既存の `add_knowledge`, `search_knowledge`, `delete_knowledge` に対応する内部関数を追加（`async` なしで他のツールから呼べるように）:

```python
def add_knowledge_internal(document: str, category: str, tags: str = None, doc_id: str = None) -> dict:
    """Internal synchronous version of add_knowledge"""
    # 既存の add_knowledge のロジックを同期関数として実装
    ...

def search_knowledge_internal(query: str, category: str = None, n_results: int = 5) -> dict:
    """Internal synchronous version of search_knowledge"""
    ...

def delete_knowledge_internal(doc_id: str) -> dict:
    """Internal synchronous version of delete_knowledge"""
    ...
```

---

## テスト手順

1. SpirrowBridge プラグインをビルド
2. MCP サーバー再起動
3. テスト実行:

```python
# テンプレート一覧
list_material_templates()

# 組み込みテンプレートから作成
create_material_from_template(
    name="M_DetectionSphere",
    template="unlit_translucent",
    path="/Game/TrapxTrap/Materials",
    color=[0.2, 0.5, 1.0],
    opacity=0.3,
    two_sided=True
)

# ユーザーテンプレート保存
save_material_template(
    name="detection_effect",
    description="Semi-transparent detection effect",
    shading_model="Unlit",
    blend_mode="Translucent",
    two_sided=True,
    emissive_color=[0.2, 0.5, 1.0],
    opacity=0.3,
    tags="detection,effect,translucent"
)

# ユーザーテンプレートから作成
create_material_from_template(
    name="M_DetectionSphere2",
    template="detection_effect",
    path="/Game/TrapxTrap/Materials"
)
```

---

## 注意事項

- UE5.7 の Material API を使用（バージョンによって微妙に異なる可能性）
- `GetEditorOnlyData()` は UE5 以降で使用
- RAG 内部関数は同期呼び出しになるため、async コンテキスト外でも使用可能に
