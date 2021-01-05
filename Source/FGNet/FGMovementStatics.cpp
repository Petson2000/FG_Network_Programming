#include "FGMovementStatics.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

FFGFrameMovement::FFGFrameMovement(AActor* InActor)
{
	StartLocation = InActor->GetActorLocation();
}

FFGFrameMovement::FFGFrameMovement(USceneComponent* InSceneComponent)
{
	StartLocation = InSceneComponent->GetComponentLocation();
}

void FFGFrameMovement::AddDelta(const FVector& InDelta)
{
	MovementDelta += InDelta;
}

bool FFGFrameMovement::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	return false;
}

void FFGFrameMovement::SerializeCompressed(FArchive& Ar)
{
	const bool bArLoading = Ar.IsLoading();

	uint8 ByteYaw = 0;

	if (!bArLoading)
	{
		ByteYaw = FRotator::CompressAxisToByte(Yaw);
	}

	uint8 B = (ByteYaw != 0);
	Ar.SerializeBits(&B, 1);

	if (B)
	{
		Ar << ByteYaw;
	}

	else
	{
		ByteYaw = 0;
	}

	if (bArLoading)
	{
		Yaw = FRotator::DecompressAxisFromByte(ByteYaw);
	}
}
