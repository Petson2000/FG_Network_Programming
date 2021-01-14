// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FGRocket.generated.h"

class AFGPlayer;

UCLASS()
class FGNET_API UFGRocket : public UStaticMeshComponent
{
	GENERATED_BODY()
	
public:	
	UFGRocket();

	virtual void BeginPlay() override;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartMoving(const FVector& Forward, const FVector& InStartLocation);

	void ApplyCorrection(const FVector& Forward);

	bool IsFree() const { return bIsFree; }

	void Explode();

	void MakeFree();

public:
	UPROPERTY(EditAnywhere, Category = Damage, meta = (ClampMin = 1.0f))
	float DamageAmount = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
	UStaticMeshComponent* MeshComponent = nullptr;

private:
	void SetRocketVisibility(bool bVisible);

private:
	FCollisionQueryParams CachedCollisionQueryParams;

	UPROPERTY(EditAnywhere, Category = VFX)
	UParticleSystem* Explosion = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Root)
	USceneComponent* RootComponent;

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
