#include "FGReplicatorComponent.h"
#include "Engine/ActorChannel.h"
#include "FGReplicatorBase.h"

UFGReplicatorComponent::UFGReplicatorComponent()
{
	SetIsReplicatedByDefault(true);
}

bool UFGReplicatorComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	WroteSomething |= Channel->ReplicateSubobjectList(SmoothReplicators, *Bunch, *RepFlags);

	return WroteSomething;
}

UFGReplicatorBase* UFGReplicatorComponent::AddReplicatorByClass(TSubclassOf<UFGReplicatorBase> ClassType, FName Name)
{
	UFGReplicatorBase* NewReplicator = NewObject<UFGReplicatorBase>(GetOwner(), ClassType, Name);
	NewReplicator->Init();
	SmoothReplicators.Add(NewReplicator);
	return NewReplicator;
}