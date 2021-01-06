#pragma once

class AActor;
class USceneComponent;

struct FFGFrameMovement
{
	FFGFrameMovement(const FVector& InStartLocation) :
		StartLocation(InStartLocation) {}

	FFGFrameMovement(AActor* InActor);

	FFGFrameMovement(USceneComponent* InSceneComponent);

	void AddDelta(const FVector& InDelta);

	FVector GetMovementDelta() const { return MovementDelta; }

	FHitResult Hit;

	FVector FinalLocation = FVector::ZeroVector;

	float Yaw = 0.0f;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

private:

	void SerializeCompressed(FArchive& Ar);

	FVector MovementDelta = FVector::ZeroVector;
	FVector StartLocation = FVector::ZeroVector;

};