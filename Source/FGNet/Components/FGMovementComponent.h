#pragma once

#include "GameFramework/MovementComponent.h"
#include "FGMovementComponent.generated.h"

struct FFGFrameMovement;

UCLASS()
class FGNET_API UFGMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
public:

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FFGFrameMovement CreateFrameMovement() const;

	void Move(FFGFrameMovement& FrameMovement);

	void ApplyGravity();

	UPROPERTY(EditAnywhere, Category = "Movement")
		float Gravity = 30.0f;

	FVector GetGravityAsVector() const { return FVector(0.0f, 0.0f, AccumulatedGravity); }
	FRotator GetFacingRotation() const { return FacingRotationCurrent; }
	FVector GetFacingDirection() const { return FacingRotationCurrent.Vector(); }

	void SetFacingRotation(const FRotator& InFacingRotation, float InRotationSpeed = -1.0f);
	void SetFacingRotation(const FQuat& InFacingRotation, float InRotationSpeed = -1.0f);
	void SetFacingDirection(const FVector& InFacingDirection, float InRotationSpeed = -1.0f);

private:

	void Internal_SetFacingRotation(const FRotator& InFacingRotation, float InRotationSpeed);
	FVector GetMovementDelta(const FFGFrameMovement& FrameMovement) const;

	FHitResult Hit;

	FRotator FacingRotationCurrent;
	FRotator FacingRotationTarget;
	float AccumulatedGravity;
	float FacingRotationSpeed = 1.0f;
};