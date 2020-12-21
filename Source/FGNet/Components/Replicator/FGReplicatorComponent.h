#pragma once

#include "Components/ActorComponent.h"
#include "FGReplicatorComponent.generated.h"

class UFGReplicatorBase;

UCLASS(meta = (BlueprintSpawnableComponent))

class FGNET_API UFGReplicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFGReplicatorComponent();

	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	//Create Replicator instance
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Smooth Replicator"))
	UFGReplicatorBase* AddReplicatorByClass(TSubclassOf<UFGReplicatorBase> ClassType, FName Name);

	template<typename ClassType>
	ClassType* AddReplicator(FName Name)
	{
		return CastChecked<ClassType>(AddReplicatorByClass(ClassType::StaticClass(), Name));
	}

private:

	UPROPERTY()
	TArray<UFGReplicatorBase*> SmoothReplicators;
};