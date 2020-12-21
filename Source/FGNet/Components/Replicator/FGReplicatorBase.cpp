#include "FGReplicatorBase.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"

int32 UFGReplicatorBase::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	AActor* OwnerActor = CastChecked<AActor>(GetOuter(), ECastCheckedType::NullAllowed);
	return (OwnerActor ? OwnerActor->GetFunctionCallspace(Function, Stack) : FunctionCallspace::Local);
}

bool UFGReplicatorBase::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	check(GetOuter() != nullptr);

	AActor* OwnerActor = CastChecked<AActor>(GetOuter());

	bool bProcessed = false;

	FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());

	if (Context != nullptr)
	{
		for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
		{
			if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(OwnerActor, Function))
			{
				Driver.NetDriver->ProcessRemoteFunction(OwnerActor, Function, Parameters, OutParms, Stack, this);
				bProcessed = true;
			}
		}
	}

	return bProcessed;
}

bool UFGReplicatorBase::IsSupportedForNetworking() const
{
	return true;
}

bool UFGReplicatorBase::IsNameStableForNetworking() const
{
	return true;
}

void UFGReplicatorBase::Tick(float DeltaTime)
{

}

bool UFGReplicatorBase::IsTickable() const
{
	return bShouldTick;
}

TStatId UFGReplicatorBase::GetStatId() const
{
	return UObject::GetStatID();
}

bool UFGReplicatorBase::IsLocallyControlled() const
{
	if (!ensure(GetOuter() != nullptr))
	{
		return false;
	}

	if (const APawn* PawnOuter = Cast<APawn>(GetOuter()))
	{
		return PawnOuter->IsLocallyControlled();
	}

	const AActor* ActorOuter = CastChecked<AActor>(GetOuter(), ECastCheckedType::NullChecked);
	return ActorOuter && ActorOuter->HasAuthority();
}

bool UFGReplicatorBase::HasAuthority() const
{
	if (!ensure(GetOuter() != nullptr))
	{
		return false;
	}

	const AActor* ActorOuter = CastChecked<AActor>(GetOuter(), ECastCheckedType::NullChecked);
	return ActorOuter && ActorOuter->HasAuthority();
}