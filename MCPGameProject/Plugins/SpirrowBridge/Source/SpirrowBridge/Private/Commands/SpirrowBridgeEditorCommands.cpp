#include "Commands/SpirrowBridgeEditorCommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ILiveCodingModule.h"  // v0.10.0 - trigger_live_coding
#include "Modules/ModuleManager.h"
#include "ShowFlags.h"           // v0.10.0 - set_showflag
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EditorAssetLibrary.h"
// Volume actors support
#include "NavMesh/NavMeshBoundsVolume.h"
#include "GameFramework/KillZVolume.h"
#include "Engine/TriggerVolume.h"
#include "Engine/BlockingVolume.h"
#include "GameFramework/PhysicsVolume.h"
#include "Engine/PostProcessVolume.h"
#include "Sound/AudioVolume.h"
#include "Lightmass/LightmassImportanceVolume.h"
#include "GameFramework/Volume.h"
// For creating brush geometry
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"

FSpirrowBridgeEditorCommands::FSpirrowBridgeEditorCommands()
{
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    else if (CommandType == TEXT("get_actor_components"))
    {
        return HandleGetActorComponents(Params);
    }
    else if (CommandType == TEXT("rename_actor"))
    {
        return HandleRenameActor(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    // Editor viewport camera + show flag + live coding (v0.10.0)
    else if (CommandType == TEXT("get_editor_camera"))
    {
        return HandleGetEditorCamera(Params);
    }
    else if (CommandType == TEXT("set_editor_camera"))
    {
        return HandleSetEditorCamera(Params);
    }
    else if (CommandType == TEXT("set_showflag"))
    {
        return HandleSetShowFlag(Params);
    }
    else if (CommandType == TEXT("trigger_live_coding"))
    {
        return HandleTriggerLiveCoding(Params);
    }
    // Asset management commands
    else if (CommandType == TEXT("rename_asset"))
    {
        return HandleRenameAsset(Params);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FSpirrowBridgeCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString Pattern;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("pattern"), Pattern))
    {
        return Error;
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
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("pattern"), Pattern);
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    ResultObj->SetNumberField(TEXT("count"), MatchingActors.Num());

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ActorType, ActorName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("type"), ActorType))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), ActorName))
    {
        return Error;
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FSpirrowBridgeCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
            Details->SetStringField(TEXT("name"), ActorName);
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::AssetAlreadyExists,
                FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName),
                Details);
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    // Check if this is a Volume actor type
    bool bIsVolumeActor = false;
    
    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    // Volume actors (require brush geometry setup)
    else if (ActorType == TEXT("NavMeshBoundsVolume"))
    {
        NewActor = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else if (ActorType == TEXT("TriggerVolume"))
    {
        NewActor = World->SpawnActor<ATriggerVolume>(ATriggerVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else if (ActorType == TEXT("BlockingVolume"))
    {
        NewActor = World->SpawnActor<ABlockingVolume>(ABlockingVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else if (ActorType == TEXT("KillZVolume"))
    {
        NewActor = World->SpawnActor<AKillZVolume>(AKillZVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else if (ActorType == TEXT("PhysicsVolume"))
    {
        NewActor = World->SpawnActor<APhysicsVolume>(APhysicsVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else if (ActorType == TEXT("PostProcessVolume"))
    {
        NewActor = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else if (ActorType == TEXT("AudioVolume"))
    {
        NewActor = World->SpawnActor<AAudioVolume>(AAudioVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else if (ActorType == TEXT("LightmassImportanceVolume"))
    {
        NewActor = World->SpawnActor<ALightmassImportanceVolume>(ALightmassImportanceVolume::StaticClass(), Location, Rotation, SpawnParams);
        bIsVolumeActor = true;
    }
    else
    {
        // Build list of supported types for error message
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("requested_type"), ActorType);
        TArray<TSharedPtr<FJsonValue>> SupportedTypes;
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("StaticMeshActor")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("PointLight")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("SpotLight")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("DirectionalLight")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("CameraActor")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("NavMeshBoundsVolume")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("TriggerVolume")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("BlockingVolume")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("KillZVolume")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("PhysicsVolume")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("PostProcessVolume")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("AudioVolume")));
        SupportedTypes.Add(MakeShared<FJsonValueString>(TEXT("LightmassImportanceVolume")));
        Details->SetArrayField(TEXT("supported_types"), SupportedTypes);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::InvalidActorType,
            FString::Printf(TEXT("Unknown actor type: %s"), *ActorType),
            Details);
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // For Volume actors, create the brush geometry
        if (bIsVolumeActor)
        {
            // Get brush size from params or use scale * default size
            float BrushSizeX = 200.0f * Scale.X;
            float BrushSizeY = 200.0f * Scale.Y;
            float BrushSizeZ = 200.0f * Scale.Z;

            // Check for explicit brush_size parameter
            if (Params->HasField(TEXT("brush_size")))
            {
                FVector BrushSize = FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("brush_size"));
                BrushSizeX = BrushSize.X;
                BrushSizeY = BrushSize.Y;
                BrushSizeZ = BrushSize.Z;
            }

            // Create brush geometry using UCubeBuilder
            UCubeBuilder* CubeBuilder = Cast<UCubeBuilder>(GEditor->FindBrushBuilder(UCubeBuilder::StaticClass()));
            if (CubeBuilder)
            {
                CubeBuilder->X = BrushSizeX;
                CubeBuilder->Y = BrushSizeY;
                CubeBuilder->Z = BrushSizeZ;

                // Use UActorFactory::CreateBrushForVolumeActor to properly create the brush
                AVolume* VolumeActor = Cast<AVolume>(NewActor);
                if (VolumeActor)
                {
                    UActorFactory::CreateBrushForVolumeActor(VolumeActor, CubeBuilder);
                    UE_LOG(LogTemp, Display, TEXT("Created brush geometry for Volume actor: %s (Size: %.0f x %.0f x %.0f)"), 
                        *ActorName, BrushSizeX, BrushSizeY, BrushSizeZ);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to find CubeBuilder for Volume actor: %s"), *ActorName);
            }
        }

        // Return the created actor's details
        TSharedPtr<FJsonObject> Result = FSpirrowBridgeCommonUtils::ActorToJsonObject(NewActor, true);
        if (bIsVolumeActor)
        {
            Result->SetBoolField(TEXT("is_volume"), true);
            Result->SetStringField(TEXT("note"), TEXT("Volume created with brush geometry. Use 'brush_size' parameter to specify custom size."));
        }
        return Result;
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ActorName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), ActorName))
    {
        return Error;
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FSpirrowBridgeCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::ActorNotFound,
        FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ActorName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), ActorName))
    {
        return Error;
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ActorNotFound,
            FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FSpirrowBridgeCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FSpirrowBridgeCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ActorName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), ActorName))
    {
        return Error;
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ActorNotFound,
            FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Always return detailed properties for this command
    return FSpirrowBridgeCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString ActorName, PropertyName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("name"), ActorName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("property_name"), PropertyName))
    {
        return Error;
    }

    // Validate property_value exists
    if (!Params->HasField(TEXT("property_value")))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::MissingRequiredParam,
            TEXT("Missing 'property_value' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::ActorNotFound,
            FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));

    // Check if we're targeting a component
    FString ComponentName;
    UObject* TargetObject = TargetActor;

    if (Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        // Find the component by name
        UActorComponent* FoundComponent = nullptr;

        TArray<UActorComponent*> Components;
        TargetActor->GetComponents(Components);

        for (UActorComponent* Component : Components)
        {
            if (Component && Component->GetName() == ComponentName)
            {
                FoundComponent = Component;
                break;
            }
        }

        if (!FoundComponent)
        {
            // Log available components for debugging
            TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
            Details->SetStringField(TEXT("component_name"), ComponentName);
            Details->SetStringField(TEXT("actor_name"), ActorName);
            
            TArray<TSharedPtr<FJsonValue>> ComponentList;
            for (UActorComponent* Component : Components)
            {
                if (Component)
                {
                    ComponentList.Add(MakeShared<FJsonValueString>(Component->GetName()));
                }
            }
            Details->SetArrayField(TEXT("available_components"), ComponentList);

            return FSpirrowBridgeCommonUtils::CreateErrorResponse(
                ESpirrowErrorCode::ComponentNotFound,
                FString::Printf(TEXT("Component '%s' not found on actor '%s'"), *ComponentName, *ActorName),
                Details);
        }

        TargetObject = FoundComponent;
    }

    // Set the property using our utility function
    FString ErrorMessage;
    if (FSpirrowBridgeCommonUtils::SetObjectProperty(TargetObject, PropertyName, PropertyValue, ErrorMessage))
    {
        // Property set successfully
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        if (!ComponentName.IsEmpty())
        {
            ResultObj->SetStringField(TEXT("component"), ComponentName);
        }
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);

        // Also include the full actor details
        ResultObj->SetObjectField(TEXT("actor_details"), FSpirrowBridgeCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::PropertySetFailed,
            ErrorMessage);
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Validate required parameters
    FString BlueprintName, ActorName;
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("blueprint_name"), BlueprintName))
    {
        return Error;
    }
    if (auto Error = FSpirrowBridgeCommonUtils::ValidateRequiredString(Params, TEXT("actor_name"), ActorName))
    {
        return Error;
    }

    // Get optional parameters
    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(Params, TEXT("path"), Path, TEXT("/Game/Blueprints"));

    // Ensure path ends with /
    if (!Path.EndsWith(TEXT("/")))
    {
        Path += TEXT("/");
    }

    // Find the blueprint
    if (BlueprintName.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
    }

    FString AssetPath = Path + BlueprintName;

    if (!FPackageName::DoesPackageExist(AssetPath))
    {
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("blueprint_name"), BlueprintName);
        Details->SetStringField(TEXT("path"), AssetPath);
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::BlueprintNotFound,
            FString::Printf(TEXT("Blueprint '%s' not found at path: %s"), *BlueprintName, *AssetPath),
            Details);
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!Blueprint)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetLoadFailed,
            FString::Printf(TEXT("Failed to load Blueprint: %s"), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FSpirrowBridgeCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        return FSpirrowBridgeCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        ESpirrowErrorCode::ActorSpawnFailed,
        FString::Printf(TEXT("Failed to spawn blueprint actor '%s'"), *BlueprintName));
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FSpirrowBridgeCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FSpirrowBridgeCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        // Find the actor
        AActor* TargetActor = nullptr;
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && Actor->GetName() == TargetActorName)
            {
                TargetActor = Actor;
                break;
            }
        }

        if (!TargetActor)
        {
            return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    // Get file path parameter
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), FilePath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'filepath' parameter"));
    }
    
    // Ensure the file path has a proper extension
    if (!FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    // Get the active viewport
    if (GEditor && GEditor->GetActiveViewport())
    {
        FViewport* Viewport = GEditor->GetActiveViewport();
        TArray<FColor> Bitmap;
        FIntRect ViewportRect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
        
        if (Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect))
        {
            TArray64<uint8> CompressedBitmap;
            FImageUtils::PNGCompressImageArray(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, Bitmap, CompressedBitmap);
            
            if (FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath))
            {
                TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                ResultObj->SetStringField(TEXT("filepath"), FilePath);
                return ResultObj;
            }
        }
    }
    
    return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Failed to take screenshot"));
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleGetActorComponents(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get all components
    TArray<UActorComponent*> Components;
    TargetActor->GetComponents(Components);

    TArray<TSharedPtr<FJsonValue>> ComponentArray;
    for (UActorComponent* Component : Components)
    {
        if (Component)
        {
            TSharedPtr<FJsonObject> ComponentObj = MakeShared<FJsonObject>();
            ComponentObj->SetStringField(TEXT("name"), Component->GetName());
            ComponentObj->SetStringField(TEXT("class"), Component->GetClass()->GetName());

            // Add scene component specific info
            if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
            {
                FVector Location = SceneComp->GetRelativeLocation();
                TArray<TSharedPtr<FJsonValue>> LocationArray;
                LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
                LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
                LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
                ComponentObj->SetArrayField(TEXT("relative_location"), LocationArray);
            }

            ComponentArray.Add(MakeShared<FJsonValueObject>(ComponentObj));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor"), ActorName);
    ResultObj->SetArrayField(TEXT("components"), ComponentArray);
    ResultObj->SetNumberField(TEXT("count"), ComponentArray.Num());

    return ResultObj;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleRenameActor(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

    FString CurrentName = Params->GetStringField(TEXT("current_name"));
    FString NewName = Params->GetStringField(TEXT("new_name"));

    if (CurrentName.IsEmpty() || NewName.IsEmpty())
    {
        ResultJson->SetStringField(TEXT("status"), TEXT("error"));
        ResultJson->SetStringField(TEXT("error"), TEXT("current_name and new_name are required"));
        return ResultJson;
    }

    // Find the actor
    AActor* FoundActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (World)
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetActorLabel() == CurrentName || It->GetName() == CurrentName)
            {
                FoundActor = *It;
                break;
            }
        }
    }

    if (!FoundActor)
    {
        ResultJson->SetStringField(TEXT("status"), TEXT("error"));
        ResultJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Actor '%s' not found"), *CurrentName));
        return ResultJson;
    }

    // Rename the actor
    FoundActor->SetActorLabel(NewName);
    FoundActor->Rename(*NewName);

    ResultJson->SetStringField(TEXT("status"), TEXT("success"));

    TSharedPtr<FJsonObject> ResultData = MakeShareable(new FJsonObject());
    ResultData->SetStringField(TEXT("old_name"), CurrentName);
    ResultData->SetStringField(TEXT("new_name"), NewName);
    ResultData->SetStringField(TEXT("class"), FoundActor->GetClass()->GetName());
    ResultJson->SetObjectField(TEXT("result"), ResultData);

    return ResultJson;
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleRenameAsset(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject());

    // Get parameters
    FString OldPath;
    FString NewName;

    if (!Params->TryGetStringField(TEXT("old_path"), OldPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'old_path' parameter"));
    }

    if (!Params->TryGetStringField(TEXT("new_name"), NewName))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
    }

    // Check if the asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(OldPath))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *OldPath)
        );
    }

    // Extract directory from old path
    FString Directory = FPaths::GetPath(OldPath);
    FString NewPath = Directory / NewName;

    // Perform the rename operation
    // UEditorAssetLibrary::RenameAsset automatically handles reference updates
    bool bSuccess = UEditorAssetLibrary::RenameAsset(OldPath, NewPath);

    if (bSuccess)
    {
        ResultJson->SetBoolField(TEXT("success"), true);
        ResultJson->SetStringField(TEXT("old_path"), OldPath);
        ResultJson->SetStringField(TEXT("new_path"), NewPath);
        ResultJson->SetStringField(TEXT("message"),
            FString::Printf(TEXT("Asset renamed successfully: %s -> %s"), *OldPath, *NewPath)
        );
    }
    else
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to rename asset: %s"), *OldPath)
        );
    }

    return ResultJson;
}

// ============================================================================
// v0.10.0 - Editor viewport camera + show flag + Live Coding
// ============================================================================

namespace
{
    FLevelEditorViewportClient* GetActivePerspectiveViewport()
    {
        if (!GEditor) return nullptr;
        // Prefer the currently-active viewport client if it's perspective
        if (FViewport* AV = GEditor->GetActiveViewport())
        {
            for (FLevelEditorViewportClient* VC : GEditor->GetLevelViewportClients())
            {
                if (VC && VC->Viewport == AV && VC->IsPerspective())
                {
                    return VC;
                }
            }
        }
        // Fall back to the first perspective viewport
        for (FLevelEditorViewportClient* VC : GEditor->GetLevelViewportClients())
        {
            if (VC && VC->IsPerspective())
            {
                return VC;
            }
        }
        return nullptr;
    }

    void AddVectorJsonField_E(TSharedPtr<FJsonObject>& Obj, const TCHAR* Name, const FVector& V)
    {
        TArray<TSharedPtr<FJsonValue>> Arr;
        Arr.Add(MakeShared<FJsonValueNumber>(V.X));
        Arr.Add(MakeShared<FJsonValueNumber>(V.Y));
        Arr.Add(MakeShared<FJsonValueNumber>(V.Z));
        Obj->SetArrayField(Name, Arr);
    }

    void AddRotatorJsonField_E(TSharedPtr<FJsonObject>& Obj, const TCHAR* Name, const FRotator& R)
    {
        TArray<TSharedPtr<FJsonValue>> Arr;
        Arr.Add(MakeShared<FJsonValueNumber>(R.Pitch));
        Arr.Add(MakeShared<FJsonValueNumber>(R.Yaw));
        Arr.Add(MakeShared<FJsonValueNumber>(R.Roll));
        Obj->SetArrayField(Name, Arr);
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleGetEditorCamera(const TSharedPtr<FJsonObject>& Params)
{
    FLevelEditorViewportClient* VC = GetActivePerspectiveViewport();
    if (!VC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(1710, TEXT("No active perspective editor viewport found"));
    }
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    AddVectorJsonField_E(Data, TEXT("location"), VC->GetViewLocation());
    AddRotatorJsonField_E(Data, TEXT("rotation"), VC->GetViewRotation());
    Data->SetNumberField(TEXT("fov"), VC->ViewFOV);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleSetEditorCamera(const TSharedPtr<FJsonObject>& Params)
{
    FLevelEditorViewportClient* VC = GetActivePerspectiveViewport();
    if (!VC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(1710, TEXT("No active perspective editor viewport found"));
    }

    bool bChanged = false;
    if (Params.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* LocArr = nullptr;
        if (Params->TryGetArrayField(TEXT("location"), LocArr) && LocArr && LocArr->Num() == 3)
        {
            VC->SetViewLocation(FVector((*LocArr)[0]->AsNumber(), (*LocArr)[1]->AsNumber(), (*LocArr)[2]->AsNumber()));
            bChanged = true;
        }
        const TArray<TSharedPtr<FJsonValue>>* RotArr = nullptr;
        if (Params->TryGetArrayField(TEXT("rotation"), RotArr) && RotArr && RotArr->Num() == 3)
        {
            VC->SetViewRotation(FRotator((*RotArr)[0]->AsNumber(), (*RotArr)[1]->AsNumber(), (*RotArr)[2]->AsNumber()));
            bChanged = true;
        }
        double FOV = 0;
        if (Params->TryGetNumberField(TEXT("fov"), FOV))
        {
            VC->ViewFOV = static_cast<float>(FMath::Clamp(FOV, 5.0, 170.0));
            bChanged = true;
        }
    }
    if (!bChanged)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("set_editor_camera requires at least 'location', 'rotation' or 'fov'"));
    }

    VC->Invalidate();

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    AddVectorJsonField_E(Data, TEXT("location"), VC->GetViewLocation());
    AddRotatorJsonField_E(Data, TEXT("rotation"), VC->GetViewRotation());
    Data->SetNumberField(TEXT("fov"), VC->ViewFOV);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleSetShowFlag(const TSharedPtr<FJsonObject>& Params)
{
    FString FlagName;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("flag"), FlagName) || FlagName.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'flag' parameter (e.g. 'Collision', 'Navigation', 'Bounds')"));
    }
    bool bEnabled = true;
    Params->TryGetBoolField(TEXT("enabled"), bEnabled);

    // 'target' = 'editor' (default) or 'pie'. Implementation: route via console showflag.
    FString Target = TEXT("editor");
    Params->TryGetStringField(TEXT("target"), Target);

    UWorld* TargetWorld = nullptr;
    if (Target.Equals(TEXT("pie"), ESearchCase::IgnoreCase))
    {
        TargetWorld = GEditor ? GEditor->PlayWorld : nullptr;
    }
    else
    {
        TargetWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    }
    if (!TargetWorld || !GEngine)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(1713,
            FString::Printf(TEXT("Target world '%s' not available"), *Target));
    }

    const FString Cmd = FString::Printf(TEXT("showflag.%s %d"), *FlagName, bEnabled ? 1 : 0);
    const bool bOk = GEngine->Exec(TargetWorld, *Cmd);
    if (!bOk)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(1713,
            FString::Printf(TEXT("Show flag '%s' was rejected by engine (unknown name?). Tried: %s"), *FlagName, *Cmd));
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("flag"), FlagName);
    Data->SetBoolField(TEXT("enabled"), bEnabled);
    Data->SetStringField(TEXT("target"), Target);
    Data->SetStringField(TEXT("console_command"), Cmd);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleTriggerLiveCoding(const TSharedPtr<FJsonObject>& Params)
{
    ILiveCodingModule* LCModule = FModuleManager::GetModulePtr<ILiveCodingModule>(TEXT("LiveCoding"));
    if (!LCModule)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(1709, TEXT("LiveCoding module not loaded - Live Coding may be disabled in this build"));
    }
    if (!LCModule->IsEnabledForSession())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(1709, TEXT("Live Coding is not enabled for this session - toggle via Editor Preferences > Live Coding"));
    }
    if (LCModule->IsCompiling())
    {
        TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
        Data->SetStringField(TEXT("status"), TEXT("already_compiling"));
        return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
    }
    LCModule->Compile();
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("status"), TEXT("requested"));
    Data->SetStringField(TEXT("note"), TEXT("Live Coding compile is async - completion is signaled in editor. Watch logs for 'PATCH SUCCESS' / 'PATCH FAILURE'"));
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
