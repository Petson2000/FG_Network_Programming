#pragma once

#include "GameFramework/Pawn.h"
#include "FGPlayer.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UFGMovementComponent;
class UStaticMeshComponent;
class USphereComponent;
class UFGPlayerSettings;
class UFGNetDebugWidget;
class AFGPickup;
class UFGRocket;

USTRUCT()
struct FGMovementData
{
	GENERATED_USTRUCT_BODY()

	float Yaw = 0.0f;

	bool NetSerialize(FArchive& Ar, class UPackageMap* PackageMap, bool& bOutSuccess)
	{
		SerializeCompressed(Ar);
		FRotator Rot;
		bOutSuccess = true;
		return true;
	}

private:

	void SerializeCompressed(FArchive& Ar)
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
};

template<>
struct TStructOpsTypeTraits<FGMovementData> : public TStructOpsTypeTraitsBase2<FGMovementData>
{
	enum
	{
		WithNetSerializer = true,
	};
};

UCLASS()
class FGNET_API AFGPlayer : public APawn
{
	GENERATED_BODY()

public:
	AFGPlayer();

	virtual void BeginPlay();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = Settings)
	UFGPlayerSettings* PlayerSettings = nullptr;
	
	UPROPERTY(EditAnywhere, Category = Debug)
	TSubclassOf<UFGNetDebugWidget> DebugMenuClass;

	UFUNCTION(BlueprintPure)
	bool IsBraking() const { return bBrake; }

	UFUNCTION(BlueprintPure)
	int32 GetPing() const;

	void OnPickup(AFGPickup* Pickup);

	void OnTakeDamage(float DamageAmount);

	UFUNCTION(Server, Reliable)
	void Server_OnPickup(AFGPickup* Pickup);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void OnHit(float DamageAmount);

	UFUNCTION(Client, Reliable)
	void Client_OnPickupRockets(int32 PickedUpRockets);

	UFUNCTION(Server, Reliable)
	void Server_OnHealthChanged(float DamageAmount);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnHealthChanged(float DamageAmount);

	UFUNCTION(Server, Unreliable)
	void Server_SendYaw(float NewYaw);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnNumRocketsChanged(int32 NewRocketAmount);

	UFUNCTION(BlueprintPure)
	int32 GetNumRockets() const { return NumRockets; }

	UFUNCTION(BlueprintImplementableEvent, Category = Player, meta = (DisplayName = "On Num Rockets Changed"))
	void BP_OnNumRocketsChanged(int32 NewNumRockets);

	UFUNCTION(BlueprintImplementableEvent, Category = Player, meta = (DisplayName = "On Health Changed"))
	void BP_OnHealthChanged(float NewHealth);

	UFUNCTION(BlueprintCallable)
	void Cheat_DecreaseHealthOnPlayer();

	void ShowDebugMenu();

	void HideDebugMenu();

	int32 GetNumActiveRockets() const;

	void FireRocket();

	void SpawnRockets();

	void HandleInvalidPickUp(AFGPickup* Pickup);

public:

	UPROPERTY(Replicated)
	float CurrentHealth = 0.0f;

	UPROPERTY(Replicated, /*Transient,*/ BlueprintReadWrite, Category = Weapon)
	TArray<UFGRocket*> RocketInstances;

private:

	void AddMovementVelocity(float DeltaTime);

	void CreateDebugWidget();

	void Handle_Acceleration(float Value);
	void Handle_Turn(float Value);
	void Handle_BrakePressed();
	void Handle_BrakeReleased();
	void Handle_DebugMenuPressed();
	void Handle_FirePressed();

	FVector GetRocketStartLocation() const;
	UFGRocket* GetFreeRocket() const;

	UFUNCTION(Server, Reliable)
	void Server_FireRocket(UFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_FireRocket(UFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation);
	
	UFUNCTION(Client, Reliable)
	void Client_RemoveRocket(UFGRocket* RocketToRemove);
	
	UFUNCTION(BlueprintCallable)
	void Cheat_IncreaseRockets(int32 InNumRockets);

	UFUNCTION(Server, Unreliable)
	void Server_SendMovement(const FVector& ClientLocation, float TimeStamp, float ClientForward, float ClientYaw, FGMovementData MovementData);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SendMovement(const FVector& InClientLocation, float TimeStamp, float ClientForward, float ClientYaw, FGMovementData MovementData);
	

private:
	
	UPROPERTY(EditAnywhere, Category = Weapon)
	TSubclassOf<UFGRocket> RocketClass;

	UPROPERTY(EditAnywhere, Category = Weapon)
	bool bSpawnWithRockets = true;

	int32 MaxActiveRockets = 50;

	float FireCooldownElapsed = 0.0f;

	UPROPERTY(EditAnywhere, Category = Weapon)
	bool bUnlimitedRockets = false;

	float ClientTimeStamp = 0.0f;
	float LastCorrectionDelta = 0.0f;
	float ServerTimeStamp = 0.0f;

	UPROPERTY(EditAnywhere, Category = Network)
	bool bPerformNetworkSmoothing = true;

	float MaxMeshDistanceFromPlayer = 5.0f;

	FVector OriginalMeshOffset = FVector::ZeroVector;

	int32 ServerNumRockets = 0;

	int32 NumRockets = 0;

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
