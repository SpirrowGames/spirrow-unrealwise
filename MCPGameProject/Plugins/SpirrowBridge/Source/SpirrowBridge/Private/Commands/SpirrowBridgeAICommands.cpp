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

// Phase G: BT Node Operation includes
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"

// Composites
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"

// Tasks
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_MoveDirectlyToward.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Tasks/BTTask_WaitBlackboardTime.h"
#include "BehaviorTree/Tasks/BTTask_PlaySound.h"
#include "BehaviorTree/Tasks/BTTask_PlayAnimation.h"
#include "BehaviorTree/Tasks/BTTask_RotateToFaceBBEntry.h"
#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"
#include "BehaviorTree/Tasks/BTTask_RunBehaviorDynamic.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"

// Decorators
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Decorators/BTDecorator_CompareBBEntries.h"
#include "BehaviorTree/Decorators/BTDecorator_ConditionalLoop.h"
#include "BehaviorTree/Decorators/BTDecorator_ConeCheck.h"
#include "BehaviorTree/Decorators/BTDecorator_Cooldown.h"
#include "BehaviorTree/Decorators/BTDecorator_DoesPathExist.h"
#include "BehaviorTree/Decorators/BTDecorator_ForceSuccess.h"
#include "BehaviorTree/Decorators/BTDecorator_IsAtLocation.h"
#include "BehaviorTree/Decorators/BTDecorator_KeepInCone.h"
#include "BehaviorTree/Decorators/BTDecorator_Loop.h"
#include "BehaviorTree/Decorators/BTDecorator_TagCooldown.h"
#include "BehaviorTree/Decorators/BTDecorator_TimeLimit.h"
#include "BehaviorTree/Decorators/BTDecorator_BlueprintBase.h"

// Services
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BehaviorTree/Services/BTService_DefaultFocus.h"
#include "BehaviorTree/Services/BTService_RunEQS.h"
#include "BehaviorTree/Services/BTService_BlueprintBase.h"

// Asset creation includes
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"
#include "Engine/Blueprint.h"

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
	// Phase G: BT Node Operation commands
	else if (CommandType == TEXT("add_bt_composite_node"))
	{
		return HandleAddBTCompositeNode(Params);
	}
	else if (CommandType == TEXT("add_bt_task_node"))
	{
		return HandleAddBTTaskNode(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_node"))
	{
		return HandleAddBTDecoratorNode(Params);
	}
	else if (CommandType == TEXT("add_bt_service_node"))
	{
		return HandleAddBTServiceNode(Params);
	}
	else if (CommandType == TEXT("connect_bt_nodes"))
	{
		return HandleConnectBTNodes(Params);
	}
	else if (CommandType == TEXT("set_bt_node_property"))
	{
		return HandleSetBTNodeProperty(Params);
	}
	else if (CommandType == TEXT("delete_bt_node"))
	{
		return HandleDeleteBTNode(Params);
	}
	else if (CommandType == TEXT("list_bt_node_types"))
	{
		return HandleListBTNodeTypes(Params);
	}

	return FSpirrowBridgeCommonUtils::CreateErrorResponse(
		ESpirrowErrorCode::UnknownCommand,
		FString::Printf(TEXT("Unknown AI command: %s"), *CommandType));
}
