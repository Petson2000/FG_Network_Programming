// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShootingComponent.generated.h"

class AFGRocket;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FGNET_API UShootingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UShootingComponent();

protected:

	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void FireRocket();

	void SpawnRockets(FActorSpawnParameters SpawnParams);

	FORCEINLINE int32 GetNumActiveRockets() { return NumRockets; }

	UFUNCTION(BlueprintPure)
	int32 GetNumRockets() const { return NumRockets; }


public:
	UPROPERTY(Replicated, Transient, BlueprintReadOnly)
	TArray<AFGRocket*> RocketInstances;

	UPROPERTY(EditAnywhere, Category = Weapon)
	TSubclassOf<AFGRocket> RocketClass;

private:

	int32 ServerNumRockets = 0;

	int32 NumRockets = 0;

	float FireCooldownElapsed = 0.0f;

	UPROPERTY(EditAnywhere, Category = Weapon)
	bool bUnlimitedRockets = false;
};
