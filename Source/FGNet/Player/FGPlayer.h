#pragma once

#include "GameFrameWork/Pawn.h"
#include "FGPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFGMovementComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class FGNET_API AFGPlayer : public APawn
{
	GENERATED_BODY()

public:
	AFGPlayer();

protected:

	virtual void BeginPlay();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Movement")
		float Acceleration = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
		float TurnSpeedDefault = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
		float MaxVelocity = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0.0, ClampMax = 1.0))
		float DefaultFriction = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0.0, ClampMax = 1.0))
		float BrakingFriction = 0.001f;

	UFUNCTION(BlueprintPure)
		bool IsBraking() const { return bBrake; }

	UFUNCTION(BlueprintPure)
		int32 GetPing() const;

	UFUNCTION(Server, Unreliable)
		void Server_SendLocation(const FVector& LocationToSend, float DeltaTime);

	UFUNCTION(Server, Unreliable)
		void Server_SendRotation(const FRotator& LocationToSend, float DeltaTime);

	UFUNCTION(NetMulticast, Unreliable)
		void Multicast_SendLocation(const FVector& LocationToSend, float DeltaTime);

	UFUNCTION(NetMulticast, Unreliable)
		void Multicast_SendRotation(const FRotator& RotationToSend, float DeltaTime);

private:
	void Handle_Acceleration(float Value);
	void Handle_Turn(float Value);
	void Handle_BrakePressed();
	void Handle_BrakeReleased();

	FVector PrevPingedLocation = FVector::ZeroVector;
	FRotator prevPingedRotation = FRotator::ZeroRotator;

	float PrevPingedTime = 0.0f;

	const float TransitionTime = 2.5f;

	float Forward = 0.0f;
	float Turn = 0.0f;

	float MovementVelocity = 0.0f;
	float Yaw = 0.0f;

	bool bBrake = false;

	UPROPERTY(VisibleDefaultsOnly, Category = "Collision")
		USphereComponent* CollisionComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Mesh")
		UStaticMeshComponent* MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Camera")
		USpringArmComponent* SpringArmComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Camera")
		UCameraComponent* CameraComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Movement")
		UFGMovementComponent* MovementComponent;
};
