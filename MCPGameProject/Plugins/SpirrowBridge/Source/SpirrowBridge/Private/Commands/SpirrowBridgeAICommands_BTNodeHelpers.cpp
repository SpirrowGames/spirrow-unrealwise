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

// For C++ class search
#include "UObject/UObjectIterator.h"

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
	// 1. Standard tasks (hardcoded)
	if (TypeString == TEXT("BTTask_MoveTo")) return UBTTask_MoveTo::StaticClass();
	if (TypeString == TEXT("BTTask_MoveDirectlyToward")) return UBTTask_MoveDirectlyToward::StaticClass();
	if (TypeString == TEXT("BTTask_Wait")) return UBTTask_Wait::StaticClass();
	if (TypeString == TEXT("BTTask_WaitBlackboardTime")) return UBTTask_WaitBlackboardTime::StaticClass();
	if (TypeString == TEXT("BTTask_PlaySound")) return UBTTask_PlaySound::StaticClass();
	if (TypeString == TEXT("BTTask_PlayAnimation")) return UBTTask_PlayAnimation::StaticClass();
	if (TypeString == TEXT("BTTask_RotateToFaceBBEntry")) return UBTTask_RotateToFaceBBEntry::StaticClass();
	if (TypeString == TEXT("BTTask_RunBehavior")) return UBTTask_RunBehavior::StaticClass();
	if (TypeString == TEXT("BTTask_RunBehaviorDynamic")) return UBTTask_RunBehaviorDynamic::StaticClass();

	// 2. Full path specification (e.g., "/Script/TrapxTrapCpp.BTTask_ChaseTarget")
	if (TypeString.StartsWith(TEXT("/Script/")))
	{
		UClass* FoundClass = LoadClass<UBTTaskNode>(nullptr, *TypeString);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// 3. Class name only - search all modules
	{
		FString ClassNameWithU = TypeString.StartsWith(TEXT("U")) ? TypeString : FString::Printf(TEXT("U%s"), *TypeString);

		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (Class->GetName() == ClassNameWithU || Class->GetName() == TypeString)
			{
				if (Class->IsChildOf(UBTTaskNode::StaticClass()))
				{
					return Class;
				}
			}
		}
	}

	// 4. Custom BP task search (fallback)
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
	// 1. Standard decorators (hardcoded)
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

	// 2. Full path specification (e.g., "/Script/ProjectName.BTDecorator_Custom")
	if (TypeString.StartsWith(TEXT("/Script/")))
	{
		UClass* FoundClass = LoadClass<UBTDecorator>(nullptr, *TypeString);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// 3. Class name only - search all modules
	{
		FString ClassNameWithU = TypeString.StartsWith(TEXT("U")) ? TypeString : FString::Printf(TEXT("U%s"), *TypeString);

		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (Class->GetName() == ClassNameWithU || Class->GetName() == TypeString)
			{
				if (Class->IsChildOf(UBTDecorator::StaticClass()))
				{
					return Class;
				}
			}
		}
	}

	// 4. Custom BP decorator search (fallback)
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
	// 1. Standard services (hardcoded)
	if (TypeString == TEXT("BTService_BlackboardBase")) return UBTService_BlackboardBase::StaticClass();
	if (TypeString == TEXT("BTService_DefaultFocus")) return UBTService_DefaultFocus::StaticClass();
	if (TypeString == TEXT("BTService_RunEQS")) return UBTService_RunEQS::StaticClass();

	// 2. Full path specification (e.g., "/Script/ProjectName.BTService_Custom")
	if (TypeString.StartsWith(TEXT("/Script/")))
	{
		UClass* FoundClass = LoadClass<UBTService>(nullptr, *TypeString);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// 3. Class name only - search all modules
	{
		FString ClassNameWithU = TypeString.StartsWith(TEXT("U")) ? TypeString : FString::Printf(TEXT("U%s"), *TypeString);

		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (Class->GetName() == ClassNameWithU || Class->GetName() == TypeString)
			{
				if (Class->IsChildOf(UBTService::StaticClass()))
				{
					return Class;
				}
			}
		}
	}

	// 4. Custom BP service search (fallback)
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
	if (!BehaviorTree) return nullptr;

	// ツリー内を検索
	if (!BehaviorTree->RootNode) return nullptr;

	// RootNode自体をチェック
	if (BehaviorTree->RootNode->GetName() == NodeId)
	{
		return BehaviorTree->RootNode;
	}

	// 再帰的に検索
	TFunction<UBTNode*(UBTCompositeNode*)> SearchInComposite = [&](UBTCompositeNode* Composite) -> UBTNode*
	{
		if (!Composite) return nullptr;

		// サービスを検索
		for (UBTService* Service : Composite->Services)
		{
			if (Service && Service->GetName() == NodeId)
			{
				return Service;
			}
		}

		// 子を検索
		for (const FBTCompositeChild& Child : Composite->Children)
		{
			// デコレータを検索
			for (UBTDecorator* Decorator : Child.Decorators)
			{
				if (Decorator && Decorator->GetName() == NodeId)
				{
					return Decorator;
				}
			}

			// 子Compositeを検索
			if (Child.ChildComposite)
			{
				if (Child.ChildComposite->GetName() == NodeId)
				{
					return Child.ChildComposite;
				}
				if (UBTNode* Found = SearchInComposite(Child.ChildComposite))
				{
					return Found;
				}
			}

			// 子Taskを検索
			if (Child.ChildTask && Child.ChildTask->GetName() == NodeId)
			{
				return Child.ChildTask;
			}
		}

		return nullptr;
	};

	return SearchInComposite(BehaviorTree->RootNode);
}

void FSpirrowBridgeAICommands::RemoveNodeFromParent(UBehaviorTree* BehaviorTree, UBTNode* TargetNode)
{
	if (!BehaviorTree || !BehaviorTree->RootNode || !TargetNode)
	{
		return;
	}

	TFunction<bool(UBTCompositeNode*)> RemoveFromComposite = [&](UBTCompositeNode* Parent) -> bool
	{
		if (!Parent) return false;

		for (int32 i = Parent->Children.Num() - 1; i >= 0; --i)
		{
			FBTCompositeChild& Child = Parent->Children[i];
			if (Child.ChildComposite == TargetNode || Child.ChildTask == TargetNode)
			{
				Parent->Children.RemoveAt(i);
				return true;
			}

			// 再帰的に検索
			if (Child.ChildComposite && RemoveFromComposite(Child.ChildComposite))
			{
				return true;
			}
		}
		return false;
	};

	RemoveFromComposite(BehaviorTree->RootNode);
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
