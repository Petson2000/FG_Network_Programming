#include "ShootingComponent.h"
#include "FGNet/FGRocket.h"
#include <Runtime/Engine/Public/Net/UnrealNetwork.h>

UShootingComponent::UShootingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UShootingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UShootingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UShootingComponent::SpawnRockets(FActorSpawnParameters SpawnParams)
{
	const int32 RocketCache = 8;

	for (int32 Index = 0; Index < RocketCache; Index++)
	{
		AFGRocket* NewRocketInstance = GetWorld()->SpawnActor<AFGRocket>(RocketClass, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation(), SpawnParams);
		RocketInstances.Add(NewRocketInstance);
	}
}

void UShootingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UShootingComponent, RocketInstances);
}
