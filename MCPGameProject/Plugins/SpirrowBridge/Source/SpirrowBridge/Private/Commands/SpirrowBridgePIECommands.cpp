#include "Commands/SpirrowBridgePIECommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "EngineUtils.h"

// In-memory ring buffer of recent log lines, fed by GLog (used by tail_editor_output_log).
namespace
{
    class FInMemoryLogRing : public FOutputDevice
    {
    public:
        static constexpr int32 MaxLines = 5000;
        TArray<FString> Lines;
        FCriticalSection Mutex;

        virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
        {
            const FString Formatted = FString::Printf(TEXT("[%s]%s: %s"),
                *Category.ToString(),
                ToString(Verbosity),
                V);
            FScopeLock Lock(&Mutex);
            Lines.Add(Formatted);
            if (Lines.Num() > MaxLines)
            {
                Lines.RemoveAt(0, Lines.Num() - MaxLines);
            }
        }

        void GetTail(int32 N, TArray<FString>& Out, const TSet<FString>* SeverityFilter = nullptr)
        {
            FScopeLock Lock(&Mutex);
            const int32 Start = FMath::Max(0, Lines.Num() - N);
            for (int32 i = Start; i < Lines.Num(); ++i)
            {
                if (SeverityFilter && SeverityFilter->Num() > 0)
                {
                    bool bMatched = false;
                    for (const FString& Sev : *SeverityFilter)
                    {
                        if (Lines[i].Contains(Sev))
                        {
                            bMatched = true;
                            break;
                        }
                    }
                    if (!bMatched) continue;
                }
                Out.Add(Lines[i]);
            }
        }

    private:
        const TCHAR* ToString(ELogVerbosity::Type V)
        {
            switch (V)
            {
                case ELogVerbosity::Fatal:       return TEXT("Fatal");
                case ELogVerbosity::Error:       return TEXT("Error");
                case ELogVerbosity::Warning:     return TEXT("Warning");
                case ELogVerbosity::Display:     return TEXT("Display");
                case ELogVerbosity::Log:         return TEXT("Log");
                case ELogVerbosity::Verbose:     return TEXT("Verbose");
                case ELogVerbosity::VeryVerbose: return TEXT("VeryVerbose");
                default:                         return TEXT("");
            }
        }
    };

    FInMemoryLogRing GLogRing;
    bool             GLogRingRegistered = false;
}

FSpirrowBridgePIECommands::FSpirrowBridgePIECommands()
{
    if (!GLogRingRegistered && GLog)
    {
        GLog->AddOutputDevice(&GLogRing);
        GLogRingRegistered = true;
    }
}

namespace
{
    // PIE error codes (1700-1799 range, defined in ERROR_CODES.md v0.10.0)
    static constexpr int32 ErrPIENotRunning      = 1700;
    static constexpr int32 ErrPIEAlreadyRunning  = 1701;
    static constexpr int32 ErrPIEStartFailed     = 1702;
    static constexpr int32 ErrPlayerControllerNotFound = 1703;
    static constexpr int32 ErrPawnNotFound        = 1704;
    static constexpr int32 ErrConsoleCommandFailed = 1705;
    static constexpr int32 ErrHighResScreenshotFailed = 1706;
    static constexpr int32 ErrLogFileNotAccessible = 1707;
    static constexpr int32 ErrLogParseError        = 1708;
    static constexpr int32 ErrLiveCodingUnavailable = 1709;
    static constexpr int32 ErrEditorViewportNotFound = 1710;
    static constexpr int32 ErrInputKeyInvalid      = 1711;
    static constexpr int32 ErrFrameSteppingNotSupported = 1712;
    static constexpr int32 ErrShowFlagInvalid      = 1713;

    UWorld* GetPIEWorldOrNull()
    {
        return (GEditor != nullptr) ? GEditor->PlayWorld : nullptr;
    }

    FViewport* GetPIEViewportOrNull()
    {
        UWorld* PIEWorld = GetPIEWorldOrNull();
        if (!PIEWorld)
        {
            return nullptr;
        }
        UGameViewportClient* GVC = PIEWorld->GetGameViewport();
        return GVC ? GVC->Viewport : nullptr;
    }

    bool ParseFloatTriple(const TArray<TSharedPtr<FJsonValue>>* Arr, double& OutA, double& OutB, double& OutC)
    {
        if (!Arr || Arr->Num() != 3) return false;
        OutA = (*Arr)[0]->AsNumber();
        OutB = (*Arr)[1]->AsNumber();
        OutC = (*Arr)[2]->AsNumber();
        return true;
    }

    void AddVectorJsonField(TSharedPtr<FJsonObject>& Obj, const TCHAR* FieldName, const FVector& V)
    {
        TArray<TSharedPtr<FJsonValue>> Arr;
        Arr.Add(MakeShared<FJsonValueNumber>(V.X));
        Arr.Add(MakeShared<FJsonValueNumber>(V.Y));
        Arr.Add(MakeShared<FJsonValueNumber>(V.Z));
        Obj->SetArrayField(FieldName, Arr);
    }

    void AddRotatorJsonField(TSharedPtr<FJsonObject>& Obj, const TCHAR* FieldName, const FRotator& R)
    {
        TArray<TSharedPtr<FJsonValue>> Arr;
        Arr.Add(MakeShared<FJsonValueNumber>(R.Pitch));
        Arr.Add(MakeShared<FJsonValueNumber>(R.Yaw));
        Arr.Add(MakeShared<FJsonValueNumber>(R.Roll));
        Obj->SetArrayField(FieldName, Arr);
    }

    bool SaveViewportToPNG(FViewport* Viewport, const FString& FilePath, FString& OutError)
    {
        if (!Viewport)
        {
            OutError = TEXT("Viewport is null");
            return false;
        }
        TArray<FColor> Bitmap;
        const FIntPoint Size = Viewport->GetSizeXY();
        FIntRect Rect(0, 0, Size.X, Size.Y);
        if (!Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), Rect))
        {
            OutError = TEXT("Viewport->ReadPixels failed");
            return false;
        }
        TArray64<uint8> Compressed;
        FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Bitmap, Compressed);
        if (!FFileHelper::SaveArrayToFile(Compressed, *FilePath))
        {
            OutError = FString::Printf(TEXT("FFileHelper::SaveArrayToFile failed for '%s'"), *FilePath);
            return false;
        }
        return true;
    }
}

namespace
{
    // Dispatcher entry: command name -> handler member function pointer
    using FHandler = TSharedPtr<FJsonObject> (FSpirrowBridgePIECommands::*)(const TSharedPtr<FJsonObject>&);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // PIE lifecycle
    if (CommandType == TEXT("start_pie"))                  return HandleStartPIE(Params);
    if (CommandType == TEXT("stop_pie"))                   return HandleStopPIE(Params);
    if (CommandType == TEXT("get_pie_state"))              return HandleGetPIEState(Params);
    if (CommandType == TEXT("pause_pie"))                  return HandlePausePIE(Params);
    if (CommandType == TEXT("resume_pie"))                 return HandleResumePIE(Params);
    if (CommandType == TEXT("step_pie_frames"))            return HandleStepPIEFrames(Params);

    // Camera + screenshot
    if (CommandType == TEXT("take_pie_screenshot"))        return HandleTakePIEScreenshot(Params);
    if (CommandType == TEXT("take_high_res_screenshot"))   return HandleTakeHighResScreenshot(Params);
    if (CommandType == TEXT("get_pie_camera"))             return HandleGetPIECamera(Params);
    if (CommandType == TEXT("set_pie_camera"))             return HandleSetPIECamera(Params);
    if (CommandType == TEXT("enable_debug_cam"))           return HandleEnableDebugCam(Params);
    if (CommandType == TEXT("disable_debug_cam"))          return HandleDisableDebugCam(Params);

    // Console + runtime control
    if (CommandType == TEXT("exec_console_command"))       return HandleExecConsoleCommand(Params);
    if (CommandType == TEXT("set_global_time_dilation"))   return HandleSetGlobalTimeDilation(Params);
    if (CommandType == TEXT("simulate_pie_input"))         return HandleSimulatePIEInput(Params);

    // PIE world introspection
    if (CommandType == TEXT("get_pie_actors"))             return HandleGetPIEActors(Params);
    if (CommandType == TEXT("find_pie_actors_by_class"))   return HandleFindPIEActorsByClass(Params);
    if (CommandType == TEXT("get_pie_actor_properties"))   return HandleGetPIEActorProperties(Params);

    // Log access
    if (CommandType == TEXT("tail_ue_log"))                return HandleTailUELog(Params);
    if (CommandType == TEXT("filter_ue_log"))              return HandleFilterUELog(Params);
    if (CommandType == TEXT("set_log_verbosity"))          return HandleSetLogVerbosity(Params);
    if (CommandType == TEXT("get_ue_log_path"))            return HandleGetUELogPath(Params);
    if (CommandType == TEXT("scan_ue_log_errors"))         return HandleScanUELogErrors(Params);
    if (CommandType == TEXT("search_ue_log"))              return HandleSearchUELog(Params);
    if (CommandType == TEXT("tail_editor_output_log"))     return HandleTailEditorOutputLog(Params);

    return FSpirrowBridgeCommonUtils::CreateErrorResponse(
        FString::Printf(TEXT("Unknown PIE command: %s"), *CommandType));
}

// ============================================================================
// Stub implementations - filled in by subsequent steps (v0.10.0 plan).
// All return ErrorCode 1000-range "not implemented" until their step lands.
// ============================================================================

namespace
{
    TSharedPtr<FJsonObject> NotImplemented(const TCHAR* CmdName)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("PIE command '%s' not yet implemented (v0.10.0 in progress)"), CmdName));
    }
}

// ----------------------------------------------------------------------------
// PIE lifecycle (Step 2 - v0.10.0)
// ----------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleStartPIE(const TSharedPtr<FJsonObject>& Params)
{
    if (!GEditor)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIEStartFailed, TEXT("GEditor is null"));
    }
    if (GEditor->PlayWorld != nullptr)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPIEAlreadyRunning, TEXT("PIE is already running. Call stop_pie first"));
    }

    FRequestPlaySessionParams PlayParams;

    FString Mode = TEXT("selected_viewport");
    if (Params.IsValid())
    {
        Params->TryGetStringField(TEXT("mode"), Mode);
    }
    if (Mode.Equals(TEXT("new_window"), ESearchCase::IgnoreCase))
    {
        PlayParams.SessionDestination = EPlaySessionDestinationType::NewProcess;
    }
    // For "selected_viewport" / default: leave as InProcess (FRequestPlaySessionParams default)

    bool bSpawnAtPlayerStart = true;
    if (Params.IsValid())
    {
        Params->TryGetBoolField(TEXT("spawn_at_player_start"), bSpawnAtPlayerStart);
    }
    if (!bSpawnAtPlayerStart && Params.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* LocArr = nullptr;
        if (Params->TryGetArrayField(TEXT("spawn_location"), LocArr) && LocArr && LocArr->Num() == 3)
        {
            PlayParams.StartLocation = FVector(
                (*LocArr)[0]->AsNumber(),
                (*LocArr)[1]->AsNumber(),
                (*LocArr)[2]->AsNumber());
        }
        const TArray<TSharedPtr<FJsonValue>>* RotArr = nullptr;
        if (Params->TryGetArrayField(TEXT("spawn_rotation"), RotArr) && RotArr && RotArr->Num() == 3)
        {
            PlayParams.StartRotation = FRotator(
                (*RotArr)[0]->AsNumber(),
                (*RotArr)[1]->AsNumber(),
                (*RotArr)[2]->AsNumber());
        }
    }

    GEditor->RequestPlaySession(PlayParams);

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("status"), TEXT("requested"));
    Data->SetStringField(TEXT("mode"), Mode);
    Data->SetStringField(TEXT("note"), TEXT("PIE will start on the next editor tick - poll get_pie_state to confirm 'running'"));
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleStopPIE(const TSharedPtr<FJsonObject>& Params)
{
    if (!GEditor)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("GEditor is null"));
    }
    if (GEditor->PlayWorld == nullptr)
    {
        TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
        Data->SetStringField(TEXT("status"), TEXT("not_running"));
        Data->SetStringField(TEXT("note"), TEXT("PIE was not running - stop_pie is a no-op"));
        return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
    }

    GEditor->RequestEndPlayMap();

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("status"), TEXT("requested"));
    Data->SetStringField(TEXT("note"), TEXT("PIE end requested. World tears down on the next tick"));
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleGetPIEState(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    UWorld* PIEWorld = GetPIEWorldOrNull();

    if (!PIEWorld)
    {
        Data->SetStringField(TEXT("state"), TEXT("stopped"));
        return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
    }

    Data->SetStringField(TEXT("state"), PIEWorld->IsPaused() ? TEXT("paused") : TEXT("running"));
    Data->SetNumberField(TEXT("time_seconds"), PIEWorld->GetTimeSeconds());
    Data->SetNumberField(TEXT("real_time_seconds"), PIEWorld->GetRealTimeSeconds());
    Data->SetNumberField(TEXT("delta_seconds"), PIEWorld->GetDeltaSeconds());

    int32 PlayerCount = 0;
    for (FConstPlayerControllerIterator It = PIEWorld->GetPlayerControllerIterator(); It; ++It)
    {
        ++PlayerCount;
    }
    Data->SetNumberField(TEXT("player_count"), PlayerCount);

    if (UPackage* Pkg = PIEWorld->GetOutermost())
    {
        Data->SetStringField(TEXT("world_package"), Pkg->GetName());
    }
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandlePausePIE(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }

    int32 PlayerIndex = 0;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("player_index"), Tmp))
        {
            PlayerIndex = static_cast<int32>(Tmp);
        }
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(PIEWorld, PlayerIndex);
    if (!PC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPlayerControllerNotFound,
            FString::Printf(TEXT("PlayerController not found at index %d"), PlayerIndex));
    }

    PC->SetPause(true);

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("state"), TEXT("paused"));
    Data->SetNumberField(TEXT("player_index"), PlayerIndex);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleResumePIE(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }

    int32 PlayerIndex = 0;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("player_index"), Tmp))
        {
            PlayerIndex = static_cast<int32>(Tmp);
        }
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(PIEWorld, PlayerIndex);
    if (!PC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPlayerControllerNotFound,
            FString::Printf(TEXT("PlayerController not found at index %d"), PlayerIndex));
    }

    PC->SetPause(false);

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("state"), TEXT("running"));
    Data->SetNumberField(TEXT("player_index"), PlayerIndex);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
// ----------------------------------------------------------------------------
// Frame stepping (Step 8 - v0.10.0)
//
// UE has no built-in "advance N frames" API. We resume PIE, install an
// OnEndFrame delegate, count frames, then re-pause + unregister.
// One step session at a time (rejected if already in flight).
// ----------------------------------------------------------------------------

namespace
{
    struct FStepSession
    {
        int32 Remaining = 0;
        TWeakObjectPtr<APlayerController> WeakPC;
        FDelegateHandle Handle;
    };
    TSharedPtr<FStepSession> GStepSession;
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleStepPIEFrames(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    if (GStepSession.IsValid())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrFrameSteppingNotSupported, TEXT("A step_pie_frames session is already in progress"));
    }

    int32 Frames = 1;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("frames"), Tmp))
        {
            Frames = FMath::Clamp(static_cast<int32>(Tmp), 1, 100);
        }
    }
    APlayerController* PC = UGameplayStatics::GetPlayerController(PIEWorld, 0);
    if (!PC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPlayerControllerNotFound, TEXT("PlayerController not found at index 0"));
    }

    GStepSession = MakeShared<FStepSession>();
    GStepSession->Remaining = Frames;
    GStepSession->WeakPC = PC;

    // Resume to let frames advance
    PC->SetPause(false);

    GStepSession->Handle = FCoreDelegates::OnEndFrame.AddLambda([]()
    {
        if (!GStepSession.IsValid()) return;
        --(GStepSession->Remaining);
        if (GStepSession->Remaining <= 0)
        {
            if (GStepSession->WeakPC.IsValid())
            {
                GStepSession->WeakPC->SetPause(true);
            }
            FCoreDelegates::OnEndFrame.Remove(GStepSession->Handle);
            GStepSession.Reset();
        }
    });

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetNumberField(TEXT("frames"), Frames);
    Data->SetStringField(TEXT("status"), TEXT("requested"));
    Data->SetStringField(TEXT("note"), TEXT("Stepping is async - PIE will be paused again after N frames. Poll get_pie_state to confirm 'paused'"));
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

// ----------------------------------------------------------------------------
// Camera + screenshot (Step 3 - v0.10.0)
// ----------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleTakePIEScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    FViewport* Viewport = GetPIEViewportOrNull();
    if (!Viewport)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPIENotRunning, TEXT("PIE viewport not available (PIE not running, or no GameViewportClient)"));
    }

    FString FilePath;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("filepath"), FilePath) || FilePath.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'filepath' parameter"));
    }
    if (!FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    FString Err;
    if (!SaveViewportToPNG(Viewport, FilePath, Err))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(Err);
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("filepath"), FilePath);
    Data->SetStringField(TEXT("source"), TEXT("pie_viewport"));
    Data->SetNumberField(TEXT("width"), Viewport->GetSizeXY().X);
    Data->SetNumberField(TEXT("height"), Viewport->GetSizeXY().Y);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleTakeHighResScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    int32 Multiplier = 2;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("multiplier"), Tmp))
        {
            Multiplier = FMath::Clamp(static_cast<int32>(Tmp), 1, 10);
        }
    }

    UWorld* TargetWorld = GetPIEWorldOrNull();
    if (!TargetWorld)
    {
        TargetWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    }
    if (!TargetWorld || !GEngine)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrHighResScreenshotFailed, TEXT("No editor or PIE world available for HighResShot"));
    }

    // Optional: filename override via CVar before exec
    FString OptionalPath;
    if (Params.IsValid())
    {
        Params->TryGetStringField(TEXT("filepath"), OptionalPath);
    }
    if (!OptionalPath.IsEmpty())
    {
        if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HighResScreenshotConfig.FilenameOverride")))
        {
            CVar->Set(*OptionalPath);
        }
    }

    const FString Cmd = FString::Printf(TEXT("HighResShot %d"), Multiplier);
    const bool bOk = GEngine->Exec(TargetWorld, *Cmd);
    if (!bOk)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrHighResScreenshotFailed, FString::Printf(TEXT("GEngine->Exec '%s' returned false"), *Cmd));
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetNumberField(TEXT("multiplier"), Multiplier);
    Data->SetStringField(TEXT("status"), TEXT("requested"));
    Data->SetStringField(TEXT("note"), TEXT("HighResShot is async - file is written to Saved/Screenshots/<Platform>/HighresScreenshotNNNNN.png on the next frame"));
    if (!OptionalPath.IsEmpty())
    {
        Data->SetStringField(TEXT("filepath_override"), OptionalPath);
    }
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleGetPIECamera(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }

    int32 PlayerIndex = 0;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("player_index"), Tmp))
        {
            PlayerIndex = static_cast<int32>(Tmp);
        }
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(PIEWorld, PlayerIndex);
    if (!PC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPlayerControllerNotFound,
            FString::Printf(TEXT("PlayerController not found at index %d"), PlayerIndex));
    }

    FVector CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    AddVectorJsonField(Data, TEXT("location"), CamLoc);
    AddRotatorJsonField(Data, TEXT("rotation"), CamRot);
    if (PC->PlayerCameraManager)
    {
        Data->SetNumberField(TEXT("fov"), PC->PlayerCameraManager->GetFOVAngle());
    }
    Data->SetNumberField(TEXT("player_index"), PlayerIndex);
    if (APawn* Pawn = PC->GetPawn())
    {
        Data->SetStringField(TEXT("pawn_name"), Pawn->GetName());
        Data->SetStringField(TEXT("pawn_class"), Pawn->GetClass()->GetName());
    }
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleSetPIECamera(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }

    int32 PlayerIndex = 0;
    bool bUseDebugCam = false;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("player_index"), Tmp))
        {
            PlayerIndex = static_cast<int32>(Tmp);
        }
        Params->TryGetBoolField(TEXT("use_debug_cam"), bUseDebugCam);
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(PIEWorld, PlayerIndex);
    if (!PC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPlayerControllerNotFound,
            FString::Printf(TEXT("PlayerController not found at index %d"), PlayerIndex));
    }

    // Parse target location/rotation from params
    bool bHasLoc = false;
    bool bHasRot = false;
    FVector NewLoc(ForceInitToZero);
    FRotator NewRot(ForceInitToZero);

    if (Params.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* LocArr = nullptr;
        if (Params->TryGetArrayField(TEXT("location"), LocArr) && LocArr && LocArr->Num() == 3)
        {
            NewLoc = FVector((*LocArr)[0]->AsNumber(), (*LocArr)[1]->AsNumber(), (*LocArr)[2]->AsNumber());
            bHasLoc = true;
        }
        const TArray<TSharedPtr<FJsonValue>>* RotArr = nullptr;
        if (Params->TryGetArrayField(TEXT("rotation"), RotArr) && RotArr && RotArr->Num() == 3)
        {
            NewRot = FRotator((*RotArr)[0]->AsNumber(), (*RotArr)[1]->AsNumber(), (*RotArr)[2]->AsNumber());
            bHasRot = true;
        }
    }
    if (!bHasLoc && !bHasRot)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("set_pie_camera requires at least 'location' or 'rotation'"));
    }

    if (bUseDebugCam && GEngine)
    {
        // Toggle DebugCam via console (free-look). Not full camera teleport - caller can follow up with editor camera APIs.
        GEngine->Exec(PIEWorld, TEXT("ToggleDebugCamera"));
    }

    APawn* Pawn = PC->GetPawn();
    if (Pawn)
    {
        const FVector ApplyLoc = bHasLoc ? NewLoc : Pawn->GetActorLocation();
        const FRotator ApplyRot = bHasRot ? NewRot : Pawn->GetActorRotation();
        FHitResult Hit;
        Pawn->SetActorLocationAndRotation(ApplyLoc, ApplyRot, false, &Hit, ETeleportType::TeleportPhysics);
        if (bHasRot)
        {
            PC->SetControlRotation(NewRot);
        }
    }
    else
    {
        // No possessed pawn - best effort: set control rotation only
        if (bHasRot)
        {
            PC->SetControlRotation(NewRot);
        }
    }

    // Read back actual final view
    FVector FinalLoc;
    FRotator FinalRot;
    PC->GetPlayerViewPoint(FinalLoc, FinalRot);

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    AddVectorJsonField(Data, TEXT("location"), FinalLoc);
    AddRotatorJsonField(Data, TEXT("rotation"), FinalRot);
    Data->SetBoolField(TEXT("debug_cam_toggled"), bUseDebugCam);
    Data->SetBoolField(TEXT("pawn_teleported"), Pawn != nullptr);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleEnableDebugCam(const TSharedPtr<FJsonObject>&)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld || !GEngine)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    GEngine->Exec(PIEWorld, TEXT("ToggleDebugCamera"));
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetBoolField(TEXT("debug_cam_toggled"), true);
    Data->SetStringField(TEXT("note"), TEXT("ToggleDebugCamera is a toggle - call disable_debug_cam (same console command) to flip back"));
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleDisableDebugCam(const TSharedPtr<FJsonObject>&)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld || !GEngine)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    GEngine->Exec(PIEWorld, TEXT("ToggleDebugCamera"));
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetBoolField(TEXT("debug_cam_toggled"), true);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
// ----------------------------------------------------------------------------
// Console exec with output capture (Step 4 - v0.10.0)
// ----------------------------------------------------------------------------

namespace
{
    class FStringOutputCapture : public FOutputDevice
    {
    public:
        FString Captured;
        virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
        {
            Captured.Append(V);
            Captured.Append(TEXT("\n"));
        }
    };
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleExecConsoleCommand(const TSharedPtr<FJsonObject>& Params)
{
    FString Command;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("command"), Command) || Command.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'command' parameter"));
    }

    FString Target = TEXT("auto");
    Params->TryGetStringField(TEXT("target"), Target);

    UWorld* TargetWorld = nullptr;
    FString ResolvedTarget;
    if (Target.Equals(TEXT("editor"), ESearchCase::IgnoreCase))
    {
        TargetWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        ResolvedTarget = TEXT("editor");
    }
    else if (Target.Equals(TEXT("pie"), ESearchCase::IgnoreCase))
    {
        TargetWorld = GetPIEWorldOrNull();
        ResolvedTarget = TEXT("pie");
    }
    else
    {
        // auto: prefer PIE if running, else editor
        if (UWorld* PW = GetPIEWorldOrNull())
        {
            TargetWorld = PW;
            ResolvedTarget = TEXT("pie");
        }
        else
        {
            TargetWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
            ResolvedTarget = TEXT("editor");
        }
    }

    if (!TargetWorld || !GEngine)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrConsoleCommandFailed,
            FString::Printf(TEXT("Target world '%s' not available"), *Target));
    }

    FStringOutputCapture Capture;
    const bool bOk = GEngine->Exec(TargetWorld, *Command, Capture);

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetBoolField(TEXT("exec_returned_true"), bOk);
    Data->SetStringField(TEXT("command"), Command);
    Data->SetStringField(TEXT("target"), ResolvedTarget);
    Data->SetStringField(TEXT("output"), Capture.Captured);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
// ----------------------------------------------------------------------------
// Runtime control + introspection (Step 5 - v0.10.0)
// ----------------------------------------------------------------------------

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleSetGlobalTimeDilation(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    double Dilation = 0;
    if (!Params.IsValid() || !Params->TryGetNumberField(TEXT("dilation"), Dilation))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'dilation' parameter (float)"));
    }
    Dilation = FMath::Clamp(Dilation, 0.0001, 20.0);
    UGameplayStatics::SetGlobalTimeDilation(PIEWorld, static_cast<float>(Dilation));

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetNumberField(TEXT("dilation"), Dilation);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleSimulatePIEInput(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    FString KeyName;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("key"), KeyName) || KeyName.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter (e.g. 'Jump', 'W', 'LeftMouseButton')"));
    }
    FString Event = TEXT("tap");
    Params->TryGetStringField(TEXT("event"), Event);

    int32 PlayerIndex = 0;
    double Tmp = 0;
    if (Params->TryGetNumberField(TEXT("player_index"), Tmp))
    {
        PlayerIndex = static_cast<int32>(Tmp);
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(PIEWorld, PlayerIndex);
    if (!PC)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrPlayerControllerNotFound, FString::Printf(TEXT("PlayerController not found at index %d"), PlayerIndex));
    }

    FKey Key(*KeyName);
    if (!Key.IsValid())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ErrInputKeyInvalid, FString::Printf(TEXT("Unknown key '%s' (FKey not valid)"), *KeyName));
    }

    FViewport* InputViewport = GetPIEViewportOrNull();
    auto PressKey = [&]()
    {
        FInputKeyEventArgs Args(InputViewport, PlayerIndex, Key, IE_Pressed, 1.0f, Key.IsGamepadKey());
        PC->InputKey(Args);
    };
    auto ReleaseKey = [&]()
    {
        FInputKeyEventArgs Args(InputViewport, PlayerIndex, Key, IE_Released, 0.0f, Key.IsGamepadKey());
        PC->InputKey(Args);
    };

    if (Event.Equals(TEXT("press"), ESearchCase::IgnoreCase))
    {
        PressKey();
    }
    else if (Event.Equals(TEXT("release"), ESearchCase::IgnoreCase))
    {
        ReleaseKey();
    }
    else  // "tap" default
    {
        PressKey();
        ReleaseKey();
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("key"), KeyName);
    Data->SetStringField(TEXT("event"), Event);
    Data->SetNumberField(TEXT("player_index"), PlayerIndex);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

namespace
{
    void FillActorJson(TSharedPtr<FJsonObject>& Obj, AActor* Actor, bool bIncludeComponents)
    {
        Obj->SetStringField(TEXT("name"), Actor->GetName());
        Obj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
        Obj->SetStringField(TEXT("class_path"), Actor->GetClass()->GetPathName());
        AddVectorJsonField(Obj, TEXT("location"), Actor->GetActorLocation());
        AddRotatorJsonField(Obj, TEXT("rotation"), Actor->GetActorRotation());
        AddVectorJsonField(Obj, TEXT("scale"), Actor->GetActorScale3D());
        if (bIncludeComponents)
        {
            TArray<TSharedPtr<FJsonValue>> Comps;
            for (UActorComponent* Comp : Actor->GetComponents())
            {
                if (!Comp) continue;
                TSharedPtr<FJsonObject> CObj = MakeShared<FJsonObject>();
                CObj->SetStringField(TEXT("name"), Comp->GetName());
                CObj->SetStringField(TEXT("class"), Comp->GetClass()->GetName());
                Comps.Add(MakeShared<FJsonValueObject>(CObj));
            }
            Obj->SetArrayField(TEXT("components"), Comps);
        }
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleGetPIEActors(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    bool bIncludeComponents = false;
    if (Params.IsValid())
    {
        Params->TryGetBoolField(TEXT("include_components"), bIncludeComponents);
    }
    TArray<TSharedPtr<FJsonValue>> Actors;
    for (TActorIterator<AActor> It(PIEWorld); It; ++It)
    {
        AActor* A = *It;
        if (!A) continue;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        FillActorJson(Obj, A, bIncludeComponents);
        Actors.Add(MakeShared<FJsonValueObject>(Obj));
    }
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetArrayField(TEXT("actors"), Actors);
    Data->SetNumberField(TEXT("count"), Actors.Num());
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleFindPIEActorsByClass(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    FString ClassName;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("class_name"), ClassName) || ClassName.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'class_name' parameter"));
    }
    bool bExactMatch = false;
    Params->TryGetBoolField(TEXT("exact_match"), bExactMatch);

    UClass* SearchClass = FSpirrowBridgeCommonUtils::FindClassByNameAnywhere(ClassName);
    if (!SearchClass)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Class not found: '%s'"), *ClassName));
    }

    TArray<TSharedPtr<FJsonValue>> Actors;
    for (TActorIterator<AActor> It(PIEWorld); It; ++It)
    {
        AActor* A = *It;
        if (!A) continue;
        const bool bMatch = bExactMatch ? (A->GetClass() == SearchClass) : A->IsA(SearchClass);
        if (!bMatch) continue;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        FillActorJson(Obj, A, false);
        Actors.Add(MakeShared<FJsonValueObject>(Obj));
    }
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("class_name"), ClassName);
    Data->SetStringField(TEXT("class_path"), SearchClass->GetPathName());
    Data->SetBoolField(TEXT("exact_match"), bExactMatch);
    Data->SetArrayField(TEXT("actors"), Actors);
    Data->SetNumberField(TEXT("count"), Actors.Num());
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleGetPIEActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* PIEWorld = GetPIEWorldOrNull();
    if (!PIEWorld)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrPIENotRunning, TEXT("PIE is not running"));
    }
    FString ActorName;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("name"), ActorName) || ActorName.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    AActor* Found = nullptr;
    for (TActorIterator<AActor> It(PIEWorld); It; ++It)
    {
        if (It->GetName().Equals(ActorName, ESearchCase::IgnoreCase))
        {
            Found = *It;
            break;
        }
    }
    if (!Found)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("PIE actor not found: '%s'"), *ActorName));
    }

    TArray<FString> NameFilter;
    if (Params.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* PropArr = nullptr;
        if (Params->TryGetArrayField(TEXT("properties"), PropArr) && PropArr)
        {
            for (const auto& V : *PropArr)
            {
                FString S;
                if (V.IsValid() && V->TryGetString(S) && !S.IsEmpty())
                {
                    NameFilter.Add(S);
                }
            }
        }
    }

    TSharedPtr<FJsonObject> Props = MakeShared<FJsonObject>();
    UClass* Cls = Found->GetClass();
    for (TFieldIterator<FProperty> It(Cls); It; ++It)
    {
        FProperty* Prop = *It;
        if (!Prop) continue;
        const FString PName = Prop->GetName();
        if (NameFilter.Num() > 0 && !NameFilter.Contains(PName))
        {
            continue;
        }
        const void* Value = Prop->ContainerPtrToValuePtr<void>(Found);
        FString Exported;
        Prop->ExportTextItem_Direct(Exported, Value, nullptr, Found, PPF_None);
        Props->SetStringField(PName, Exported);
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("name"), ActorName);
    Data->SetStringField(TEXT("class"), Cls->GetName());
    Data->SetStringField(TEXT("class_path"), Cls->GetPathName());
    Data->SetObjectField(TEXT("properties"), Props);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
// ----------------------------------------------------------------------------
// Log access (Step 6 - v0.10.0)
//
// All disk-backed commands target Saved/Logs/<Project>.log via FPaths.
// `tail_editor_output_log` reads from the in-memory GLogRing instead.
//
// Structured parsing is intentionally left to the Python side - we just
// return the raw lines and `search_ue_log` matches them as substrings.
// (Per-line UE log format is `[YYYY.MM.DD-HH.MM.SS:ms][frame]Cat: Sev: msg`,
// trivial to regex-parse client-side without inflating the C++ payload.)
// ----------------------------------------------------------------------------

namespace
{
    FString GetActiveLogFilePath()
    {
        const FString LogDir = FPaths::ProjectLogDir();
        const FString ProjectName = FApp::GetProjectName();
        FString Combined = FPaths::Combine(LogDir, ProjectName + TEXT(".log"));
        return FPaths::ConvertRelativePathToFull(Combined);
    }

    bool ReadLogFileLines(int32 MaxLines, TArray<FString>& OutLines, FString& OutPath, FString& OutError)
    {
        OutPath = GetActiveLogFilePath();
        if (!FPaths::FileExists(OutPath))
        {
            OutError = FString::Printf(TEXT("Log file not found: %s"), *OutPath);
            return false;
        }
        // FILEREAD_AllowWrite: UE editor holds the log open with exclusive write lock,
        // so the default LoadFileToString fails with sharing violation. AllowWrite tells
        // the reader to set FILE_SHARE_WRITE so we can read alongside the writer.
        FString Content;
        if (!FFileHelper::LoadFileToString(Content, *OutPath, FFileHelper::EHashOptions::None, FILEREAD_AllowWrite))
        {
            OutError = FString::Printf(TEXT("Failed to read log file: %s"), *OutPath);
            return false;
        }
        TArray<FString> AllLines;
        Content.ParseIntoArrayLines(AllLines, /*bCullEmpty=*/false);
        const int32 Start = FMath::Max(0, AllLines.Num() - MaxLines);
        for (int32 i = Start; i < AllLines.Num(); ++i)
        {
            OutLines.Add(AllLines[i]);
        }
        return true;
    }
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleTailUELog(const TSharedPtr<FJsonObject>& Params)
{
    int32 LineCount = 50;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("lines"), Tmp))
        {
            LineCount = FMath::Clamp(static_cast<int32>(Tmp), 1, 100000);
        }
    }
    TArray<FString> Lines;
    FString Path, Err;
    if (!ReadLogFileLines(LineCount, Lines, Path, Err))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrLogFileNotAccessible, Err);
    }
    TArray<TSharedPtr<FJsonValue>> Out;
    for (const FString& L : Lines) Out.Add(MakeShared<FJsonValueString>(L));

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("path"), Path);
    Data->SetArrayField(TEXT("lines"), Out);
    Data->SetNumberField(TEXT("count"), Out.Num());
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleFilterUELog(const TSharedPtr<FJsonObject>& Params)
{
    FString Category;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("category"), Category) || Category.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'category' parameter"));
    }
    int32 LineCount = 50;
    double Tmp = 0;
    if (Params->TryGetNumberField(TEXT("lines"), Tmp))
    {
        LineCount = FMath::Clamp(static_cast<int32>(Tmp), 1, 100000);
    }
    // Read up to 50000 trailing lines from disk and filter
    TArray<FString> All;
    FString Path, Err;
    if (!ReadLogFileLines(50000, All, Path, Err))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrLogFileNotAccessible, Err);
    }
    TArray<FString> Matched;
    for (const FString& L : All)
    {
        if (L.Contains(Category))
        {
            Matched.Add(L);
        }
    }
    const int32 Start = FMath::Max(0, Matched.Num() - LineCount);
    TArray<TSharedPtr<FJsonValue>> Out;
    for (int32 i = Start; i < Matched.Num(); ++i)
    {
        Out.Add(MakeShared<FJsonValueString>(Matched[i]));
    }
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("path"), Path);
    Data->SetStringField(TEXT("category"), Category);
    Data->SetArrayField(TEXT("lines"), Out);
    Data->SetNumberField(TEXT("count"), Out.Num());
    Data->SetNumberField(TEXT("scanned_lines"), All.Num());
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleSetLogVerbosity(const TSharedPtr<FJsonObject>& Params)
{
    FString Category, Verbosity;
    if (!Params.IsValid()
        || !Params->TryGetStringField(TEXT("category"), Category) || Category.IsEmpty()
        || !Params->TryGetStringField(TEXT("verbosity"), Verbosity) || Verbosity.IsEmpty())
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("Missing 'category' or 'verbosity' parameter"));
    }
    if (!GEngine)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(TEXT("GEngine null"));
    }
    const FString Cmd = FString::Printf(TEXT("Log %s %s"), *Category, *Verbosity);
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    GEngine->Exec(World, *Cmd);

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("category"), Category);
    Data->SetStringField(TEXT("verbosity"), Verbosity);
    Data->SetStringField(TEXT("console_command"), Cmd);
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleGetUELogPath(const TSharedPtr<FJsonObject>&)
{
    const FString Path = GetActiveLogFilePath();
    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("path"), Path);
    Data->SetBoolField(TEXT("exists"), FPaths::FileExists(Path));
    if (FPaths::FileExists(Path))
    {
        Data->SetNumberField(TEXT("size_bytes"), IFileManager::Get().FileSize(*Path));
    }
    Data->SetStringField(TEXT("project_name"), FApp::GetProjectName());
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleScanUELogErrors(const TSharedPtr<FJsonObject>& Params)
{
    int32 ScanLines = 5000;
    int32 MaxResults = 200;
    TArray<FString> SeverityFilter = { TEXT("Error"), TEXT("Warning"), TEXT("Fatal") };

    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("lines"), Tmp))
        {
            ScanLines = FMath::Clamp(static_cast<int32>(Tmp), 100, 100000);
        }
        if (Params->TryGetNumberField(TEXT("max_results"), Tmp))
        {
            MaxResults = FMath::Clamp(static_cast<int32>(Tmp), 1, 10000);
        }
        const TArray<TSharedPtr<FJsonValue>>* SevArr = nullptr;
        if (Params->TryGetArrayField(TEXT("severity_filter"), SevArr) && SevArr)
        {
            SeverityFilter.Reset();
            for (const auto& V : *SevArr)
            {
                FString S;
                if (V.IsValid() && V->TryGetString(S) && !S.IsEmpty())
                {
                    SeverityFilter.Add(S);
                }
            }
        }
    }

    TArray<FString> Lines;
    FString Path, Err;
    if (!ReadLogFileLines(ScanLines, Lines, Path, Err))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrLogFileNotAccessible, Err);
    }

    TArray<TSharedPtr<FJsonValue>> Out;
    for (const FString& L : Lines)
    {
        bool bMatched = false;
        for (const FString& Sev : SeverityFilter)
        {
            // Match `: <Sev>:` to avoid false positives like "WarningCount" in message body.
            const FString Marker = FString::Printf(TEXT(": %s:"), *Sev);
            if (L.Contains(Marker) || L.Contains(FString::Printf(TEXT("%s "), *Sev)))
            {
                bMatched = true;
                break;
            }
        }
        if (!bMatched) continue;
        Out.Add(MakeShared<FJsonValueString>(L));
        if (Out.Num() >= MaxResults) break;
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("path"), Path);
    Data->SetArrayField(TEXT("lines"), Out);
    Data->SetNumberField(TEXT("count"), Out.Num());
    Data->SetNumberField(TEXT("scanned_lines"), Lines.Num());
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleSearchUELog(const TSharedPtr<FJsonObject>& Params)
{
    int32 ScanLines = 5000;
    int32 MaxResults = 200;
    TArray<FString> Keywords;
    TArray<FString> Severities;
    TArray<FString> Categories;

    auto AddStringOrArray = [&Params](const TCHAR* FieldName, TArray<FString>& Out)
    {
        FString S;
        if (Params->TryGetStringField(FieldName, S))
        {
            if (!S.IsEmpty()) Out.Add(S);
            return;
        }
        const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
        if (Params->TryGetArrayField(FieldName, Arr) && Arr)
        {
            for (const auto& V : *Arr)
            {
                FString Tmp;
                if (V.IsValid() && V->TryGetString(Tmp) && !Tmp.IsEmpty())
                {
                    Out.Add(Tmp);
                }
            }
        }
    };

    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("lines"), Tmp))
        {
            ScanLines = FMath::Clamp(static_cast<int32>(Tmp), 100, 100000);
        }
        if (Params->TryGetNumberField(TEXT("max_results"), Tmp))
        {
            MaxResults = FMath::Clamp(static_cast<int32>(Tmp), 1, 10000);
        }
        AddStringOrArray(TEXT("keyword"),  Keywords);
        AddStringOrArray(TEXT("severity"), Severities);
        AddStringOrArray(TEXT("category"), Categories);
    }

    TArray<FString> Lines;
    FString Path, Err;
    if (!ReadLogFileLines(ScanLines, Lines, Path, Err))
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(ErrLogFileNotAccessible, Err);
    }

    TArray<TSharedPtr<FJsonValue>> Out;
    for (const FString& L : Lines)
    {
        if (Keywords.Num() > 0)
        {
            bool bMatched = false;
            for (const FString& K : Keywords)
            {
                if (L.Contains(K)) { bMatched = true; break; }
            }
            if (!bMatched) continue;
        }
        if (Severities.Num() > 0)
        {
            bool bMatched = false;
            for (const FString& S : Severities)
            {
                if (L.Contains(FString::Printf(TEXT(": %s:"), *S))
                 || L.Contains(FString::Printf(TEXT("%s "), *S)))
                {
                    bMatched = true; break;
                }
            }
            if (!bMatched) continue;
        }
        if (Categories.Num() > 0)
        {
            bool bMatched = false;
            for (const FString& C : Categories)
            {
                if (L.Contains(C)) { bMatched = true; break; }
            }
            if (!bMatched) continue;
        }
        Out.Add(MakeShared<FJsonValueString>(L));
        if (Out.Num() >= MaxResults) break;
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("path"), Path);
    Data->SetArrayField(TEXT("lines"), Out);
    Data->SetNumberField(TEXT("count"), Out.Num());
    Data->SetNumberField(TEXT("scanned_lines"), Lines.Num());
    Data->SetStringField(TEXT("note"), TEXT("Per-line parsing (timestamp/frame/category/severity/message) is left to the client - UE log format is `[YYYY.MM.DD-HH.MM.SS:ms][frame]Cat: Sev: msg`"));
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FSpirrowBridgePIECommands::HandleTailEditorOutputLog(const TSharedPtr<FJsonObject>& Params)
{
    int32 LineCount = 50;
    TSet<FString> SeverityFilter;
    if (Params.IsValid())
    {
        double Tmp = 0;
        if (Params->TryGetNumberField(TEXT("lines"), Tmp))
        {
            LineCount = FMath::Clamp(static_cast<int32>(Tmp), 1, FInMemoryLogRing::MaxLines);
        }
        const TArray<TSharedPtr<FJsonValue>>* SevArr = nullptr;
        if (Params->TryGetArrayField(TEXT("severity_filter"), SevArr) && SevArr)
        {
            for (const auto& V : *SevArr)
            {
                FString S;
                if (V.IsValid() && V->TryGetString(S) && !S.IsEmpty())
                {
                    SeverityFilter.Add(S);
                }
            }
        }
    }

    TArray<FString> Tail;
    GLogRing.GetTail(LineCount, Tail, SeverityFilter.Num() > 0 ? &SeverityFilter : nullptr);

    TArray<TSharedPtr<FJsonValue>> Out;
    for (const FString& L : Tail) Out.Add(MakeShared<FJsonValueString>(L));

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetArrayField(TEXT("lines"), Out);
    Data->SetNumberField(TEXT("count"), Out.Num());
    Data->SetNumberField(TEXT("ring_capacity"), FInMemoryLogRing::MaxLines);
    Data->SetStringField(TEXT("source"), TEXT("in_memory_ring (FOutputDevice subscriber registered at PIECommands construction)"));
    return FSpirrowBridgeCommonUtils::CreateSuccessResponse(Data);
}
