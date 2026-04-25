#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for PIE (Play-In-Editor) lifecycle, runtime introspection,
 * camera control, console exec, and log access MCP commands.
 *
 * Most handlers require GEditor->PlayWorld != nullptr; the few that do not
 * (logs, console exec on editor world) are documented inline.
 */
class SPIRROWBRIDGE_API FSpirrowBridgePIECommands
{
public:
    FSpirrowBridgePIECommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // PIE lifecycle
    TSharedPtr<FJsonObject> HandleStartPIE(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleStopPIE(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetPIEState(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandlePausePIE(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleResumePIE(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleStepPIEFrames(const TSharedPtr<FJsonObject>& Params);

    // Camera + screenshot (PIE viewport)
    TSharedPtr<FJsonObject> HandleTakePIEScreenshot(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleTakeHighResScreenshot(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetPIECamera(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetPIECamera(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleEnableDebugCam(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDisableDebugCam(const TSharedPtr<FJsonObject>& Params);

    // Console exec + runtime control
    TSharedPtr<FJsonObject> HandleExecConsoleCommand(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetGlobalTimeDilation(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSimulatePIEInput(const TSharedPtr<FJsonObject>& Params);

    // PIE world introspection
    TSharedPtr<FJsonObject> HandleGetPIEActors(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFindPIEActorsByClass(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetPIEActorProperties(const TSharedPtr<FJsonObject>& Params);

    // Log access
    TSharedPtr<FJsonObject> HandleTailUELog(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFilterUELog(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetLogVerbosity(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetUELogPath(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleScanUELogErrors(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSearchUELog(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleTailEditorOutputLog(const TSharedPtr<FJsonObject>& Params);
};
