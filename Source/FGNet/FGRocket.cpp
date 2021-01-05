#include "FGRocket.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "FGNet/Player/FGPlayer.h"

UFGRocket::UFGRocket()
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneCompRoot"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));

	SetUsingAbsoluteLocation(true);
	SetIsReplicatedByDefault(true);
}

void UFGRocket::BeginPlay()
{
	Super::BeginPlay();
	CachedCollisionQueryParams.AddIgnoredComponent(this);
	SetRocketVisibility(false);
}

void UFGRocket::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	LifeTimeElapsed -= DeltaTime;
	DistanceMoved += MovementVelocity * DeltaTime;

	FacingRotationStart = FQuat::Slerp(FacingRotationStart.ToOrientationQuat(), FacingRotationCorrection, 0.9f * DeltaTime).Vector(); 

#if !UE_BUILD_SHIPPING

	if (bDebugDrawCorrection)
	{
		const float ArrowLength = 3000.0f;
		const float ArrowSize = 50.0f;

		DrawDebugDirectionalArrow(GetWorld(), RocketStartLocation, RocketStartLocation + OriginalFacingDirection * ArrowLength, ArrowSize, FColor::Red);
		DrawDebugDirectionalArrow(GetWorld(), RocketStartLocation, RocketStartLocation + FacingRotationStart * ArrowLength, ArrowSize, FColor::Green);
	}

#endif // !UE_BUILD_SHIPPING

	const FVector NewLocation = RocketStartLocation + FacingRotationStart * DistanceMoved;

	SetWorldLocation(NewLocation);
	FHitResult Hit;

	const FVector StartLocation = NewLocation;
	const FVector EndLocation = StartLocation + FacingRotationStart * 100.0f;
	GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, CachedCollisionQueryParams);

	if (Cast<AFGPlayer>(Hit.Actor))
	{
		AFGPlayer* HitPlayer;
		HitPlayer = Cast<AFGPlayer>(Hit.Actor);
		HitPlayer->OnHit(DamageAmount);
		Explode();
		return;
	}

	if (Hit.bBlockingHit)
	{
		Explode();
	}

	if (LifeTimeElapsed < 0.0f)
	{
		Explode();
	}
}

void UFGRocket::StartMoving(const FVector& Forward, const FVector& InStartLocation)
{
	FacingRotationStart = Forward;
	FacingRotationCorrection = FacingRotationStart.ToOrientationQuat();
	RocketStartLocation = InStartLocation;

	SetWorldLocationAndRotation(InStartLocation, Forward.Rotation());

	//SetRelativeLocation(InStartLocation);
	//SetRelativeRotation(Forward.Rotation());

	bIsFree = false;
	SetComponentTickEnabled(true);
	//SetRocketVisibility(true);
	LifeTimeElapsed = LifeTime;
	DistanceMoved = 0.0f;
	OriginalFacingDirection = FacingRotationStart;
}

void UFGRocket::ApplyCorrection(const FVector& Forward)
{
	FacingRotationCorrection = Forward.ToOrientationQuat();
}

void UFGRocket::Explode()
{
	if (Explosion != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Explosion, GetRelativeLocation(), GetRelativeRotation(), true);
	}

	MakeFree();
}

void UFGRocket::MakeFree()
{
	//Disable Mesh
	bIsFree = true;
	SetComponentTickEnabled(false);
	//SetRocketVisibility(false);
}

void UFGRocket::SetRocketVisibility(bool bIsVisible)
{
	RootComponent->SetVisibility(bIsVisible, true);
}
