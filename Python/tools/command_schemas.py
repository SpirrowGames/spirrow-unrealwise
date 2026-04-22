"""Command schemas for SpirrowBridge meta-tools help system.

Complete registry of all MCP tool commands with parameter documentation.
Used by the help() tool to provide on-demand parameter info.
"""

COMMAND_SCHEMAS = {
    # =========================================================================
    # EDITOR (17 commands)
    # =========================================================================
    "editor": {
        "get_actors_in_level": {
            "brief": "List all actors in the current level",
            "params": {},
        },
        "find_actors_by_name": {
            "brief": "Find actors by name pattern",
            "params": {
                "pattern": {"type": "str", "required": True, "desc": "Name pattern to search (partial match)"},
            },
        },
        "spawn_actor": {
            "brief": "Spawn a new actor in the level",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Actor name (must be unique)"},
                "type": {"type": "str", "required": True, "desc": "Actor type (StaticMeshActor, PointLight, SpotLight, DirectionalLight, CameraActor, NavMeshBoundsVolume, TriggerVolume, BlockingVolume, etc.)"},
                "location": {"type": "list[float]", "default": [0, 0, 0], "desc": "World [x,y,z] location"},
                "rotation": {"type": "list[float]", "default": [0, 0, 0], "desc": "Rotation [pitch,yaw,roll] in degrees"},
                "scale": {"type": "list[float]", "default": [1, 1, 1], "desc": "Scale [x,y,z]"},
                "brush_size": {"type": "list[float]", "desc": "For volumes: explicit [x,y,z] brush dimensions"},
                "rationale": {"type": "str", "desc": "Design rationale (auto-saved to knowledge base)"},
            },
        },
        "delete_actor": {
            "brief": "Delete an actor by name",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Actor name"},
            },
        },
        "set_actor_transform": {
            "brief": "Set actor transform (location/rotation/scale)",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Actor name"},
                "location": {"type": "list[float]", "desc": "[x,y,z] location"},
                "rotation": {"type": "list[float]", "desc": "[pitch,yaw,roll] rotation"},
                "scale": {"type": "list[float]", "desc": "[x,y,z] scale"},
            },
        },
        "get_actor_properties": {
            "brief": "Get all properties of an actor",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Actor name"},
            },
        },
        "set_actor_property": {
            "brief": "Set a property on an actor",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Actor name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "property_value": {"type": "any", "required": True, "desc": "Property value"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "set_actor_component_property": {
            "brief": "Set a property on a component of an actor in the level",
            "params": {
                "actor_name": {"type": "str", "required": True, "desc": "Actor name"},
                "component_name": {"type": "str", "required": True, "desc": "Component name (e.g. StaticMeshComponent0)"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "property_value": {"type": "any", "required": True, "desc": "Property value"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "get_actor_components": {
            "brief": "Get all components of an actor",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Actor name"},
            },
        },
        "rename_actor": {
            "brief": "Rename an actor",
            "params": {
                "current_name": {"type": "str", "required": True, "desc": "Current actor name"},
                "new_name": {"type": "str", "required": True, "desc": "New actor name"},
            },
        },
        "spawn_blueprint_actor": {
            "brief": "Spawn an actor from a Blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name to spawn from"},
                "actor_name": {"type": "str", "required": True, "desc": "Name for spawned actor"},
                "location": {"type": "list[float]", "default": [0, 0, 0], "desc": "World location"},
                "rotation": {"type": "list[float]", "default": [0, 0, 0], "desc": "Rotation in degrees"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path of the blueprint"},
            },
        },
        "rename_asset": {
            "brief": "Rename an asset in Content Browser",
            "params": {
                "old_path": {"type": "str", "required": True, "desc": "Current asset path"},
                "new_name": {"type": "str", "required": True, "desc": "New name (not full path)"},
            },
        },
        "create_level": {
            "brief": "Create a new Level (.umap) on disk and switch editor to it",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Level name (no slashes/spaces)"},
                "path": {"type": "str", "default": "/Game/Maps", "desc": "Content folder (must start with /Game/)"},
                "template": {"type": "str", "default": "default", "desc": "'default' (UE 5.7 default = World Partition enabled), 'empty' (/Engine/Maps/Templates/Empty = non-WP), or an explicit /Game/... path to an existing level asset"},
                "overwrite": {"type": "bool", "default": False, "desc": "If true, replace any existing level at the same path"},
            },
        },
        "save_current_level": {
            "brief": "Save the currently-open Level to disk",
            "params": {},
        },
        "open_level": {
            "brief": "Load an existing Level (.umap) into the editor",
            "params": {
                "level_path": {"type": "str", "required": True, "desc": "Asset path (e.g. '/Game/Maps/MyMap'). Must already exist"},
            },
        },
        "get_world_settings": {
            "brief": "Read properties from the current level's AWorldSettings (singleton per level)",
            "params": {
                "properties": {"type": "list[str]", "default": None, "desc": "Property names to return. If omitted, returns a curated preset of 9 common fields (DefaultGameMode='GameMode Override', DefaultPhysicsVolumeClass, KillZ, KillZDamageType, WorldToMeters, GlobalGravityZ, TimeDilation, bEnableWorldBoundsChecks, bEnableWorldComposition). Unknown names go into 'unknown_properties' array (not an error)"},
            },
        },
        "set_world_properties": {
            "brief": "Batch-set properties on the current level's AWorldSettings",
            "params": {
                "properties": {"type": "dict[str, any]", "required": True, "desc": "Key = UPROPERTY name on AWorldSettings (e.g. 'DefaultGameMode', 'KillZ'), value = typed value (class picker properties take a class path string like '/Game/Blueprints/BP_GM.BP_GM_C'). Partial success: per-property failures are reported in 'failed' array; 'applied' lists successes. success=false only if all failed. Level is marked dirty on any success (call save_current_level to persist)"},
            },
        },
    },

    # =========================================================================
    # BLUEPRINT (21 commands)
    # =========================================================================
    "blueprint": {
        "create_blueprint": {
            "brief": "Create a new Blueprint class",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "parent_class": {"type": "str", "required": True, "desc": "Parent class (Actor, Character, etc.)"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "add_component_to_blueprint": {
            "brief": "Add a component to a Blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Target blueprint"},
                "component_type": {"type": "str", "required": True, "desc": "Component class name (without U prefix)"},
                "component_name": {"type": "str", "required": True, "desc": "Name for new component"},
                "location": {"type": "list[float]", "default": [0, 0, 0], "desc": "Component position"},
                "rotation": {"type": "list[float]", "default": [0, 0, 0], "desc": "Component rotation"},
                "scale": {"type": "list[float]", "default": [1, 1, 1], "desc": "Component scale"},
                "component_properties": {"type": "dict", "default": {}, "desc": "Additional properties"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "set_static_mesh_properties": {
            "brief": "Set static mesh on a StaticMeshComponent",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "required": True, "desc": "Component name"},
                "static_mesh": {"type": "str", "default": "/Engine/BasicShapes/Cube.Cube", "desc": "Static mesh path"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "set_component_property": {
            "brief": "Set a property on a Blueprint component",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "required": True, "desc": "Component name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "property_value": {"type": "any", "required": True, "desc": "Property value"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "set_physics_properties": {
            "brief": "Set physics properties on a component",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "required": True, "desc": "Component name"},
                "simulate_physics": {"type": "bool", "default": True, "desc": "Enable physics simulation"},
                "gravity_enabled": {"type": "bool", "default": True, "desc": "Enable gravity"},
                "mass": {"type": "float", "default": 1.0, "desc": "Mass in kg"},
                "linear_damping": {"type": "float", "default": 0.01, "desc": "Linear damping"},
                "angular_damping": {"type": "float", "default": 0.0, "desc": "Angular damping"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "compile_blueprint": {
            "brief": "Compile a Blueprint (regular or Level Script)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' (default) or 'level_blueprint' to target the Level Script Blueprint"},
                "level_path": {"type": "str", "desc": "Level asset path (e.g. /Game/Maps/MyMap). Omit for the currently edited level. Only used when target_type=level_blueprint"},
            },
        },
        "set_blueprint_property": {
            "brief": "Set a property on Blueprint class defaults",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "property_value": {"type": "any", "required": True, "desc": "Property value"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "duplicate_blueprint": {
            "brief": "Duplicate an existing Blueprint",
            "params": {
                "source_name": {"type": "str", "required": True, "desc": "Source blueprint name"},
                "new_name": {"type": "str", "required": True, "desc": "New blueprint name"},
                "source_path": {"type": "str", "default": "/Game/Blueprints", "desc": "Source content path"},
                "destination_path": {"type": "str", "desc": "Destination path (defaults to source_path)"},
            },
        },
        "get_blueprint_graph": {
            "brief": "Get node graph structure of a Blueprint (regular or Level Script)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' (default) or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
            },
        },
        "scan_project_classes": {
            "brief": "Scan project for C++ classes and Blueprint assets",
            "params": {
                "class_type": {"type": "str", "default": "all", "desc": "Type: cpp, blueprint, or all"},
                "parent_class": {"type": "str", "desc": "Filter by parent class"},
                "module_filter": {"type": "str", "desc": "Filter by module name"},
                "path_filter": {"type": "str", "desc": "Filter by content path"},
                "include_engine": {"type": "bool", "default": False, "desc": "Include engine classes"},
                "exclude_reinst": {"type": "bool", "default": True, "desc": "Exclude REINST classes"},
                "blueprint_type": {"type": "str", "desc": "Filter: actor/widget/anim/controlrig/interface/gamemode/controller/character/pawn"},
            },
        },
        "set_blueprint_class_array": {
            "brief": "Set a TSubclassOf array property",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "class_paths": {"type": "list[str]", "required": True, "desc": "Array of class paths"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "set_struct_array_property": {
            "brief": "Set a struct array property on a Blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "values": {"type": "list[dict]", "required": True, "desc": "Array of struct values"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "create_data_asset": {
            "brief": "Create a new DataAsset",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Asset name"},
                "parent_class": {"type": "str", "required": True, "desc": "Parent DataAsset class"},
                "path": {"type": "str", "default": "/Game/Data", "desc": "Content path"},
                "initial_values": {"type": "dict", "default": {}, "desc": "Initial property values"},
            },
        },
        "set_class_property": {
            "brief": "Set a TSubclassOf property to a class reference",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "class_path": {"type": "str", "required": True, "desc": "Class reference path"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "set_object_property": {
            "brief": "Set a UObject property to an asset reference",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "asset_path": {"type": "str", "required": True, "desc": "Asset reference path"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "get_blueprint_properties": {
            "brief": "Get all configurable properties on a Blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "include_inherited": {"type": "bool", "default": True, "desc": "Include inherited properties"},
                "category_filter": {"type": "str", "desc": "Filter by property category"},
            },
        },
        "set_struct_property": {
            "brief": "Update fields of a struct array element",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "index": {"type": "int", "required": True, "desc": "Array element index"},
                "values": {"type": "dict", "required": True, "desc": "Struct field values"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "set_data_asset_property": {
            "brief": "Set a property on a DataAsset (supports TMap via dict or dot notation)",
            "params": {
                "asset_name": {"type": "str", "required": True, "desc": "DataAsset name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name. Use dot notation for TMap entry: 'MapProp.KeyName'"},
                "property_value": {"type": "any", "required": True, "desc": "Value to set. For TMap: dict to overwrite all, single value with dot notation, null to delete entry"},
                "path": {"type": "str", "default": "/Game/Data", "desc": "Content path"},
            },
        },
        "get_data_asset_properties": {
            "brief": "Get all properties of a DataAsset",
            "params": {
                "asset_name": {"type": "str", "required": True, "desc": "DataAsset name"},
                "path": {"type": "str", "default": "/Game/", "desc": "Content path"},
                "include_inherited": {"type": "bool", "default": True, "desc": "Include inherited properties"},
                "category_filter": {"type": "str", "desc": "Filter by property category"},
            },
        },
        "batch_set_properties": {
            "brief": "Set multiple properties in a single operation",
            "params": {
                "asset_name": {"type": "str", "required": True, "desc": "Asset name"},
                "properties": {"type": "dict", "required": True, "desc": "Properties to set"},
                "path": {"type": "str", "default": "/Game/Data", "desc": "Content path"},
                "asset_type": {"type": "str", "default": "dataasset", "desc": "Asset type"},
            },
        },
        "find_cpp_function_in_blueprints": {
            "brief": "Find all Blueprint locations where a function is called",
            "params": {
                "function_name": {"type": "str", "required": True, "desc": "Function name to search for"},
                "class_name": {"type": "str", "desc": "Filter by class name"},
                "path_filter": {"type": "str", "desc": "Filter by content path"},
                "include_blueprint_functions": {"type": "bool", "default": True, "desc": "Include Blueprint-defined functions"},
            },
        },
    },

    # =========================================================================
    # BLUEPRINT_NODE (21 commands)
    #
    # All commands in this section accept two optional params (not shown in
    # every schema entry for brevity):
    #   target_type: "blueprint" (default) or "level_blueprint"
    #   level_path:  "/Game/Maps/MyMap" etc. Only used when
    #                target_type=level_blueprint; omit to target the currently
    #                edited persistent level's Level Script Blueprint.
    # When target_type=level_blueprint, blueprint_name / path are ignored.
    # =========================================================================
    "blueprint_node": {
        "add_blueprint_event_node": {
            "brief": "Add an event node (works on regular BP or Level Script BP)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "event_name": {"type": "str", "required": True, "desc": "Event name (e.g. BeginPlay, Tick, ActorBeginOverlap for LSB)"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' (default) or 'level_blueprint' to target the Level Script Blueprint"},
                "level_path": {"type": "str", "desc": "Level asset path (e.g. /Game/Maps/MyMap). Omit for current level. Only used when target_type=level_blueprint"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "add_blueprint_input_action_node": {
            "brief": "Add an input action event node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "action_name": {"type": "str", "required": True, "desc": "Input action name"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_blueprint_function_node": {
            "brief": "Add a function call node (regular BP or LSB). Falls back to external UPROPERTY Set/Get if function_name is Set<Prop>/Get<Prop> and no matching function exists",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "target": {"type": "str", "required": True, "desc": "Target class name (function owner, or owner of UPROPERTY for fallback)"},
                "function_name": {"type": "str", "required": True, "desc": "Function name. If not found and starts with Set/Get/K2_Set/K2_Get, the handler falls back to spawning a UK2Node_VariableSet/Get for the matching UPROPERTY"},
                "params": {"type": "dict", "default": {}, "desc": "Function parameters"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "connect_blueprint_nodes": {
            "brief": "Connect two Blueprint nodes (works on regular BP or Level Script BP)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "source_node_id": {"type": "str", "required": True, "desc": "Source node ID"},
                "source_pin": {"type": "str", "required": True, "desc": "Source pin name"},
                "target_node_id": {"type": "str", "required": True, "desc": "Target node ID"},
                "target_pin": {"type": "str", "required": True, "desc": "Target pin name"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
            },
        },
        "add_blueprint_variable": {
            "brief": "Add a variable to a Blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "variable_name": {"type": "str", "required": True, "desc": "Variable name"},
                "variable_type": {"type": "str", "required": True, "desc": "Variable type"},
                "is_exposed": {"type": "bool", "default": False, "desc": "Expose to editor"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "add_blueprint_get_self_component_reference": {
            "brief": "Add a self component reference node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "required": True, "desc": "Component name"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_blueprint_self_reference": {
            "brief": "Add a self reference node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "find_blueprint_nodes": {
            "brief": "Find nodes in a Blueprint graph (works on regular BP or Level Script BP)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "node_type": {"type": "any", "desc": "Filter by node type"},
                "event_type": {"type": "any", "desc": "Filter by event type"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
            },
        },
        "set_node_pin_value": {
            "brief": "Set a default value on a node pin. Handles primitive, struct, Class, SoftClass, Object, SoftObject and Interface pins (regular BP or LSB)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "node_id": {"type": "str", "required": True, "desc": "Node ID"},
                "pin_name": {"type": "str", "required": True, "desc": "Pin name"},
                "pin_value": {"type": "any", "required": True, "desc": "Pin value. For Class pins: class path like '/Script/VoxelRuntime.VoxelCollapseSubsystem' or bare class name. For Object pins: asset path. For primitives/struct: string representation. The handler dispatches to Pin->DefaultValue (primitives) or Pin->DefaultObject (Class/Object) automatically"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
            },
        },
        "add_variable_get_node": {
            "brief": "Add a variable getter node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "variable_name": {"type": "str", "required": True, "desc": "Variable name"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_variable_set_node": {
            "brief": "Add a variable setter node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "variable_name": {"type": "str", "required": True, "desc": "Variable name"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_branch_node": {
            "brief": "Add a branch (if/else) node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "delete_blueprint_node": {
            "brief": "Delete a Blueprint node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "node_id": {"type": "str", "required": True, "desc": "Node ID to delete"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "move_blueprint_node": {
            "brief": "Move a Blueprint node to a new position",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "node_id": {"type": "str", "required": True, "desc": "Node ID"},
                "position": {"type": "list[int]", "required": True, "desc": "New position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_sequence_node": {
            "brief": "Add a sequence node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "num_outputs": {"type": "int", "default": 2, "desc": "Number of output pins"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_delay_node": {
            "brief": "Add a delay node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "duration": {"type": "float", "default": 1.0, "desc": "Delay duration in seconds"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_foreach_loop_node": {
            "brief": "DEPRECATED - returns error. Use add_forloop_with_break_node instead",
            "params": {},
        },
        "add_forloop_with_break_node": {
            "brief": "Add a for-loop with break node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "first_index": {"type": "int", "default": 0, "desc": "First loop index"},
                "last_index": {"type": "int", "default": 10, "desc": "Last loop index"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_print_string_node": {
            "brief": "Add a print string node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "message": {"type": "str", "default": "Hello", "desc": "Message to print"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_math_node": {
            "brief": "Add a math operation node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "operation": {"type": "str", "required": True, "desc": "Add, Subtract, Multiply, Divide, etc."},
                "value_type": {"type": "str", "default": "Float", "desc": "Value type"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_comparison_node": {
            "brief": "Add a comparison node",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "operation": {"type": "str", "required": True, "desc": "Equal, NotEqual, Greater, Less, etc."},
                "value_type": {"type": "str", "default": "Float", "desc": "Value type"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_external_property_set_node": {
            "brief": "Add a VariableSet node for a UPROPERTY on another class (e.g. Subsystem field)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "target_class": {"type": "str", "required": True, "desc": "Owner class name, e.g. 'VoxelCollapseSubsystem' (U/A prefix handled automatically)"},
                "property_name": {"type": "str", "required": True, "desc": "UPROPERTY name. Must be BlueprintReadWrite (not BlueprintReadOnly)"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
            },
        },
        "add_external_property_get_node": {
            "brief": "Add a VariableGet node for a UPROPERTY on another class",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "target_class": {"type": "str", "required": True, "desc": "Owner class name, e.g. 'VoxelCollapseSubsystem'"},
                "property_name": {"type": "str", "required": True, "desc": "UPROPERTY name. Must be BlueprintVisible"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
            },
        },
        "add_get_subsystem_node": {
            "brief": "Add a typed UK2Node_GetSubsystem with the subsystem class baked in (ReturnValue is strongly typed, no Cast needed)",
            "params": {
                "blueprint_name": {"type": "str", "required": False, "desc": "Blueprint name (required unless target_type=level_blueprint)"},
                "subsystem_class": {"type": "str", "required": True, "desc": "Subsystem class, e.g. '/Script/VoxelRuntime.VoxelCollapseSubsystem' or 'VoxelCollapseSubsystem'"},
                "subsystem_kind": {"type": "str", "default": "GameInstance", "desc": "One of: 'GameInstance' (default), 'World', 'Engine', 'LocalPlayer'. Determines which K2 node variant is spawned and validates the class against the expected base"},
                "node_position": {"type": "list[int]", "default": [0, 0], "desc": "Node position [x,y]"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
                "target_type": {"type": "str", "default": "blueprint", "desc": "'blueprint' or 'level_blueprint'"},
                "level_path": {"type": "str", "desc": "Level asset path. Omit for current level. Only used when target_type=level_blueprint"},
            },
        },
    },

    # =========================================================================
    # UMG_WIDGET (18 commands)
    # =========================================================================
    "umg_widget": {
        "create_umg_widget_blueprint": {
            "brief": "Create a new UMG Widget Blueprint",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget blueprint name"},
                "parent_class": {
                    "type": "str",
                    "default": "UserWidget",
                    "desc": "Parent widget class. Accepts: bare name ('UserWidget', 'CommonUserWidget'), "
                            "C++ class path ('/Script/UMG.UserWidget'), or Blueprint class path "
                            "('/Game/UI/BP_MyBase.BP_MyBase_C'). Must descend from UUserWidget. "
                            "Errors: ClassNotFound (1211) if unresolvable, InvalidParamValue (1005) if not a UserWidget subclass.",
                },
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "bind_widget_event": {
            "brief": "Bind a widget event to a function",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "widget_component_name": {"type": "str", "required": True, "desc": "Widget component name"},
                "event_name": {"type": "str", "required": True, "desc": "Event name"},
                "function_name": {"type": "str", "default": "auto", "desc": "Function name (auto-generated if not specified)"},
            },
        },
        "add_widget_to_viewport": {
            "brief": "Add widget to viewport",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "z_order": {"type": "int", "default": 0, "desc": "Z-order for layering"},
            },
        },
        "set_text_block_binding": {
            "brief": "Set up dynamic property binding",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "text_block_name": {"type": "str", "required": True, "desc": "Text block name"},
                "binding_property": {"type": "str", "required": True, "desc": "Property to bind"},
                "binding_type": {"type": "str", "default": "Text", "desc": "Binding type"},
            },
        },
        "add_text_to_widget": {
            "brief": "Add a text element",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "text_name": {"type": "str", "required": True, "desc": "Text element name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "text": {"type": "str", "default": "+", "desc": "Text content"},
                "font_size": {"type": "int", "default": 32, "desc": "Font size"},
                "color": {"type": "list[float]", "default": [1, 1, 1, 1], "desc": "Text color [r,g,b,a]"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset (CanvasPanel only)"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Alignment [x,y] (CanvasPanel only)"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_image_to_widget": {
            "brief": "Add an image element",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "image_name": {"type": "str", "required": True, "desc": "Image element name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "texture_path": {"type": "str", "default": "", "desc": "Texture asset path"},
                "size": {"type": "list[float]", "default": [32, 32], "desc": "Size [w,h]"},
                "color_tint": {"type": "list[float]", "default": [1, 1, 1, 1], "desc": "Color tint [r,g,b,a]"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset (CanvasPanel only)"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Alignment [x,y] (CanvasPanel only)"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_progressbar_to_widget": {
            "brief": "Add a progress bar",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "progressbar_name": {"type": "str", "required": True, "desc": "Progress bar name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "percent": {"type": "float", "default": 0, "desc": "Fill percent (0-1)"},
                "fill_color": {"type": "list[float]", "default": [0, 0.5, 1, 1], "desc": "Fill color [r,g,b,a]"},
                "background_color": {"type": "list[float]", "default": [0.1, 0.1, 0.1, 1], "desc": "Background color [r,g,b,a]"},
                "size": {"type": "list[float]", "default": [200, 20], "desc": "Size [w,h]"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_border_to_widget": {
            "brief": "Add a Border widget (UBorder) — single-child container with background brush",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "border_name": {"type": "str", "required": True, "desc": "Border element name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "brush_color": {"type": "list[float]", "default": [1, 1, 1, 1], "desc": "Background brush color [r,g,b,a]"},
                "content_color_and_opacity": {"type": "list[float]", "default": [1, 1, 1, 1], "desc": "Content tint [r,g,b,a]"},
                "padding": {"type": "list[float]", "default": [0, 0, 0, 0], "desc": "Padding [left,top,right,bottom]"},
                "horizontal_alignment": {"type": "str", "default": "Fill", "desc": "Content H-alignment (Left/Center/Right/Fill)"},
                "vertical_alignment": {"type": "str", "default": "Fill", "desc": "Content V-alignment (Top/Center/Bottom/Fill)"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset (CanvasPanel only)"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Slot alignment [x,y] (CanvasPanel only)"},
                "position": {"type": "list[float]", "default": [0, 0], "desc": "Position [x,y] (CanvasPanel only)"},
                "size": {"type": "list[float]", "desc": "Size [w,h] (omit for auto-size on CanvasPanel)"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "get_widget_elements": {
            "brief": "Get all elements in a widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "set_widget_element_property": {
            "brief": "Set a property on a widget element (property_value accepts string / array / object / number / bool)",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "element_name": {"type": "str", "required": True, "desc": "Element name"},
                "property_name": {"type": "str", "required": True, "desc": "Property name (nested paths like 'Brush.TintColor' require string property_value)"},
                "property_value": {
                    "type": "any",
                    "required": True,
                    "desc": "Property value. Accepted forms: "
                            "(a) string — for special aliases (Visibility/Text/Justification/Percent) and ImportText_Direct fallback; "
                            "(b) list[float] — FLinearColor [r,g,b,a], FMargin [L,T,R,B], FVector [x,y,z], FVector2D [x,y]; "
                            "(c) dict — struct field assignment {\"R\":1, \"G\":0.5, ...}; "
                            "(d) number/bool — primitive UPROPERTY (int, float, bool). "
                            "ColorAndOpacity on TextBlock/Image accepts [r,g,b,a] arrays directly (v0.9.8 🆕)."
                },
                "parent_name": {"type": "str", "desc": "Optional: restrict element lookup to this panel's subtree (disambiguates when same-name widgets exist in multiple parents)"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_button_to_widget": {
            "brief": "Add a button with text",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "button_name": {"type": "str", "required": True, "desc": "Button name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "text": {"type": "str", "default": "", "desc": "Button text"},
                "font_size": {"type": "int", "default": 14, "desc": "Font size"},
                "text_color": {"type": "list[float]", "default": [1, 1, 1, 1], "desc": "Text color [r,g,b,a]"},
                "normal_color": {"type": "list[float]", "default": [0.1, 0.1, 0.1, 1], "desc": "Normal state color"},
                "hovered_color": {"type": "list[float]", "default": [0.2, 0.2, 0.2, 1], "desc": "Hovered state color"},
                "pressed_color": {"type": "list[float]", "default": [0.05, 0.05, 0.05, 1], "desc": "Pressed state color"},
                "size": {"type": "list[float]", "default": [200, 50], "desc": "Size [w,h]"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "bind_widget_component_event": {
            "brief": "Bind a component event",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "component_name": {"type": "str", "required": True, "desc": "Component name"},
                "event_type": {"type": "str", "required": True, "desc": "Event type"},
                "function_name": {"type": "str", "default": "auto", "desc": "Function name (auto-generated)"},
                "create_function": {"type": "bool", "default": True, "desc": "Create function if missing"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_slider_to_widget": {
            "brief": "Add a slider widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "slider_name": {"type": "str", "required": True, "desc": "Slider name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "value": {"type": "float", "default": 0, "desc": "Initial value"},
                "min_value": {"type": "float", "default": 0, "desc": "Minimum value"},
                "max_value": {"type": "float", "default": 1, "desc": "Maximum value"},
                "step_size": {"type": "float", "default": 0, "desc": "Step size (0 = continuous)"},
                "orientation": {"type": "str", "default": "Horizontal", "desc": "Orientation"},
                "bar_color": {"type": "list[float]", "desc": "Bar color [r,g,b,a]"},
                "handle_color": {"type": "list[float]", "desc": "Handle color [r,g,b,a]"},
                "size": {"type": "list[float]", "desc": "Size [w,h]"},
                "anchor": {"type": "str", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_checkbox_to_widget": {
            "brief": "Add a checkbox widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "checkbox_name": {"type": "str", "required": True, "desc": "Checkbox name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "is_checked": {"type": "bool", "default": False, "desc": "Initial checked state"},
                "label_text": {"type": "str", "default": "", "desc": "Label text"},
                "checked_color": {"type": "list[float]", "desc": "Checked state color"},
                "unchecked_color": {"type": "list[float]", "desc": "Unchecked state color"},
                "anchor": {"type": "str", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_combobox_to_widget": {
            "brief": "Add a combobox widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "combobox_name": {"type": "str", "required": True, "desc": "Combobox name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "options": {"type": "list[str]", "default": [], "desc": "Dropdown options"},
                "selected_index": {"type": "int", "default": 0, "desc": "Default selected index"},
                "font_size": {"type": "int", "default": 14, "desc": "Font size"},
                "size": {"type": "list[float]", "desc": "Size [w,h]"},
                "anchor": {"type": "str", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_editabletext_to_widget": {
            "brief": "Add an editable text field",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "text_name": {"type": "str", "required": True, "desc": "Text field name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "hint_text": {"type": "str", "default": "", "desc": "Placeholder hint text"},
                "is_password": {"type": "bool", "default": False, "desc": "Mask as password"},
                "is_multiline": {"type": "bool", "default": False, "desc": "Allow multiple lines"},
                "font_size": {"type": "int", "default": 14, "desc": "Font size"},
                "text_color": {"type": "list[float]", "desc": "Text color [r,g,b,a]"},
                "size": {"type": "list[float]", "desc": "Size [w,h]"},
                "anchor": {"type": "str", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_spinbox_to_widget": {
            "brief": "Add a spin box",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "spinbox_name": {"type": "str", "required": True, "desc": "Spin box name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "value": {"type": "float", "default": 0, "desc": "Initial value"},
                "min_value": {"type": "float", "default": 0, "desc": "Minimum value"},
                "max_value": {"type": "float", "default": 100, "desc": "Maximum value"},
                "delta": {"type": "float", "default": 1, "desc": "Step delta"},
                "size": {"type": "list[float]", "desc": "Size [w,h]"},
                "anchor": {"type": "str", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "remove_widget_element": {
            "brief": "Remove a widget element",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "element_name": {"type": "str", "required": True, "desc": "Element name to remove"},
                "parent_name": {"type": "str", "desc": "Optional: restrict lookup to this panel's subtree"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
    },

    # =========================================================================
    # UMG_LAYOUT (5 commands)
    # =========================================================================
    "umg_layout": {
        "add_vertical_box_to_widget": {
            "brief": "Add a vertical box container",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "box_name": {"type": "str", "required": True, "desc": "Box name"},
                "parent_name": {"type": "str", "desc": "Parent element name"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Alignment [x,y]"},
                "position": {"type": "list[float]", "default": [0, 0], "desc": "Position [x,y]"},
                "size": {"type": "list[float]", "desc": "Size [w,h]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_horizontal_box_to_widget": {
            "brief": "Add a horizontal box container",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "box_name": {"type": "str", "required": True, "desc": "Box name"},
                "parent_name": {"type": "str", "desc": "Parent element name"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Alignment [x,y]"},
                "position": {"type": "list[float]", "default": [0, 0], "desc": "Position [x,y]"},
                "size": {"type": "list[float]", "desc": "Size [w,h]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "reparent_widget_element": {
            "brief": "Move element to a different parent (v0.9.7: full canonical pattern + integrity checks prevent double-parent corruption)",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "element_name": {"type": "str", "required": True, "desc": "Element to move"},
                "new_parent_name": {"type": "str", "required": True, "desc": "New parent element name"},
                "parent_name": {"type": "str", "desc": "Optional: restrict source-element lookup to this panel's subtree (current parent)"},
                "slot_index": {"type": "int", "default": -1, "desc": "Slot index (-1 = append; other values currently accepted but append-only)"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_widget_switcher_to_widget": {
            "brief": "Add a Widget Switcher (UWidgetSwitcher) container",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "switcher_name": {"type": "str", "required": True, "desc": "Widget Switcher element name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "active_widget_index": {"type": "int", "default": 0, "desc": "Initial ActiveWidgetIndex"},
                "anchor": {"type": "str", "default": "Center", "desc": "Anchor preset (CanvasPanel only)"},
                "alignment": {"type": "list[float]", "default": [0.5, 0.5], "desc": "Slot alignment [x,y] (CanvasPanel only)"},
                "position": {"type": "list[float]", "default": [0, 0], "desc": "Position [x,y] (CanvasPanel only)"},
                "size": {"type": "list[float]", "desc": "Size [w,h] (omit for auto-size on CanvasPanel)"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "set_widget_slot_property": {
            "brief": "Set slot/layout properties. Dispatches by slot type: CanvasPanelSlot / VerticalBoxSlot / HorizontalBoxSlot / OverlaySlot / BorderSlot / WidgetSwitcherSlot",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "element_name": {"type": "str", "required": True, "desc": "Element name"},
                "parent_name": {"type": "str", "desc": "Optional: restrict element lookup to this panel's subtree"},
                # ── CanvasPanelSlot-specific ──
                "position": {"type": "list[float]", "desc": "[CanvasPanel] Position [x,y]"},
                "size": {"type": "list[float]", "desc": "[CanvasPanel] Size [w,h]"},
                "anchor": {"type": "str", "desc": "[CanvasPanel] Anchor preset (TopLeft/TopCenter/.../BottomRight). Takes precedence over anchor_min/max."},
                "anchor_min": {"type": "list[float]", "desc": "[CanvasPanel] Explicit anchor min [x,y] in 0-1 UV space"},
                "anchor_max": {"type": "list[float]", "desc": "[CanvasPanel] Explicit anchor max [x,y] in 0-1 UV space"},
                "offset_left": {"type": "float", "desc": "[CanvasPanel] Slot offset Left (FMargin.Left)"},
                "offset_top": {"type": "float", "desc": "[CanvasPanel] Slot offset Top"},
                "offset_right": {"type": "float", "desc": "[CanvasPanel] Slot offset Right"},
                "offset_bottom": {"type": "float", "desc": "[CanvasPanel] Slot offset Bottom"},
                "alignment": {"type": "list[float]", "desc": "[CanvasPanel] Alignment [x,y]"},
                "z_order": {"type": "int", "desc": "[CanvasPanel] Z-order"},
                "auto_size": {"type": "bool", "desc": "[CanvasPanel] Enable auto-sizing"},
                # ── VBox / HBox / Overlay / Border slot fields 🆕 v0.9.9 ──
                "padding": {"type": "list[float]", "desc": "[VBox/HBox/Overlay/Border] Padding [L,T,R,B] FMargin"},
                "horizontal_alignment": {"type": "str", "desc": "[VBox/HBox/Overlay/Border] Content H-alignment (Left/Center/Right/Fill)"},
                "vertical_alignment": {"type": "str", "desc": "[VBox/HBox/Overlay/Border] Content V-alignment (Top/Center/Bottom/Fill)"},
                # ── VBox / HBox specific ──
                "size_rule": {"type": "str", "desc": "[VBox/HBox] Size rule (Auto / Fill)"},
                "size_value": {"type": "float", "desc": "[VBox/HBox] Size value (Fill weight when size_rule=Fill)"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_scrollbox_to_widget": {
            "brief": "Add a scroll box",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "scrollbox_name": {"type": "str", "required": True, "desc": "Scroll box name"},
                "parent_name": {"type": "str", "desc": "Parent panel name (defaults to root canvas)"},
                "orientation": {"type": "str", "default": "Vertical", "desc": "Scroll orientation"},
                "scroll_bar_visibility": {"type": "str", "default": "Visible", "desc": "Scroll bar visibility"},
                "size": {"type": "list[float]", "desc": "Size [w,h]"},
                "anchor": {"type": "str", "desc": "Anchor preset"},
                "alignment": {"type": "list[float]", "desc": "Alignment [x,y]"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
    },

    # =========================================================================
    # UMG_VARIABLE (5 commands)
    # =========================================================================
    "umg_variable": {
        "add_widget_variable": {
            "brief": "Add a variable to a widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "variable_name": {"type": "str", "required": True, "desc": "Variable name"},
                "variable_type": {"type": "str", "required": True, "desc": "Variable type"},
                "default_value": {"type": "any", "desc": "Default value"},
                "is_exposed": {"type": "bool", "default": False, "desc": "Expose to editor"},
                "category": {"type": "str", "desc": "Variable category"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "set_widget_variable_default": {
            "brief": "Set variable default value",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "variable_name": {"type": "str", "required": True, "desc": "Variable name"},
                "default_value": {"type": "any", "required": True, "desc": "Default value"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_widget_array_variable": {
            "brief": "Add an array variable to a widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "variable_name": {"type": "str", "required": True, "desc": "Variable name"},
                "element_type": {"type": "str", "required": True, "desc": "Array element type"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_widget_function": {
            "brief": "Add a function to a widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "function_name": {"type": "str", "required": True, "desc": "Function name"},
                "inputs": {"type": "list", "desc": "Input parameters"},
                "outputs": {"type": "list", "desc": "Output parameters"},
                "is_pure": {"type": "bool", "default": False, "desc": "Pure function (no exec pins)"},
                "category": {"type": "str", "desc": "Function category"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_widget_event": {
            "brief": "Add a custom event to a widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "event_name": {"type": "str", "required": True, "desc": "Event name"},
                "inputs": {"type": "list", "desc": "Event input parameters"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
    },

    # =========================================================================
    # UMG_ANIMATION (4 commands)
    # =========================================================================
    "umg_animation": {
        "create_widget_animation": {
            "brief": "Create a widget animation",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "animation_name": {"type": "str", "required": True, "desc": "Animation name"},
                "length": {"type": "float", "default": 1.0, "desc": "Animation length in seconds"},
                "loop": {"type": "bool", "default": False, "desc": "Loop animation"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_animation_track": {
            "brief": "Add an animation track",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "animation_name": {"type": "str", "required": True, "desc": "Animation name"},
                "target_widget": {"type": "str", "required": True, "desc": "Target widget element"},
                "property_name": {"type": "str", "required": True, "desc": "Property to animate"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "add_animation_keyframe": {
            "brief": "Add a keyframe to animation",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "animation_name": {"type": "str", "required": True, "desc": "Animation name"},
                "target_widget": {"type": "str", "required": True, "desc": "Target widget element"},
                "property_name": {"type": "str", "required": True, "desc": "Property to animate"},
                "time": {"type": "float", "required": True, "desc": "Keyframe time in seconds"},
                "value": {"type": "any", "required": True, "desc": "Property value at this keyframe"},
                "interpolation": {"type": "str", "default": "Linear", "desc": "Interpolation mode"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
        "get_widget_animations": {
            "brief": "Get all animations in a widget",
            "params": {
                "widget_name": {"type": "str", "required": True, "desc": "Widget name"},
                "path": {"type": "str", "default": "/Game/UI", "desc": "Content path"},
            },
        },
    },

    # =========================================================================
    # PROJECT (13 commands)
    # =========================================================================
    "project": {
        "create_input_mapping": {
            "brief": "Create an input mapping",
            "params": {
                "action_name": {"type": "str", "required": True, "desc": "Action name"},
                "key": {"type": "str", "required": True, "desc": "Key binding"},
                "input_type": {"type": "str", "default": "Action", "desc": "Input type (Action/Axis)"},
            },
        },
        "create_input_action": {
            "brief": "Create an Enhanced Input Action",
            "params": {
                "action_name": {"type": "str", "required": True, "desc": "Action name"},
                "value_type": {"type": "str", "default": "Digital", "desc": "Value type (Digital/Axis1D/Axis2D/Axis3D)"},
                "path": {"type": "str", "default": "/Game/Input", "desc": "Content path"},
            },
        },
        "create_input_mapping_context": {
            "brief": "Create an Input Mapping Context",
            "params": {
                "context_name": {"type": "str", "required": True, "desc": "Context name"},
                "path": {"type": "str", "default": "/Game/Input", "desc": "Content path"},
            },
        },
        "add_action_to_mapping_context": {
            "brief": "Add action to mapping context",
            "params": {
                "context_name": {"type": "str", "required": True, "desc": "Mapping context name"},
                "action_name": {"type": "str", "required": True, "desc": "Input action name"},
                "key": {"type": "str", "required": True, "desc": "Key binding"},
                "trigger_type": {"type": "str", "default": "Down", "desc": "Trigger type"},
                "modifiers": {"type": "list", "default": [], "desc": "Input modifiers"},
                "context_path": {"type": "str", "default": "/Game/Input", "desc": "Context asset path"},
                "action_path": {"type": "str", "default": "/Game/Input", "desc": "Action asset path"},
            },
        },
        "delete_asset": {
            "brief": "Delete an asset",
            "params": {
                "asset_path": {"type": "str", "required": True, "desc": "Full asset path"},
            },
        },
        "add_mapping_context_to_blueprint": {
            "brief": "Add mapping context to blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "context_name": {"type": "str", "required": True, "desc": "Mapping context name"},
                "priority": {"type": "int", "default": 0, "desc": "Context priority"},
                "context_path": {"type": "str", "default": "/Game/Input", "desc": "Context asset path"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Blueprint content path"},
            },
        },
        "set_default_mapping_context": {
            "brief": "Set default mapping context on blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "context_name": {"type": "str", "required": True, "desc": "Mapping context name"},
                "priority": {"type": "int", "default": 0, "desc": "Context priority"},
                "context_path": {"type": "str", "default": "/Game/Input", "desc": "Context asset path"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Blueprint content path"},
            },
        },
        "asset_exists": {
            "brief": "Check if an asset exists",
            "params": {
                "asset_path": {"type": "str", "required": True, "desc": "Full asset path"},
            },
        },
        "create_content_folder": {
            "brief": "Create a content folder",
            "params": {
                "folder_path": {"type": "str", "required": True, "desc": "Folder path to create"},
            },
        },
        "list_assets_in_folder": {
            "brief": "List assets in a folder",
            "params": {
                "folder_path": {"type": "str", "required": True, "desc": "Folder path"},
                "class_filter": {"type": "str", "desc": "Filter by asset class"},
                "recursive": {"type": "bool", "default": False, "desc": "Search recursively"},
            },
        },
        "import_texture": {
            "brief": "Import a texture",
            "params": {
                "source": {"type": "str", "required": True, "desc": "Source file path or URL"},
                "asset_name": {"type": "str", "required": True, "desc": "Asset name"},
                "destination_path": {"type": "str", "default": "/Game/Textures", "desc": "Destination content path"},
                "source_type": {"type": "str", "default": "file", "desc": "Source type (file/url)"},
                "compression": {"type": "str", "default": "Default", "desc": "Compression setting"},
                "srgb": {"type": "bool", "default": True, "desc": "sRGB color space"},
                "lod_group": {"type": "str", "default": "World", "desc": "LOD group"},
            },
        },
        "get_project_info": {
            "brief": "Get project information",
            "params": {},
        },
        "find_asset_references": {
            "brief": "Find all references to an asset",
            "params": {
                "asset_path": {"type": "str", "required": True, "desc": "Asset path to search references for"},
            },
        },
    },

    # =========================================================================
    # AI (21 commands)
    # =========================================================================
    "ai": {
        "create_blackboard": {
            "brief": "Create a Blackboard asset",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Blackboard name"},
                "path": {"type": "str", "default": "/Game/AI/Blackboards", "desc": "Content path"},
            },
        },
        "add_blackboard_key": {
            "brief": "Add a key to a Blackboard",
            "params": {
                "blackboard_name": {"type": "str", "required": True, "desc": "Blackboard name"},
                "key_name": {"type": "str", "required": True, "desc": "Key name"},
                "key_type": {"type": "str", "required": True, "desc": "Key type (Bool, Int, Float, String, Name, Vector, Rotator, Object, Class, Enum)"},
                "path": {"type": "str", "default": "/Game/AI/Blackboards", "desc": "Content path"},
                "instance_synced": {"type": "bool", "default": False, "desc": "Instance synced"},
                "base_class": {"type": "str", "desc": "Base class for Object/Class key types"},
            },
        },
        "remove_blackboard_key": {
            "brief": "Remove a key from a Blackboard",
            "params": {
                "blackboard_name": {"type": "str", "required": True, "desc": "Blackboard name"},
                "key_name": {"type": "str", "required": True, "desc": "Key name to remove"},
                "path": {"type": "str", "default": "/Game/AI/Blackboards", "desc": "Content path"},
            },
        },
        "list_blackboard_keys": {
            "brief": "List all keys in a Blackboard",
            "params": {
                "blackboard_name": {"type": "str", "required": True, "desc": "Blackboard name"},
                "path": {"type": "str", "default": "/Game/AI/Blackboards", "desc": "Content path"},
            },
        },
        "create_behavior_tree": {
            "brief": "Create a Behavior Tree",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Behavior Tree name"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
                "blackboard_name": {"type": "str", "desc": "Associated Blackboard name"},
                "blackboard_path": {"type": "str", "default": "/Game/AI/Blackboards", "desc": "Blackboard content path"},
            },
        },
        "set_behavior_tree_blackboard": {
            "brief": "Set Blackboard for a BehaviorTree",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "blackboard_name": {"type": "str", "required": True, "desc": "Blackboard name"},
                "behavior_tree_path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "BT content path"},
                "blackboard_path": {"type": "str", "default": "/Game/AI/Blackboards", "desc": "BB content path"},
            },
        },
        "get_behavior_tree_structure": {
            "brief": "Get BehaviorTree structure",
            "params": {
                "name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
        "add_bt_composite_node": {
            "brief": "Add a composite node (Selector/Sequence/SimpleParallel)",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "node_type": {"type": "str", "required": True, "desc": "Composite type (Selector/Sequence/SimpleParallel)"},
                "parent_node_id": {"type": "str", "default": "", "desc": "Parent node ID (empty for root)"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
                "node_name": {"type": "str", "desc": "Custom node name"},
                "child_index": {"type": "int", "default": -1, "desc": "Child index (-1 = append)"},
                "node_position": {"type": "list[int]", "desc": "Node position [x,y]"},
            },
        },
        "add_bt_task_node": {
            "brief": "Add a task node",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "task_type": {"type": "str", "required": True, "desc": "Task type"},
                "parent_node_id": {"type": "str", "required": True, "desc": "Parent node ID"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
                "node_name": {"type": "str", "desc": "Custom node name"},
                "child_index": {"type": "int", "default": -1, "desc": "Child index (-1 = append)"},
                "node_position": {"type": "list[int]", "desc": "Node position [x,y]"},
            },
        },
        "add_bt_decorator_node": {
            "brief": "Add a decorator to a node",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "decorator_type": {"type": "str", "required": True, "desc": "Decorator type"},
                "target_node_id": {"type": "str", "required": True, "desc": "Target node ID to decorate"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
                "node_name": {"type": "str", "desc": "Custom node name"},
            },
        },
        "add_bt_service_node": {
            "brief": "Add a service to a node",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "service_type": {"type": "str", "required": True, "desc": "Service type"},
                "target_node_id": {"type": "str", "required": True, "desc": "Target node ID"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
                "node_name": {"type": "str", "desc": "Custom node name"},
            },
        },
        "connect_bt_nodes": {
            "brief": "Connect two BT nodes",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "parent_node_id": {"type": "str", "required": True, "desc": "Parent node ID"},
                "child_node_id": {"type": "str", "required": True, "desc": "Child node ID"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
                "child_index": {"type": "int", "default": -1, "desc": "Child index (-1 = append)"},
            },
        },
        "set_bt_node_property": {
            "brief": "Set a property on a BT node",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "node_id": {"type": "str", "required": True, "desc": "Node ID"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "property_value": {"type": "any", "required": True, "desc": "Property value"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
        "delete_bt_node": {
            "brief": "Delete a BT node",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "node_id": {"type": "str", "required": True, "desc": "Node ID to delete"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
        "list_bt_node_types": {
            "brief": "List available BT node types",
            "params": {
                "category": {"type": "str", "default": "all", "desc": "Filter by category (all/composite/task/decorator/service)"},
            },
        },
        "set_bt_node_position": {
            "brief": "Set position of a BT node",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "node_id": {"type": "str", "required": True, "desc": "Node ID"},
                "position": {"type": "list[int]", "required": True, "desc": "Position [x,y]"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
        "auto_layout_bt": {
            "brief": "Auto-layout BehaviorTree nodes",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
                "horizontal_spacing": {"type": "int", "default": 300, "desc": "Horizontal spacing between nodes"},
                "vertical_spacing": {"type": "int", "default": 150, "desc": "Vertical spacing between nodes"},
            },
        },
        "list_bt_nodes": {
            "brief": "List all nodes in a BehaviorTree",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
        "list_ai_assets": {
            "brief": "List AI assets (BTs, BBs, etc.)",
            "params": {
                "asset_type": {"type": "str", "default": "all", "desc": "Asset type filter (all/behavior_tree/blackboard)"},
                "path_filter": {"type": "str", "desc": "Filter by content path"},
            },
        },
        "detect_broken_bt_nodes": {
            "brief": "Detect broken/invalid BT nodes",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
        "fix_broken_bt_nodes": {
            "brief": "Fix broken BT nodes by deleting them",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
        "repair_broken_bt_nodes": {
            "brief": "Repair broken BT nodes by re-resolving class and recreating NodeInstance",
            "params": {
                "behavior_tree_name": {"type": "str", "required": True, "desc": "BehaviorTree name"},
                "path": {"type": "str", "default": "/Game/AI/BehaviorTrees", "desc": "Content path"},
            },
        },
    },

    # =========================================================================
    # PERCEPTION (6 commands)
    # =========================================================================
    "perception": {
        "add_ai_perception_component": {
            "brief": "Add AIPerception component to Blueprint",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "default": "AIPerceptionComponent", "desc": "Component name"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "configure_sight_sense": {
            "brief": "Configure sight sense",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "default": "AIPerceptionComponent", "desc": "Perception component name"},
                "sight_radius": {"type": "float", "default": 3000, "desc": "Sight radius"},
                "lose_sight_radius": {"type": "float", "default": 3500, "desc": "Lose sight radius"},
                "peripheral_vision_angle": {"type": "float", "default": 90, "desc": "Peripheral vision half-angle in degrees"},
                "detection_by_affiliation": {"type": "dict", "desc": "Detection by affiliation settings"},
                "auto_success_range": {"type": "float", "default": 500, "desc": "Auto success range within this distance"},
                "max_age": {"type": "float", "default": 5, "desc": "Max stimulus age in seconds"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "configure_hearing_sense": {
            "brief": "Configure hearing sense",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "default": "AIPerceptionComponent", "desc": "Perception component name"},
                "hearing_range": {"type": "float", "default": 3000, "desc": "Hearing range"},
                "detection_by_affiliation": {"type": "dict", "desc": "Detection by affiliation settings"},
                "max_age": {"type": "float", "default": 5, "desc": "Max stimulus age in seconds"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "configure_damage_sense": {
            "brief": "Configure damage sense",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "default": "AIPerceptionComponent", "desc": "Perception component name"},
                "max_age": {"type": "float", "default": 5, "desc": "Max stimulus age in seconds"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "set_perception_dominant_sense": {
            "brief": "Set dominant perception sense",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "sense_type": {"type": "str", "required": True, "desc": "Sense type (Sight/Hearing/Damage)"},
                "component_name": {"type": "str", "default": "AIPerceptionComponent", "desc": "Perception component name"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "add_perception_stimuli_source": {
            "brief": "Add perception stimuli source component",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "component_name": {"type": "str", "default": "AIPerceptionStimuliSourceComponent", "desc": "Component name"},
                "register_as_source_for": {"type": "list", "desc": "Sense types to register as source for"},
                "auto_register": {"type": "bool", "default": True, "desc": "Auto register with perception system"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
    },

    # =========================================================================
    # EQS (5 commands)
    # =========================================================================
    "eqs": {
        "create_eqs_query": {
            "brief": "Create an EQS query",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Query name"},
                "path": {"type": "str", "default": "/Game/AI/EQS", "desc": "Content path"},
            },
        },
        "add_eqs_generator": {
            "brief": "Add a generator to EQS query",
            "params": {
                "query_name": {"type": "str", "required": True, "desc": "Query name"},
                "generator_type": {"type": "str", "required": True, "desc": "Generator type"},
                "grid_size": {"type": "float", "default": 1000, "desc": "Grid size"},
                "space_between": {"type": "float", "default": 100, "desc": "Space between points"},
                "inner_radius": {"type": "float", "default": 300, "desc": "Inner radius"},
                "outer_radius": {"type": "float", "default": 1000, "desc": "Outer radius"},
                "circle_radius": {"type": "float", "default": 500, "desc": "Circle radius"},
                "number_of_points": {"type": "int", "default": 8, "desc": "Number of points"},
                "searched_actor_class": {"type": "str", "desc": "Actor class for ActorsOfClass generator"},
                "generate_around": {"type": "str", "default": "Querier", "desc": "Generate around context"},
                "path": {"type": "str", "default": "/Game/AI/EQS", "desc": "Content path"},
            },
        },
        "add_eqs_test": {
            "brief": "Add a test to EQS query",
            "params": {
                "query_name": {"type": "str", "required": True, "desc": "Query name"},
                "test_type": {"type": "str", "required": True, "desc": "Test type"},
                "generator_index": {"type": "int", "default": 0, "desc": "Target generator index"},
                "distance_to": {"type": "str", "default": "Querier", "desc": "Distance to context"},
                "trace_from": {"type": "str", "default": "Querier", "desc": "Trace from context"},
                "trace_to": {"type": "str", "default": "Item", "desc": "Trace to context"},
                "trace_channel": {"type": "str", "default": "Visibility", "desc": "Trace channel"},
                "test_purpose": {"type": "str", "default": "Score", "desc": "Test purpose (Score/Filter)"},
                "scoring_equation": {"type": "str", "default": "Linear", "desc": "Scoring equation"},
                "scoring_factor": {"type": "float", "default": 1.0, "desc": "Scoring factor"},
                "path": {"type": "str", "default": "/Game/AI/EQS", "desc": "Content path"},
            },
        },
        "set_eqs_test_property": {
            "brief": "Set property on an EQS test",
            "params": {
                "query_name": {"type": "str", "required": True, "desc": "Query name"},
                "generator_index": {"type": "int", "required": True, "desc": "Generator index"},
                "test_index": {"type": "int", "required": True, "desc": "Test index"},
                "property_name": {"type": "str", "required": True, "desc": "Property name"},
                "property_value": {"type": "any", "required": True, "desc": "Property value"},
                "path": {"type": "str", "default": "/Game/AI/EQS", "desc": "Content path"},
            },
        },
        "list_eqs_assets": {
            "brief": "List EQS assets",
            "params": {
                "path_filter": {"type": "str", "desc": "Filter by content path"},
            },
        },
    },

    # =========================================================================
    # GAS (8 commands)
    # =========================================================================
    "gas": {
        "add_gameplay_tags": {
            "brief": "Add gameplay tags",
            "params": {
                "tags": {"type": "list[str]", "required": True, "desc": "Tags to add"},
                "comments": {"type": "dict", "desc": "Comments for tags {tag: comment}"},
            },
        },
        "list_gameplay_tags": {
            "brief": "List gameplay tags",
            "params": {
                "filter_prefix": {"type": "str", "desc": "Filter by tag prefix"},
            },
        },
        "remove_gameplay_tag": {
            "brief": "Remove a gameplay tag",
            "params": {
                "tag": {"type": "str", "required": True, "desc": "Tag to remove"},
            },
        },
        "list_gas_assets": {
            "brief": "List GAS-related assets",
            "params": {
                "asset_type": {"type": "str", "default": "all", "desc": "Asset type filter"},
                "path_filter": {"type": "str", "desc": "Filter by content path"},
            },
        },
        "create_gameplay_effect": {
            "brief": "Create a Gameplay Effect",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Effect name"},
                "duration_policy": {"type": "str", "default": "Instant", "desc": "Duration policy (Instant/HasDuration/Infinite)"},
                "duration_magnitude": {"type": "float", "desc": "Duration magnitude"},
                "period": {"type": "float", "desc": "Period for periodic effects"},
                "modifiers": {"type": "list", "desc": "Attribute modifiers"},
                "application_tags": {"type": "list", "desc": "Tags applied while effect is active"},
                "removal_tags": {"type": "list", "desc": "Tags that remove this effect"},
                "path": {"type": "str", "default": "/Game/GAS/Effects", "desc": "Content path"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "create_gas_character": {
            "brief": "Create a GAS-enabled character",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Character name"},
                "parent_class": {"type": "str", "default": "Character", "desc": "Parent class"},
                "asc_component_name": {"type": "str", "default": "AbilitySystemComponent", "desc": "ASC component name"},
                "replication_mode": {"type": "str", "default": "Mixed", "desc": "Replication mode"},
                "default_abilities": {"type": "list", "desc": "Default abilities to grant"},
                "default_effects": {"type": "list", "desc": "Default effects to apply"},
                "path": {"type": "str", "default": "/Game/GAS/Characters", "desc": "Content path"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
        "set_ability_system_defaults": {
            "brief": "Set ASC defaults",
            "params": {
                "blueprint_name": {"type": "str", "required": True, "desc": "Blueprint name"},
                "asc_component_name": {"type": "str", "default": "AbilitySystemComponent", "desc": "ASC component name"},
                "default_abilities": {"type": "list", "desc": "Default abilities"},
                "default_effects": {"type": "list", "desc": "Default effects"},
                "path": {"type": "str", "default": "/Game/Blueprints", "desc": "Content path"},
            },
        },
        "create_gameplay_ability": {
            "brief": "Create a Gameplay Ability",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Ability name"},
                "parent_class": {"type": "str", "default": "GameplayAbility", "desc": "Parent class"},
                "ability_tags": {"type": "list", "desc": "Tags identifying this ability"},
                "cancel_abilities_with_tags": {"type": "list", "desc": "Tags of abilities to cancel"},
                "block_abilities_with_tags": {"type": "list", "desc": "Tags of abilities to block"},
                "activation_owned_tags": {"type": "list", "desc": "Tags added during activation"},
                "activation_required_tags": {"type": "list", "desc": "Tags required for activation"},
                "activation_blocked_tags": {"type": "list", "desc": "Tags that block activation"},
                "cost_effect": {"type": "str", "desc": "Cost gameplay effect"},
                "cooldown_effect": {"type": "str", "desc": "Cooldown gameplay effect"},
                "instancing_policy": {"type": "str", "default": "InstancedPerActor", "desc": "Instancing policy"},
                "net_execution_policy": {"type": "str", "default": "LocalPredicted", "desc": "Net execution policy"},
                "path": {"type": "str", "default": "/Game/GAS/Abilities", "desc": "Content path"},
                "rationale": {"type": "str", "desc": "Design rationale"},
            },
        },
    },

    # =========================================================================
    # MATERIAL (6 commands)
    # =========================================================================
    "material": {
        "list_material_templates": {
            "brief": "List available material templates",
            "params": {
                "source": {"type": "str", "default": "all", "desc": "Source filter (all/builtin/user)"},
            },
        },
        "get_material_template": {
            "brief": "Get a material template by name",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Template name"},
            },
        },
        "save_material_template": {
            "brief": "Save a user-defined material template",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Template name"},
                "description": {"type": "str", "default": "", "desc": "Template description"},
                "shading_model": {"type": "str", "default": "DefaultLit", "desc": "Shading model"},
                "blend_mode": {"type": "str", "default": "Opaque", "desc": "Blend mode"},
                "two_sided": {"type": "bool", "default": False, "desc": "Two-sided rendering"},
                "base_color": {"type": "list[float]", "desc": "Base color [r,g,b]"},
                "opacity": {"type": "float", "default": 1.0, "desc": "Opacity"},
                "emissive_color": {"type": "list[float]", "desc": "Emissive color [r,g,b]"},
                "emissive_strength": {"type": "float", "default": 1.0, "desc": "Emissive strength"},
                "tags": {"type": "str", "default": "", "desc": "Comma-separated tags"},
            },
        },
        "delete_material_template": {
            "brief": "Delete a user material template",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Template name to delete"},
            },
        },
        "create_material_from_template": {
            "brief": "Create material from template",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Material name"},
                "template": {"type": "str", "required": True, "desc": "Template name"},
                "path": {"type": "str", "default": "/Game/Materials", "desc": "Content path"},
                "color": {"type": "list[float]", "desc": "Override base color"},
                "opacity": {"type": "float", "desc": "Override opacity"},
                "two_sided": {"type": "bool", "desc": "Override two-sided setting"},
            },
        },
        "create_simple_material": {
            "brief": "Create a material with settings",
            "params": {
                "name": {"type": "str", "required": True, "desc": "Material name"},
                "path": {"type": "str", "default": "/Game/Materials", "desc": "Content path"},
                "shading_model": {"type": "str", "default": "DefaultLit", "desc": "Shading model"},
                "blend_mode": {"type": "str", "default": "Opaque", "desc": "Blend mode"},
                "two_sided": {"type": "bool", "default": False, "desc": "Two-sided rendering"},
                "base_color": {"type": "list[float]", "desc": "Base color [r,g,b]"},
                "opacity": {"type": "float", "default": 1.0, "desc": "Opacity"},
                "emissive_color": {"type": "list[float]", "desc": "Emissive color [r,g,b]"},
                "emissive_strength": {"type": "float", "default": 1.0, "desc": "Emissive strength"},
            },
        },
    },

    # =========================================================================
    # CONFIG (3 commands)
    # =========================================================================
    "config": {
        "get_config_value": {
            "brief": "Get a config value",
            "params": {
                "section": {"type": "str", "required": True, "desc": "Config section name"},
                "key": {"type": "str", "required": True, "desc": "Config key"},
                "config_file": {"type": "str", "default": "DefaultEngine", "desc": "Config file name"},
            },
        },
        "set_config_value": {
            "brief": "Set a config value",
            "params": {
                "section": {"type": "str", "required": True, "desc": "Config section name"},
                "key": {"type": "str", "required": True, "desc": "Config key"},
                "value": {"type": "str", "required": True, "desc": "Config value"},
                "config_file": {"type": "str", "default": "DefaultEngine", "desc": "Config file name"},
            },
        },
        "list_config_sections": {
            "brief": "List config file sections",
            "params": {
                "config_file": {"type": "str", "default": "DefaultEngine", "desc": "Config file name"},
            },
        },
    },
}


def get_all_commands() -> dict[str, list[str]]:
    """Return a dict of category -> list of command names."""
    return {
        category: list(commands.keys())
        for category, commands in COMMAND_SCHEMAS.items()
    }


def get_command_count() -> dict[str, int]:
    """Return a dict of category -> command count."""
    return {
        category: len(commands)
        for category, commands in COMMAND_SCHEMAS.items()
    }


def get_total_command_count() -> int:
    """Return the total number of commands across all categories."""
    return sum(len(commands) for commands in COMMAND_SCHEMAS.values())
