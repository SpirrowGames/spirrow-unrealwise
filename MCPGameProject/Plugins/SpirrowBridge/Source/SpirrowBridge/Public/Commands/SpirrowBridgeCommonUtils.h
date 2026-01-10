#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class AActor;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_Event;
class UK2Node_CallFunction;
class UK2Node_VariableGet;
class UK2Node_VariableSet;
class UK2Node_InputAction;
class UK2Node_Self;
class UFunction;
class UWidgetBlueprint;

/**
 * Error codes for SpirrowBridge operations
 */
namespace ESpirrowErrorCode
{
    // General errors (1000-1099)
    constexpr int32 Success = 0;
    constexpr int32 UnknownError = 1000;
    constexpr int32 UnknownCommand = 1001;
    constexpr int32 InvalidParams = 1002;
    constexpr int32 MissingRequiredParam = 1003;
    constexpr int32 InvalidParamType = 1004;
    constexpr int32 InvalidParamValue = 1005;
    constexpr int32 InvalidParameter = 1006;
    constexpr int32 OperationFailed = 1007;
    constexpr int32 SystemError = 1008;

    // Asset errors (1100-1199)
    constexpr int32 AssetNotFound = 1100;
    constexpr int32 AssetLoadFailed = 1101;
    constexpr int32 AssetAlreadyExists = 1102;
    constexpr int32 AssetCreationFailed = 1103;
    constexpr int32 AssetDeleteFailed = 1104;
    constexpr int32 InvalidAssetPath = 1105;

    // Blueprint errors (1200-1299)
    constexpr int32 BlueprintNotFound = 1200;
    constexpr int32 BlueprintCompileFailed = 1201;
    constexpr int32 BlueprintInvalidClass = 1202;
    constexpr int32 EventGraphNotFound = 1203;
    constexpr int32 NodeCreationFailed = 1204;
    constexpr int32 NodeConnectionFailed = 1205;
    constexpr int32 PinNotFound = 1206;
    constexpr int32 VariableNotFound = 1207;
    constexpr int32 FunctionNotFound = 1208;
    constexpr int32 GraphNotFound = 1209;
    constexpr int32 NodeNotFound = 1210;
    constexpr int32 ClassNotFound = 1211;
    constexpr int32 InvalidOperation = 1212;
    constexpr int32 InvalidBlueprint = 1213;
    constexpr int32 InvalidComponent = 1214;
    constexpr int32 ConnectionFailed = 1215;
    constexpr int32 NotSupported = 1216;
    constexpr int32 NodeAlreadyExists = 1217;
    constexpr int32 PropertyTypeMismatch = 1218;

    // Widget errors (1300-1399)
    constexpr int32 WidgetNotFound = 1300;
    constexpr int32 WidgetElementNotFound = 1301;
    constexpr int32 WidgetCreationFailed = 1302;
    constexpr int32 WidgetTreeNotFound = 1303;
    constexpr int32 CanvasPanelNotFound = 1304;
    constexpr int32 AnimationNotFound = 1305;

    // Actor errors (1400-1499)
    constexpr int32 ActorNotFound = 1400;
    constexpr int32 ActorSpawnFailed = 1401;
    constexpr int32 ComponentNotFound = 1402;
    constexpr int32 PropertyNotFound = 1403;
    constexpr int32 PropertySetFailed = 1404;
    constexpr int32 ComponentCreationFailed = 1405;
    constexpr int32 InvalidActorType = 1406;

    // GAS errors (1500-1599)
    constexpr int32 GameplayTagInvalid = 1500;
    constexpr int32 GameplayEffectFailed = 1501;
    constexpr int32 GameplayAbilityFailed = 1502;

    // Config/File errors (1600-1699)
    constexpr int32 ConfigKeyNotFound = 1600;
    constexpr int32 FileWriteFailed = 1601;
    constexpr int32 FileReadFailed = 1602;
}

/**
 * Common utilities for SpirrowBridge commands
 */
class SPIRROWBRIDGE_API FSpirrowBridgeCommonUtils
{
public:
    // ============================================
    // JSON Response utilities
    // ============================================
    
    /** Create an error response with message only */
    static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& Message);
    
    /** Create an error response with error code and message */
    static TSharedPtr<FJsonObject> CreateErrorResponse(int32 ErrorCode, const FString& Message);
    
    /** Create an error response with error code, message, and additional details */
    static TSharedPtr<FJsonObject> CreateErrorResponse(int32 ErrorCode, const FString& Message, const TSharedPtr<FJsonObject>& Details);
    
    /** Create a success response with optional data */
    static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);
    
    // ============================================
    // Parameter Validation utilities
    // ============================================
    
    /** Validate required string parameter, returns error response if invalid */
    static TSharedPtr<FJsonObject> ValidateRequiredString(
        const TSharedPtr<FJsonObject>& Params, 
        const FString& ParamName, 
        FString& OutValue);
    
    /** Validate optional string parameter with default value */
    static void GetOptionalString(
        const TSharedPtr<FJsonObject>& Params, 
        const FString& ParamName, 
        FString& OutValue, 
        const FString& DefaultValue = TEXT(""));
    
    /** Validate required number parameter */
    static TSharedPtr<FJsonObject> ValidateRequiredNumber(
        const TSharedPtr<FJsonObject>& Params, 
        const FString& ParamName, 
        double& OutValue);
    
    /** Validate optional number parameter with default */
    static void GetOptionalNumber(
        const TSharedPtr<FJsonObject>& Params, 
        const FString& ParamName, 
        double& OutValue, 
        double DefaultValue = 0.0);
    
    /** Validate required boolean parameter */
    static TSharedPtr<FJsonObject> ValidateRequiredBool(
        const TSharedPtr<FJsonObject>& Params, 
        const FString& ParamName, 
        bool& OutValue);
    
    /** Validate optional boolean parameter with default */
    static void GetOptionalBool(
        const TSharedPtr<FJsonObject>& Params, 
        const FString& ParamName, 
        bool& OutValue, 
        bool DefaultValue = false);
    
    // ============================================
    // Asset Validation utilities
    // ============================================
    
    /** Validate Blueprint exists and return it, or error response */
    static TSharedPtr<FJsonObject> ValidateBlueprint(
        const FString& BlueprintName, 
        const FString& Path, 
        UBlueprint*& OutBlueprint);
    
    /** Validate Widget Blueprint exists and return it, or error response */
    static TSharedPtr<FJsonObject> ValidateWidgetBlueprint(
        const FString& WidgetName, 
        const FString& Path, 
        UWidgetBlueprint*& OutWidget);
    
    /** Validate asset path format */
    static bool IsValidAssetPath(const FString& Path, FString& OutErrorMessage);
    
    // ============================================
    // JSON Array utilities
    // ============================================
    static void GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray);
    static void GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray);
    static FVector2D GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static FVector GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static FRotator GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static FLinearColor GetLinearColorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, const FLinearColor& DefaultValue = FLinearColor::White);
    
    // ============================================
    // Actor utilities
    // ============================================
    static TSharedPtr<FJsonValue> ActorToJson(AActor* Actor);
    static TSharedPtr<FJsonObject> ActorToJsonObject(AActor* Actor, bool bDetailed = false);
    
    // ============================================
    // Blueprint utilities
    // ============================================
    static UBlueprint* FindBlueprint(const FString& BlueprintName, const FString& Path = TEXT("/Game/Blueprints"));
    static UBlueprint* FindBlueprintByName(const FString& BlueprintName, const FString& Path = TEXT("/Game/Blueprints"));
    static UEdGraph* FindOrCreateEventGraph(UBlueprint* Blueprint);
    
    // ============================================
    // Blueprint node utilities
    // ============================================
    static UK2Node_Event* CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position);
    static UK2Node_CallFunction* CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position);
    static UK2Node_VariableGet* CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position);
    static UK2Node_VariableSet* CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position);
    static UK2Node_InputAction* CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position);
    static UK2Node_Self* CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position);
    static bool ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                UEdGraphNode* TargetNode, const FString& TargetPinName);
    static UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction = EGPD_MAX);
    static UK2Node_Event* FindExistingEventNode(UEdGraph* Graph, const FString& EventName);

    // ============================================
    // Property utilities
    // ============================================
    static bool SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                 const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage);
    
    /** Set a struct property value from JSON (internal helper) */
    static bool SetStructPropertyValue(void* StructAddr, FStructProperty* StructProp,
                                       const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage);
    
    /** Set individual struct field value (recursive helper) */
    static bool SetStructFieldValue(void* StructAddr, UScriptStruct* ScriptStruct,
                                    const FString& FieldName, const TSharedPtr<FJsonValue>& Value,
                                    FString& OutErrorMessage);
    
    // ============================================
    // Logging utilities
    // ============================================
    
    /** Log an error with command context */
    static void LogCommandError(const FString& CommandName, const FString& Message);
    
    /** Log a warning with command context */
    static void LogCommandWarning(const FString& CommandName, const FString& Message);
    
    /** Log info with command context */
    static void LogCommandInfo(const FString& CommandName, const FString& Message);
};
