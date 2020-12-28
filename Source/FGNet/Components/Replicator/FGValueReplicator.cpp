#include "FGValueReplicator.h"
#include "Net/UnrealNetwork.h"

void UFGValueReplicator::Tick(float DeltaTime)
{

}

void UFGValueReplicator::Init()
{
	bIsSleeping = true;
	bHasSentTerminalValue = true;
	bHasRevievedTerminalValue = true;
}

void UFGValueReplicator::SetValue(float InValue)
{

}

//float UFGValueReplicator::GetValue()
//{
//	return ReplicatedValueCurrent;
//}

//void UFGValueReplicator::BroadcastDelegate()
//{
//	if (OnValueChanged.IsBound())
//	{
//		OnValueChanged.Broadcast();
//	}
//}

void UFGValueReplicator::Server_SendTerminalValue_Implementation(int32 SyncTag, float TerminalValue)
{

}

void UFGValueReplicator::Server_SendReplicatedValue_Implementation(int32 SyncTag, float ReplicatedValue)
{

}

void UFGValueReplicator::Multicast_SendTerminalValue_Implementation(int32 SyncTag, float ReplicatedValue)
{

}

void UFGValueReplicator::Multicast_SendReplicatedValue_Implementation(int32 SyncTag, float ReplicatedValue)
{

}

bool UFGValueReplicator::ShouldTick() const 
{
	return true;
}