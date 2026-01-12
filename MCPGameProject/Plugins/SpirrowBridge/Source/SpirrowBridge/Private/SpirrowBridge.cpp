#include "SpirrowBridge.h"
#include "MCPServerRunnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Containers/Ticker.h"  // For FTSTicker (import operations that bypass TaskGraph)
// Add Blueprint related includes
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
// UE5.5 correct includes
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
// Blueprint Graph specific includes
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "GameFramework/InputSettings.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
// Include our new command handler classes
#include "Commands/SpirrowBridgeEditorCommands.h"
#include "Commands/SpirrowBridgeBlueprintCommands.h"
#include "Commands/SpirrowBridgeBlueprintNodeCommands.h"
#include "Commands/SpirrowBridgeProjectCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Commands/SpirrowBridgeUMGWidgetCommands.h"
#include "Commands/SpirrowBridgeUMGLayoutCommands.h"
#include "Commands/SpirrowBridgeUMGAnimationCommands.h"
#include "Commands/SpirrowBridgeUMGVariableCommands.h"
#include "Commands/SpirrowBridgeConfigCommands.h"
#include "Commands/SpirrowBridgeGASCommands.h"
#include "Commands/SpirrowBridgeMaterialCommands.h"
#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeAIPerceptionCommands.h"
#include "Commands/SpirrowBridgeEQSCommands.h"

// Default settings
#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557

USpirrowBridge::USpirrowBridge()
{
    EditorCommands = MakeShared<FSpirrowBridgeEditorCommands>();
    BlueprintCommands = MakeShared<FSpirrowBridgeBlueprintCommands>();
    BlueprintNodeCommands = MakeShared<FSpirrowBridgeBlueprintNodeCommands>();
    ProjectCommands = MakeShared<FSpirrowBridgeProjectCommands>();
    UMGWidgetCommands = MakeShared<FSpirrowBridgeUMGWidgetCommands>();
    UMGLayoutCommands = MakeShared<FSpirrowBridgeUMGLayoutCommands>();
    UMGAnimationCommands = MakeShared<FSpirrowBridgeUMGAnimationCommands>();
    UMGVariableCommands = MakeShared<FSpirrowBridgeUMGVariableCommands>();
    ConfigCommands = MakeShared<FSpirrowBridgeConfigCommands>();
    GASCommands = MakeShared<FSpirrowBridgeGASCommands>();
    MaterialCommands = MakeShared<FSpirrowBridgeMaterialCommands>();
    AICommands = MakeShared<FSpirrowBridgeAICommands>();
    AIPerceptionCommands = MakeShared<FSpirrowBridgeAIPerceptionCommands>();
    EQSCommands = MakeShared<FSpirrowBridgeEQSCommands>();
}

USpirrowBridge::~USpirrowBridge()
{
    EditorCommands.Reset();
    BlueprintCommands.Reset();
    BlueprintNodeCommands.Reset();
    ProjectCommands.Reset();
    UMGWidgetCommands.Reset();
    UMGLayoutCommands.Reset();
    UMGAnimationCommands.Reset();
    UMGVariableCommands.Reset();
    ConfigCommands.Reset();
    GASCommands.Reset();
    MaterialCommands.Reset();
    AICommands.Reset();
    AIPerceptionCommands.Reset();
    EQSCommands.Reset();
}

// Initialize subsystem
void USpirrowBridge::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Initializing"));
    
    bIsRunning = false;
    ListenerSocket = nullptr;
    ConnectionSocket = nullptr;
    ServerThread = nullptr;
    Port = MCP_SERVER_PORT;
    FIPv4Address::Parse(MCP_SERVER_HOST, ServerAddress);

    // Start the server automatically
    StartServer();
}

// Clean up resources when subsystem is destroyed
void USpirrowBridge::Deinitialize()
{
    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Shutting down"));
    StopServer();
}

// Start the MCP server
void USpirrowBridge::StartServer()
{
    if (bIsRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpirrowBridge: Server is already running"));
        return;
    }

    // Create socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("SpirrowBridge: Failed to get socket subsystem"));
        return;
    }

    // Create listener socket
    TSharedPtr<FSocket> NewListenerSocket = MakeShareable(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UnrealMCPListener"), false));
    if (!NewListenerSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("SpirrowBridge: Failed to create listener socket"));
        return;
    }

    // Allow address reuse for quick restarts
    NewListenerSocket->SetReuseAddr(true);
    NewListenerSocket->SetNonBlocking(true);

    // Bind to address
    FIPv4Endpoint Endpoint(ServerAddress, Port);
    if (!NewListenerSocket->Bind(*Endpoint.ToInternetAddr()))
    {
        UE_LOG(LogTemp, Error, TEXT("SpirrowBridge: Failed to bind listener socket to %s:%d"), *ServerAddress.ToString(), Port);
        return;
    }

    // Start listening
    if (!NewListenerSocket->Listen(5))
    {
        UE_LOG(LogTemp, Error, TEXT("SpirrowBridge: Failed to start listening"));
        return;
    }

    ListenerSocket = NewListenerSocket;
    bIsRunning = true;
    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Server started on %s:%d"), *ServerAddress.ToString(), Port);

    // Start server thread
    ServerThread = FRunnableThread::Create(
        new FMCPServerRunnable(this, ListenerSocket),
        TEXT("UnrealMCPServerThread"),
        0, TPri_Normal
    );

    if (!ServerThread)
    {
        UE_LOG(LogTemp, Error, TEXT("SpirrowBridge: Failed to create server thread"));
        StopServer();
        return;
    }
}

// Stop the MCP server
void USpirrowBridge::StopServer()
{
    if (!bIsRunning)
    {
        return;
    }

    bIsRunning = false;

    // Clean up thread
    if (ServerThread)
    {
        ServerThread->Kill(true);
        delete ServerThread;
        ServerThread = nullptr;
    }

    // Close sockets
    if (ConnectionSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket.Get());
        ConnectionSocket.Reset();
    }

    if (ListenerSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket.Get());
        ListenerSocket.Reset();
    }

    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Server stopped"));
}

// Execute a command received from a client
FString USpirrowBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Executing command: %s"), *CommandType);

    // Import operations use InterchangeEngine internally which also uses TaskGraph.
    // Using AsyncTask(GameThread) + InterchangeEngine causes TaskGraph RecursionGuard assertion failure.
    // So we use FTSTicker for import operations instead, which runs on GameThread tick without TaskGraph.
    bool bIsImportOperation = (CommandType == TEXT("import_texture"));

    if (bIsImportOperation)
    {
        // Use TSharedPtr for Promise since FTickerDelegate needs copyable lambdas
        TSharedPtr<TPromise<FString>> PromisePtr = MakeShared<TPromise<FString>>();
        TFuture<FString> Future = PromisePtr->GetFuture();

        // Use FTSTicker for import operations to avoid TaskGraph recursion
        // FTSTicker runs on the engine tick, outside of TaskGraph context
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [this, CommandType, Params, PromisePtr](float DeltaTime) -> bool
            {
                UE_LOG(LogTemp, Display, TEXT("SpirrowBridge: Executing import via FTSTicker: %s"), *CommandType);

                TSharedPtr<FJsonObject> ResponseJson = MakeShareable(new FJsonObject);

                try
                {
                    TSharedPtr<FJsonObject> ResultJson = ProjectCommands->HandleCommand(CommandType, Params);

                    bool bSuccess = true;
                    FString ErrorMessage = TEXT("Unknown error");

                    if (ResultJson->HasField(TEXT("success")))
                    {
                        bSuccess = ResultJson->GetBoolField(TEXT("success"));
                        if (!bSuccess && ResultJson->HasField(TEXT("error")))
                        {
                            ErrorMessage = ResultJson->GetStringField(TEXT("error"));
                        }
                    }

                    if (bSuccess)
                    {
                        ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
                        ResponseJson->SetObjectField(TEXT("result"), ResultJson);
                    }
                    else
                    {
                        ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                        ResponseJson->SetStringField(TEXT("error"), ErrorMessage);
                    }
                }
                catch (const std::exception& e)
                {
                    ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                    ResponseJson->SetStringField(TEXT("error"), UTF8_TO_TCHAR(e.what()));
                }

                FString ResultString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
                FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
                PromisePtr->SetValue(ResultString);

                return false; // Don't continue ticking - one-shot execution
            }
        ));

        return Future.Get();
    }

    // Create a promise to wait for the result (for non-import operations)
    TPromise<FString> Promise;
    TFuture<FString> Future = Promise.GetFuture();

    // Queue execution on Game Thread (for non-import operations)
    AsyncTask(ENamedThreads::GameThread, [this, CommandType, Params, Promise = MoveTemp(Promise)]() mutable
    {
        TSharedPtr<FJsonObject> ResponseJson = MakeShareable(new FJsonObject);
        
        try
        {
            TSharedPtr<FJsonObject> ResultJson;
            
            if (CommandType == TEXT("ping"))
            {
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetStringField(TEXT("message"), TEXT("pong"));
            }
            // Editor Commands (including actor manipulation)
            else if (CommandType == TEXT("get_actors_in_level") ||
                     CommandType == TEXT("find_actors_by_name") ||
                     CommandType == TEXT("spawn_actor") ||
                     CommandType == TEXT("create_actor") ||
                     CommandType == TEXT("delete_actor") ||
                     CommandType == TEXT("set_actor_transform") ||
                     CommandType == TEXT("get_actor_properties") ||
                     CommandType == TEXT("set_actor_property") ||
                     CommandType == TEXT("get_actor_components") ||
                     CommandType == TEXT("rename_actor") ||
                     CommandType == TEXT("rename_asset") ||
                     CommandType == TEXT("spawn_blueprint_actor") ||
                     CommandType == TEXT("focus_viewport") ||
                     CommandType == TEXT("take_screenshot"))
            {
                ResultJson = EditorCommands->HandleCommand(CommandType, Params);
            }
            // Blueprint Commands
            else if (CommandType == TEXT("create_blueprint") ||
                     CommandType == TEXT("add_component_to_blueprint") ||
                     CommandType == TEXT("set_component_property") ||
                     CommandType == TEXT("set_physics_properties") ||
                     CommandType == TEXT("compile_blueprint") ||
                     CommandType == TEXT("set_blueprint_property") ||
                     CommandType == TEXT("set_static_mesh_properties") ||
                     CommandType == TEXT("set_pawn_properties") ||
                     CommandType == TEXT("scan_project_classes") ||
                     CommandType == TEXT("duplicate_blueprint") ||
                     CommandType == TEXT("get_blueprint_graph") ||
                     CommandType == TEXT("set_blueprint_class_array") ||
                     CommandType == TEXT("set_struct_array_property") ||
                     // New property commands (v0.8.8)
                     CommandType == TEXT("create_data_asset") ||
                     CommandType == TEXT("set_class_property") ||
                     CommandType == TEXT("set_object_property") ||
                     CommandType == TEXT("get_blueprint_properties") ||
                     CommandType == TEXT("set_struct_property") ||
                     CommandType == TEXT("set_data_asset_property") ||
                     // Batch operations (v0.8.9)
                     CommandType == TEXT("batch_set_properties"))
            {
                ResultJson = BlueprintCommands->HandleCommand(CommandType, Params);
            }
            // Blueprint Node Commands
            else if (CommandType == TEXT("connect_blueprint_nodes") || 
                     CommandType == TEXT("add_blueprint_get_self_component_reference") ||
                     CommandType == TEXT("add_blueprint_self_reference") ||
                     CommandType == TEXT("find_blueprint_nodes") ||
                     CommandType == TEXT("add_blueprint_event_node") ||
                     CommandType == TEXT("add_blueprint_input_action_node") ||
                     CommandType == TEXT("add_blueprint_function_node") ||
                     CommandType == TEXT("add_blueprint_get_component_node") ||
                     CommandType == TEXT("add_blueprint_variable") ||
                     // New node manipulation commands
                     CommandType == TEXT("set_node_pin_value") ||
                     CommandType == TEXT("add_variable_get_node") ||
                     CommandType == TEXT("add_variable_set_node") ||
                     CommandType == TEXT("add_branch_node") ||
                     CommandType == TEXT("delete_node") ||
                     CommandType == TEXT("move_node") ||
                     // Control flow nodes
                     CommandType == TEXT("add_sequence_node") ||
                     CommandType == TEXT("add_delay_node") ||
                     CommandType == TEXT("add_foreach_loop_node") ||
                     CommandType == TEXT("add_forloop_with_break_node") ||
                     // Debug & utility nodes
                     CommandType == TEXT("add_print_string_node") ||
                     // Math & comparison nodes
                     CommandType == TEXT("add_math_node") ||
                     CommandType == TEXT("add_comparison_node"))
            {
                ResultJson = BlueprintNodeCommands->HandleCommand(CommandType, Params);
            }
            // Project Commands
            else if (CommandType == TEXT("create_input_mapping") ||
                     CommandType == TEXT("create_input_action") ||
                     CommandType == TEXT("create_input_mapping_context") ||
                     CommandType == TEXT("add_action_to_mapping_context") ||
                     CommandType == TEXT("get_input_mapping_context") ||
                     CommandType == TEXT("get_input_action") ||
                     CommandType == TEXT("remove_action_from_mapping_context") ||
                     CommandType == TEXT("delete_asset") ||
                     CommandType == TEXT("add_mapping_context_to_blueprint") ||
                     CommandType == TEXT("set_default_mapping_context") ||
                     // Asset utility commands
                     CommandType == TEXT("asset_exists") ||
                     CommandType == TEXT("create_content_folder") ||
                     CommandType == TEXT("list_assets_in_folder") ||
                     CommandType == TEXT("import_texture") ||
                     CommandType == TEXT("get_project_info") ||
                     CommandType == TEXT("find_asset_references"))
            {
                ResultJson = ProjectCommands->HandleCommand(CommandType, Params);
            }
            // UMG Widget Commands
            else if (CommandType == TEXT("create_umg_widget_blueprint") ||
                     CommandType == TEXT("add_text_to_widget") ||
                     CommandType == TEXT("add_text_block_to_widget") ||
                     CommandType == TEXT("add_image_to_widget") ||
                     CommandType == TEXT("add_progressbar_to_widget") ||
                     CommandType == TEXT("add_button_to_widget") ||
                     CommandType == TEXT("add_slider_to_widget") ||
                     CommandType == TEXT("add_checkbox_to_widget") ||
                     CommandType == TEXT("add_combobox_to_widget") ||
                     CommandType == TEXT("add_editabletext_to_widget") ||
                     CommandType == TEXT("add_spinbox_to_widget") ||
                     CommandType == TEXT("add_scrollbox_to_widget") ||
                     CommandType == TEXT("add_widget_to_viewport"))
            {
                ResultJson = UMGWidgetCommands->HandleCommand(CommandType, Params);
            }
            // UMG Layout Commands
            else if (CommandType == TEXT("add_vertical_box_to_widget") ||
                     CommandType == TEXT("add_horizontal_box_to_widget") ||
                     CommandType == TEXT("get_widget_elements") ||
                     CommandType == TEXT("set_widget_slot_property") ||
                     CommandType == TEXT("set_widget_element_property") ||
                     CommandType == TEXT("reparent_widget_element") ||
                     CommandType == TEXT("remove_widget_element"))
            {
                ResultJson = UMGLayoutCommands->HandleCommand(CommandType, Params);
            }
            // UMG Animation Commands
            else if (CommandType == TEXT("create_widget_animation") ||
                     CommandType == TEXT("add_animation_track") ||
                     CommandType == TEXT("add_animation_keyframe") ||
                     CommandType == TEXT("get_widget_animations"))
            {
                ResultJson = UMGAnimationCommands->HandleCommand(CommandType, Params);
            }
            // UMG Variable Commands
            else if (CommandType == TEXT("add_widget_variable") ||
                     CommandType == TEXT("add_widget_array_variable") ||
                     CommandType == TEXT("set_widget_variable_default") ||
                     CommandType == TEXT("add_widget_function") ||
                     CommandType == TEXT("add_widget_event") ||
                     CommandType == TEXT("bind_widget_to_variable") ||
                     CommandType == TEXT("bind_widget_event") ||
                     CommandType == TEXT("set_text_block_binding") ||
                     CommandType == TEXT("bind_widget_component_event"))
            {
                ResultJson = UMGVariableCommands->HandleCommand(CommandType, Params);
            }
            // Config Commands
            else if (CommandType == TEXT("get_config_value") ||
                     CommandType == TEXT("set_config_value") ||
                     CommandType == TEXT("list_config_sections"))
            {
                ResultJson = ConfigCommands->HandleCommand(CommandType, Params);
            }
            // GAS Commands
            else if (CommandType == TEXT("add_gameplay_tags") ||
                     CommandType == TEXT("list_gameplay_tags") ||
                     CommandType == TEXT("remove_gameplay_tag") ||
                     CommandType == TEXT("list_gas_assets") ||
                     CommandType == TEXT("create_gameplay_effect") ||
                     CommandType == TEXT("create_gas_character") ||
                     CommandType == TEXT("set_ability_system_defaults") ||
                     CommandType == TEXT("create_gameplay_ability"))
            {
                ResultJson = GASCommands->HandleCommand(CommandType, Params);
            }
            // Material Commands
            else if (CommandType == TEXT("create_simple_material"))
            {
                ResultJson = MaterialCommands->HandleCommand(CommandType, Params);
            }
            // AI Commands
            else if (CommandType == TEXT("create_blackboard") ||
                     CommandType == TEXT("add_blackboard_key") ||
                     CommandType == TEXT("remove_blackboard_key") ||
                     CommandType == TEXT("list_blackboard_keys") ||
                     CommandType == TEXT("create_behavior_tree") ||
                     CommandType == TEXT("set_behavior_tree_blackboard") ||
                     CommandType == TEXT("get_behavior_tree_structure") ||
                     CommandType == TEXT("list_ai_assets") ||
                     // Phase G: BT Node Operations
                     CommandType == TEXT("add_bt_composite_node") ||
                     CommandType == TEXT("add_bt_task_node") ||
                     CommandType == TEXT("add_bt_decorator_node") ||
                     CommandType == TEXT("add_bt_service_node") ||
                     CommandType == TEXT("connect_bt_nodes") ||
                     CommandType == TEXT("set_bt_node_property") ||
                     CommandType == TEXT("delete_bt_node") ||
                     CommandType == TEXT("list_bt_node_types") ||
                     // BT Node Position Commands
                     CommandType == TEXT("set_bt_node_position") ||
                     CommandType == TEXT("auto_layout_bt") ||
                     CommandType == TEXT("list_bt_nodes"))
            {
                ResultJson = AICommands->HandleCommand(CommandType, Params);
            }
            // AI Perception Commands (Phase H-1)
            else if (CommandType == TEXT("add_ai_perception_component") ||
                     CommandType == TEXT("configure_sight_sense") ||
                     CommandType == TEXT("configure_hearing_sense") ||
                     CommandType == TEXT("configure_damage_sense") ||
                     CommandType == TEXT("set_perception_dominant_sense") ||
                     CommandType == TEXT("add_perception_stimuli_source"))
            {
                ResultJson = AIPerceptionCommands->HandleCommand(CommandType, Params);
            }
            // EQS Commands (Phase H-2)
            else if (CommandType == TEXT("create_eqs_query") ||
                     CommandType == TEXT("add_eqs_generator") ||
                     CommandType == TEXT("add_eqs_test") ||
                     CommandType == TEXT("set_eqs_test_property") ||
                     CommandType == TEXT("list_eqs_assets"))
            {
                ResultJson = EQSCommands->HandleCommand(CommandType, Params);
            }
            else
            {
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown command: %s"), *CommandType));
                
                FString ResultString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
                FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
                Promise.SetValue(ResultString);
                return;
            }
            
            // Check if the result contains an error
            bool bSuccess = true;
            FString ErrorMessage;
            
            if (ResultJson->HasField(TEXT("success")))
            {
                bSuccess = ResultJson->GetBoolField(TEXT("success"));
                if (!bSuccess && ResultJson->HasField(TEXT("error")))
                {
                    ErrorMessage = ResultJson->GetStringField(TEXT("error"));
                }
            }
            
            if (bSuccess)
            {
                // Set success status and include the result
                ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
                ResponseJson->SetObjectField(TEXT("result"), ResultJson);
            }
            else
            {
                // Set error status and include the error message
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), ErrorMessage);
            }
        }
        catch (const std::exception& e)
        {
            ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
            ResponseJson->SetStringField(TEXT("error"), UTF8_TO_TCHAR(e.what()));
        }
        
        FString ResultString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
        FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
        Promise.SetValue(ResultString);
    });
    
    return Future.Get();
}