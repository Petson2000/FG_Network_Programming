// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FGRocket.generated.h"

class AFGPlayer;

UCLASS()
class FGNET_API AFGRocket : public AActor
{
	GENERATED_BODY()
	
public:	
	AFGRocket();

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	void StartMoving(const FVector& Forward, const FVector& InStartLocation);

	void ApplyCorrection(const FVector& Forward);

	bool IsFree() const { return bIsFree; }

	void Explode();

	void MakeFree();

public:

	UPROPERTY(EditAnywhere, Category = Damage, meta = (ClampMin = 1.0f))
	float DamageAmount = 2.0f;


private:

	void SetRocketVisibility(bool bVisible);

private:

	FCollisionQueryParams CachedCollisionQueryParams;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystem* Explosion = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	UStaticMeshComponent* MeshComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = Debug)
	bool bDebugDrawCorrection = true;

	FVector OriginalFacingDirection = FVector::ZeroVector;

	FVector FacingRotationStart = FVector::ZeroVector;

	FQuat FacingRotationCorrection = FQuat::Identity;

	FVector RocketStartLocation = FVector::ZeroVector;


	float LifeTime = 2.0f;
	float LifeTimeElapsed = 0.0f;

	float DistanceMoved = 0.0f;

	UPROPERTY(EditAnywhere)
	float MovementVelocity = 1300.0f;

	bool bIsFree = true;
};
