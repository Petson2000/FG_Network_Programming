#pragma once

#include "GameFrameWork/Pawn.h"
#include "FGPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFGMovementComponent;
class UStaticMeshComponent;
class USphereComponent;
class UFGPlayerSettings;
class UFGNetDebugWidget;

UCLASS()
class FGNET_API AFGPlayer : public APawn
{
	GENERATED_BODY()

public:
	AFGPlayer();


public:

	UPROPERTY(EditAnywhere, Category = Settings)
	UFGPlayerSettings* PlayerSettings = nullptr;
	
	UPROPERTY(EditAnywhere, Category = Debug)
	TSubclassOf<UFGNetDebugWidget> DebugMenuClass;

	UFUNCTION(BlueprintPure)
	bool IsBraking() const { return bBrake; }

	UFUNCTION(BlueprintPure)
	int32 GetPing() const;

protected:

	virtual void BeginPlay();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Unreliable)
	void Server_SendLocation(const FVector& LocationToSend, float DeltaTime);

	UFUNCTION(Server, Unreliable)
	void Server_SendRotation(const FRotator& LocationToSend, float DeltaTime);
	
	UFUNCTION(Server, Unreliable)
	void Server_SendYaw(float NewYaw);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SendLocation(const FVector& LocationToSend, float DeltaTime);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SendRotation(const FRotator& RotationToSend, float DeltaTime);

	void ShowDebugMenu();
	void HideDebugMenu();

private:

	void CreateDebugWidget();
	void Handle_Acceleration(float Value);
	void Handle_Turn(float Value);
	void Handle_BrakePressed();
	void Handle_BrakeReleased();
	void Handle_DebugMenuPressed();

private:

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

	UPROPERTY(Transient)
	UFGNetDebugWidget* DebugMenuInstance = nullptr;

	bool bShowDebugMenu = false;

	FVector prevPingedLocation = FVector::ZeroVector;

	FRotator prevPingedRotation = FRotator::ZeroRotator;

	float PrevPingedTime = 0.0f;

	const float TransitionTime = 2.5f;

	UPROPERTY(Replicated)
	float ReplicatedYaw = 0.0f;

	UPROPERTY(Replicated)
	FVector ReplicatedLocation = FVector::ZeroVector;

	float Forward = 0.0f;

	float Turn = 0.0f;

	float MovementVelocity = 0.0f;

	float Yaw = 0.0f;

	bool bBrake = false;
};
