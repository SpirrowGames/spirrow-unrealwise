#include "Commands/SpirrowBridgeAICommands.h"
#include "Commands/SpirrowBridgeCommonUtils.h"

// Phase G: BT Node Operation includes
#include "BehaviorTree/BehaviorTree.h"
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

// Asset management
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"

// ===== Phase G: BT Node Operation Helper Functions =====

UClass* FSpirrowBridgeAICommands::GetBTCompositeNodeClass(const FString& TypeString)
{
	if (TypeString == TEXT("Selector")) return UBTComposite_Selector::StaticClass();
	if (TypeString == TEXT("Sequence")) return UBTComposite_Sequence::StaticClass();
	if (TypeString == TEXT("SimpleParallel")) return UBTComposite_SimpleParallel::StaticClass();
	return nullptr;
}

UClass* FSpirrowBridgeAICommands::GetBTTaskNodeClass(const FString& TypeString)
{
	if (TypeString == TEXT("BTTask_MoveTo")) return UBTTask_MoveTo::StaticClass();
	if (TypeString == TEXT("BTTask_MoveDirectlyToward")) return UBTTask_MoveDirectlyToward::StaticClass();
	if (TypeString == TEXT("BTTask_Wait")) return UBTTask_Wait::StaticClass();
	if (TypeString == TEXT("BTTask_WaitBlackboardTime")) return UBTTask_WaitBlackboardTime::StaticClass();
	if (TypeString == TEXT("BTTask_PlaySound")) return UBTTask_PlaySound::StaticClass();
	if (TypeString == TEXT("BTTask_PlayAnimation")) return UBTTask_PlayAnimation::StaticClass();
	if (TypeString == TEXT("BTTask_RotateToFaceBBEntry")) return UBTTask_RotateToFaceBBEntry::StaticClass();
	if (TypeString == TEXT("BTTask_RunBehavior")) return UBTTask_RunBehavior::StaticClass();
	if (TypeString == TEXT("BTTask_RunBehaviorDynamic")) return UBTTask_RunBehaviorDynamic::StaticClass();

	// Custom BP task search
	FString BlueprintPath = FString::Printf(TEXT("/Game/AI/Tasks/%s.%s"), *TypeString, *TypeString);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (Blueprint && Blueprint->GeneratedClass)
	{
		return Blueprint->GeneratedClass;
	}

	return nullptr;
}

UClass* FSpirrowBridgeAICommands::GetBTDecoratorClass(const FString& TypeString)
{
	if (TypeString == TEXT("BTDecorator_Blackboard")) return UBTDecorator_Blackboard::StaticClass();
	if (TypeString == TEXT("BTDecorator_CompareBBEntries")) return UBTDecorator_CompareBBEntries::StaticClass();
	if (TypeString == TEXT("BTDecorator_ConditionalLoop")) return UBTDecorator_ConditionalLoop::StaticClass();
	if (TypeString == TEXT("BTDecorator_ConeCheck")) return UBTDecorator_ConeCheck::StaticClass();
	if (TypeString == TEXT("BTDecorator_Cooldown")) return UBTDecorator_Cooldown::StaticClass();
	if (TypeString == TEXT("BTDecorator_DoesPathExist")) return UBTDecorator_DoesPathExist::StaticClass();
	if (TypeString == TEXT("BTDecorator_ForceSuccess")) return UBTDecorator_ForceSuccess::StaticClass();
	if (TypeString == TEXT("BTDecorator_IsAtLocation")) return UBTDecorator_IsAtLocation::StaticClass();
	if (TypeString == TEXT("BTDecorator_KeepInCone")) return UBTDecorator_KeepInCone::StaticClass();
	if (TypeString == TEXT("BTDecorator_Loop")) return UBTDecorator_Loop::StaticClass();
	if (TypeString == TEXT("BTDecorator_TagCooldown")) return UBTDecorator_TagCooldown::StaticClass();
	if (TypeString == TEXT("BTDecorator_TimeLimit")) return UBTDecorator_TimeLimit::StaticClass();

	// Custom BP decorator search
	FString BlueprintPath = FString::Printf(TEXT("/Game/AI/Decorators/%s.%s"), *TypeString, *TypeString);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (Blueprint && Blueprint->GeneratedClass)
	{
		return Blueprint->GeneratedClass;
	}

	return nullptr;
}

UClass* FSpirrowBridgeAICommands::GetBTServiceClass(const FString& TypeString)
{
	if (TypeString == TEXT("BTService_BlackboardBase")) return UBTService_BlackboardBase::StaticClass();
	if (TypeString == TEXT("BTService_DefaultFocus")) return UBTService_DefaultFocus::StaticClass();
	if (TypeString == TEXT("BTService_RunEQS")) return UBTService_RunEQS::StaticClass();

	// Custom BP service search
	FString BlueprintPath = FString::Printf(TEXT("/Game/AI/Services/%s.%s"), *TypeString, *TypeString);
	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (Blueprint && Blueprint->GeneratedClass)
	{
		return Blueprint->GeneratedClass;
	}

	return nullptr;
}

TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::BTNodeToJson(UBTNode* Node)
{
	TSharedPtr<FJsonObject> NodeJson = MakeShareable(new FJsonObject());

	if (!Node) return NodeJson;

	NodeJson->SetStringField(TEXT("node_id"), Node->GetName());
	NodeJson->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
	NodeJson->SetStringField(TEXT("node_name"), Node->NodeName.IsEmpty() ? Node->GetClass()->GetName() : Node->NodeName);

	return NodeJson;
}

UBTNode* FSpirrowBridgeAICommands::FindBTNodeById(UBehaviorTree* BehaviorTree, const FString& NodeId)
{
	if (!BehaviorTree || !BehaviorTree->RootNode) return nullptr;

	// Recursive node search
	TFunction<UBTNode*(UBTCompositeNode*)> SearchInComposite = [&](UBTCompositeNode* Composite) -> UBTNode*
	{
		if (!Composite) return nullptr;

		if (Composite->GetName() == NodeId)
		{
			return Composite;
		}

		// Search decorators (in children, since UE 5.6+ stores decorators per child)
		for (const FBTCompositeChild& Child : Composite->Children)
		{
			for (UBTDecorator* Decorator : Child.Decorators)
			{
				if (Decorator && Decorator->GetName() == NodeId)
				{
					return Decorator;
				}
			}
		}

		// Search services
		for (UBTService* Service : Composite->Services)
		{
			if (Service && Service->GetName() == NodeId)
			{
				return Service;
			}
		}

		// Search child nodes
		for (const FBTCompositeChild& Child : Composite->Children)
		{
			if (Child.ChildComposite)
			{
				if (UBTNode* Found = SearchInComposite(Child.ChildComposite))
				{
					return Found;
				}
			}
			if (Child.ChildTask)
			{
				if (Child.ChildTask->GetName() == NodeId)
				{
					return Child.ChildTask;
				}
				// Task decorators (stored in FBTCompositeChild, not on the task itself)
				for (UBTDecorator* Decorator : Child.Decorators)
				{
					if (Decorator && Decorator->GetName() == NodeId)
					{
						return Decorator;
					}
				}
			}
		}

		return nullptr;
	};

	return SearchInComposite(BehaviorTree->RootNode);
}

FString FSpirrowBridgeAICommands::GetCompositeDescription(const FString& Type)
{
	if (Type == TEXT("Selector")) return TEXT("Tries children in order until one succeeds");
	if (Type == TEXT("Sequence")) return TEXT("Runs children in order until one fails");
	if (Type == TEXT("SimpleParallel")) return TEXT("Runs main task with background tasks");
	return TEXT("");
}

FString FSpirrowBridgeAICommands::GetTaskDescription(const FString& Type)
{
	if (Type == TEXT("BTTask_MoveTo")) return TEXT("Move to a location or actor");
	if (Type == TEXT("BTTask_MoveDirectlyToward")) return TEXT("Move directly toward target");
	if (Type == TEXT("BTTask_Wait")) return TEXT("Wait for specified seconds");
	if (Type == TEXT("BTTask_WaitBlackboardTime")) return TEXT("Wait using blackboard time value");
	if (Type == TEXT("BTTask_PlaySound")) return TEXT("Play a sound");
	if (Type == TEXT("BTTask_PlayAnimation")) return TEXT("Play an animation");
	if (Type == TEXT("BTTask_RotateToFaceBBEntry")) return TEXT("Rotate to face blackboard entry");
	if (Type == TEXT("BTTask_RunBehavior")) return TEXT("Run a sub-BehaviorTree");
	if (Type == TEXT("BTTask_RunBehaviorDynamic")) return TEXT("Run a dynamic sub-BehaviorTree");
	return TEXT("");
}

FString FSpirrowBridgeAICommands::GetDecoratorDescription(const FString& Type)
{
	if (Type == TEXT("BTDecorator_Blackboard")) return TEXT("Check blackboard value condition");
	if (Type == TEXT("BTDecorator_CompareBBEntries")) return TEXT("Compare two blackboard entries");
	if (Type == TEXT("BTDecorator_Cooldown")) return TEXT("Limit execution frequency");
	if (Type == TEXT("BTDecorator_DoesPathExist")) return TEXT("Check if path exists to target");
	if (Type == TEXT("BTDecorator_ForceSuccess")) return TEXT("Force child to return success");
	if (Type == TEXT("BTDecorator_IsAtLocation")) return TEXT("Check if at location");
	if (Type == TEXT("BTDecorator_Loop")) return TEXT("Loop child execution");
	if (Type == TEXT("BTDecorator_TagCooldown")) return TEXT("Gameplay tag based cooldown");
	if (Type == TEXT("BTDecorator_TimeLimit")) return TEXT("Set time limit for child");
	return TEXT("");
}

FString FSpirrowBridgeAICommands::GetServiceDescription(const FString& Type)
{
	if (Type == TEXT("BTService_DefaultFocus")) return TEXT("Set AI focus target");
	if (Type == TEXT("BTService_RunEQS")) return TEXT("Run Environment Query");
	if (Type == TEXT("BTService_BlackboardBase")) return TEXT("Base class for blackboard updates");
	return TEXT("");
}
