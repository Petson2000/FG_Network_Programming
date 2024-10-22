#include "FGPlayer.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerState.h"
#include "../Components/FGMovementComponent.h"
#include "../FGMovementStatics.h"
#include "Net/UnrealNetwork.h"
#include "FGPlayerSettings.h"
#include "../Debug/UI/FGNetDebugWidget.h"
#include "../FGPickup.h"
#include "../FGRocket.h"

const static float MaxMoveDeltaTime = 0.125f;

AFGPlayer::AFGPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->SetupAttachment(CollisionComponent);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	MovementComponent = CreateDefaultSubobject<UFGMovementComponent>(TEXT("MovementComponent"));
	SetReplicateMovement(false);
}

void AFGPlayer::BeginPlay()
{
	Super::BeginPlay();

	if (!ensure(PlayerSettings != nullptr))
	{
		return;
	}

	SpawnRockets();

	CurrentHealth = PlayerSettings->MaxHealth;

	MovementComponent->SetUpdatedComponent(CollisionComponent);

	CreateDebugWidget();

	if (DebugMenuInstance != nullptr)
	{
		DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	BP_OnHealthChanged(CurrentHealth);
	BP_OnNumRocketsChanged(NumRockets);

	OriginalMeshOffset = MeshComponent->GetRelativeLocation();
}

void AFGPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Accelerate"), this, &AFGPlayer::Handle_Acceleration);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFGPlayer::Handle_Turn);
	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &AFGPlayer::Handle_BrakePressed);
	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &AFGPlayer::Handle_BrakeReleased);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AFGPlayer::Handle_FirePressed);
	PlayerInputComponent->BindAction(TEXT("DebugMenu"), IE_Pressed, this, &AFGPlayer::Handle_DebugMenuPressed);
}

void AFGPlayer::OnHit(float DamageAmount)
{
	OnTakeDamage(DamageAmount);
}

void AFGPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ensure(PlayerSettings != nullptr))
	{
		return;
	}

	FireCooldownElapsed -= DeltaTime;

	FFGFrameMovement FrameMovement = MovementComponent->CreateFrameMovement();
	FGMovementData MovementData;


	if (IsLocallyControlled())
	{
		ClientTimeStamp += DeltaTime;

		const float MaxVelocity = PlayerSettings->MaxVelocity;
		const float Friction = IsBraking() ? PlayerSettings->BrakingFriction : PlayerSettings->Friction;
		const float Alpha = FMath::Clamp(FMath::Abs(MovementVelocity / (PlayerSettings->MaxVelocity * 0.75f)), 0.0f, 1.0f);
		const float TurnSpeed = FMath::InterpEaseOut(0.0f, PlayerSettings->TurnSpeedDefault, Alpha, 5.0f);
		const float MovementDirection = MovementVelocity > 0.0f ? Turn : -Turn;

		Yaw += (MovementDirection * TurnSpeed) * DeltaTime;
		FQuat WantedFacingDirection = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
		MovementComponent->SetFacingRotation(WantedFacingDirection);

		AddMovementVelocity(DeltaTime);
		MovementVelocity *= FMath::Pow(Friction, DeltaTime);

		MovementData.Yaw = GetActorRotation().Yaw;

		MovementComponent->ApplyGravity();
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);

		MovementComponent->Move(FrameMovement);
		Server_SendMovement(GetActorLocation(), ClientTimeStamp, Forward, MovementData);
	}

	else
	{
		const float Friction = IsBraking() ? PlayerSettings->BrakingFriction : PlayerSettings->Friction;
		MovementVelocity *= FMath::Pow(Friction, DeltaTime);
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);
		MovementComponent->Move(FrameMovement);

		if (bPerformNetworkSmoothing)
		{
			//If mesh is close we don't need that high speed
			float InterpSpeed = 0.0f;

			float TestFloat = FVector::Distance(OriginalMeshOffset, MeshComponent->GetRelativeLocation());

			if (FVector::Distance(OriginalMeshOffset, MeshComponent->GetRelativeLocation()) < MaxMeshDistanceFromPlayer)
			{
				InterpSpeed = 1.75f;
			}

			else
			{
				InterpSpeed = 5.0f;
			}

			GEngine->AddOnScreenDebugMessage(-10, 1.f, FColor::Green, FString::Printf(TEXT("Distance: %f"), TestFloat));
			GEngine->AddOnScreenDebugMessage(-10, 1.f, FColor::Green, FString::Printf(TEXT("InterpSpeed: %f"), InterpSpeed));

			const FVector NewRelativeLocation = FMath::VInterpTo(MeshComponent->GetRelativeLocation(), OriginalMeshOffset, LastCorrectionDelta, 5.0f);
			MeshComponent->SetRelativeLocation(NewRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void AFGPlayer::SpawnRockets()
{
	if (HasAuthority() && RocketClass != nullptr)
	{
		const int32 RocketCache = 8;

		for (int32 Index = 0; Index < RocketCache; Index++)
		{
			UFGRocket* Rocket = NewObject<UFGRocket>(RocketClass);

			if (Rocket != nullptr && GetWorld() != nullptr)
			{
				Rocket->SetAbsolute(true);
				Rocket->SetWorldLocation(GetActorLocation());
				Rocket->RegisterComponentWithWorld(GetWorld());
				Rocket->AttachTo(RootComponent);
				RocketInstances.Add(Rocket);
			}
		}
	}
}

void AFGPlayer::HandleInvalidPickUp(AFGPickup* Pickup)
{
	if (ServerNumRockets - Pickup->NumRockets >= 0)
	{
		ServerNumRockets -= Pickup->NumRockets;
	}

	else
	{
		ServerNumRockets = 0;
	}

	if (NumRockets - Pickup->NumRockets >= 0)
	{
		NumRockets -= Pickup->NumRockets;
	}

	else
	{
		NumRockets = 0;
	}

	Pickup->ReActivatePickup();
	Multicast_OnNumRocketsChanged(NumRockets);
}

int32 AFGPlayer::GetPing() const
{
	if (GetPlayerState())
	{
		return static_cast<int32>(GetPlayerState()->GetPing());
	}

	return 0;
}

void AFGPlayer::Client_OnPickupRockets_Implementation(int32 PickedUpRockets)
{
	NumRockets += PickedUpRockets;
	BP_OnNumRocketsChanged(NumRockets);
	Multicast_OnNumRocketsChanged(NumRockets);
}

void AFGPlayer::Server_OnPickup_Implementation(AFGPickup* Pickup)
{
	if (!Pickup->GetIsPickedUp())
	{
		Client_OnPickupRockets(Pickup->NumRockets);
	}

	else
	{
		HandleInvalidPickUp(Pickup);
	}
}

void AFGPlayer::OnPickup(AFGPickup* Pickup)
{
	Pickup->SetVisibility(false);
	Server_OnPickup(Pickup);
}

void AFGPlayer::OnTakeDamage(float DamageAmount)
{
	//Spawn effects and alike
	Server_OnTakeDamage(DamageAmount);
}

void AFGPlayer::OnHeal(float HealAmount)
{
	//Spawn effects / sound etc here.
	Server_OnHeal(HealAmount);
}

void AFGPlayer::Server_OnTakeDamage_Implementation(float DamageAmount)
{
	if (CurrentHealth - DamageAmount >= 0)
	{
		Multicast_OnTakeDamage(DamageAmount);
	}
}

void AFGPlayer::Multicast_OnTakeDamage_Implementation(float DamageAmount)
{
	CurrentHealth -= DamageAmount;
	BP_OnHealthChanged(CurrentHealth);
}

void AFGPlayer::Server_OnHeal_Implementation(float HealAmount)
{
	if (CurrentHealth + HealAmount <= PlayerSettings->MaxHealth)
	{
		Multicast_OnHeal(HealAmount);
	}
}

void AFGPlayer::Multicast_OnHeal_Implementation(float HealAmount)
{
	CurrentHealth += HealAmount;
	BP_OnHealthChanged(CurrentHealth);
}

void AFGPlayer::Multicast_OnNumRocketsChanged_Implementation(int32 NewRocketAmount)
{
	BP_OnNumRocketsChanged(NewRocketAmount);
}

void AFGPlayer::Server_FireRocket_Implementation(UFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation)
{
	if ((ServerNumRockets - 1) < 0 && !bUnlimitedRockets)
	{
		Client_RemoveRocket(NewRocket);
	}

	else
	{
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(RocketFacingRotation.Yaw, GetActorForwardVector().Rotation().Yaw);
		const FRotator NewFacingRotation = RocketFacingRotation + FRotator(0.0f, DeltaYaw, 0.0f);
		ServerNumRockets--;
		Multicast_FireRocket(NewRocket, RocketStartLocation, NewFacingRotation);
		Multicast_OnNumRocketsChanged(ServerNumRockets);
	}
}

void AFGPlayer::Client_RemoveRocket_Implementation(UFGRocket* RocketToRemove)
{
	RocketToRemove->MakeFree();
}

void AFGPlayer::Cheat_IncreaseRockets(int32 InNumRockets)
{
	if (IsLocallyControlled())
	{
		NumRockets += InNumRockets;
	}
}

void AFGPlayer::Server_SendMovement_Implementation(const FVector& ClientLocation, float TimeStamp, float ClientForward, FGMovementData MovementData)
{
	Multicast_SendMovement(ClientLocation, TimeStamp, ClientForward, MovementData);
}

void AFGPlayer::Multicast_SendMovement_Implementation(const FVector& InClientLocation, float TimeStamp, float ClientForward, FGMovementData MovementData)
{
	if (!IsLocallyControlled())
	{
		Forward = ClientForward;
		const float DeltaTime = FMath::Min(TimeStamp - ClientTimeStamp, MaxMoveDeltaTime);
		ClientTimeStamp = TimeStamp;
		AddMovementVelocity(DeltaTime);

		MovementComponent->SetFacingRotation(FRotator(0.0f, MovementData.Yaw, 0.0f));

		const FVector DeltaDiff = InClientLocation - GetActorLocation();

		if (DeltaDiff.SizeSquared() > FMath::Square(40.0f))
		{
			if (bPerformNetworkSmoothing)
			{
				const FScopedPreventAttachedComponentMove PreventMeshMove(MeshComponent);
				MovementComponent->UpdatedComponent->SetWorldLocation(InClientLocation, false, nullptr, ETeleportType::TeleportPhysics);
				LastCorrectionDelta = DeltaTime;
			}

			else
			{
				SetActorLocation(InClientLocation);
			}
		}
	}
}

void AFGPlayer::Multicast_FireRocket_Implementation(UFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation)
{
	if (!ensure(NewRocket != nullptr))
	{
		return;
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		NewRocket->ApplyCorrection(RocketFacingRotation.Vector());
	}

	else
	{
		NumRockets--;
		NewRocket->StartMoving(RocketFacingRotation.Vector(), RocketStartLocation);
	}
}

FVector AFGPlayer::GetRocketStartLocation() const
{
	const FVector StartLocation = GetActorLocation() + GetActorForwardVector() * 100.0f;
	return StartLocation;
}

UFGRocket* AFGPlayer::GetFreeRocket() const
{
	for (UFGRocket* Rocket : RocketInstances)
	{
		if (Rocket == nullptr)
		{
			continue;
		}

		if (Rocket->IsFree())
		{
			return Rocket;
		}
	}

	return nullptr;
}

void AFGPlayer::Server_SendYaw_Implementation(float NewYaw)
{
	ReplicatedYaw = NewYaw;
}

void AFGPlayer::Handle_Acceleration(float Value)
{
	Forward = Value;
}

void AFGPlayer::Handle_Turn(float Value)
{
	Turn = Value;
}

void AFGPlayer::Handle_BrakePressed()
{
	bBrake = true;
}

void AFGPlayer::Handle_BrakeReleased()
{
	bBrake = false;
}

void AFGPlayer::Handle_DebugMenuPressed()
{
	bShowDebugMenu = !bShowDebugMenu;

	if (bShowDebugMenu)
	{
		ShowDebugMenu();
	}

	else
	{
		HideDebugMenu();
	}
}

void AFGPlayer::Handle_FirePressed()
{
	FireRocket();
}

void AFGPlayer::Cheat_DecreaseHealthOnPlayer()
{
	OnTakeDamage(10);
}

void AFGPlayer::Cheat_IncreasePlayerHealth()
{
	OnHeal(10.0f);
}

void AFGPlayer::ShowDebugMenu()
{
	CreateDebugWidget();

	if (DebugMenuInstance == nullptr)
	{
		return;
	}

	DebugMenuInstance->SetVisibility(ESlateVisibility::Visible);
	DebugMenuInstance->BP_OnShowWidget();
}

void AFGPlayer::HideDebugMenu()
{
	if (DebugMenuInstance == nullptr)
	{
		return;
	}

	DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	DebugMenuInstance->BP_OnHideWidget();
}

int32 AFGPlayer::GetNumActiveRockets() const
{
	return NumRockets;
}

void AFGPlayer::FireRocket()
{
	if (FireCooldownElapsed > 0.0f)
	{
		return;
	}

	if (NumRockets <= 0 && !bUnlimitedRockets)
	{
		return;
	}

	if (GetNumActiveRockets() >= MaxActiveRockets)
	{
		return;
	}

	UFGRocket* NewRocket = GetFreeRocket();

	if (!ensure(NewRocket != nullptr))
	{
		return;
	}

	FireCooldownElapsed = PlayerSettings->FireCooldown;

	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		if (HasAuthority())
		{
			Server_FireRocket(NewRocket, GetRocketStartLocation(), GetActorRotation());
		}

		else
		{
			NumRockets--;
			NewRocket->StartMoving(GetActorForwardVector(), GetRocketStartLocation());
			Server_FireRocket(NewRocket, GetRocketStartLocation(), GetActorRotation());
		}
	}
}

void AFGPlayer::AddMovementVelocity(float DeltaTime)
{
	if (!ensure(PlayerSettings != nullptr))
	{
		return;
	}

	const float MaxVelocity = PlayerSettings->MaxVelocity;
	const float Acceleration = PlayerSettings->Acceleration;

	MovementVelocity += Forward * Acceleration * DeltaTime;
	MovementVelocity = FMath::Clamp(MovementVelocity, -MaxVelocity, MaxVelocity);
}

void AFGPlayer::CreateDebugWidget()
{
	if (DebugMenuClass == nullptr)
	{
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	if (DebugMenuInstance == nullptr)
	{
		DebugMenuInstance = CreateWidget<UFGNetDebugWidget>(GetWorld(), DebugMenuClass);
		DebugMenuInstance->AddToViewport();
	}
}

void AFGPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFGPlayer, ReplicatedYaw);
	DOREPLIFETIME(AFGPlayer, CurrentHealth);
	DOREPLIFETIME(AFGPlayer, ReplicatedLocation);
	DOREPLIFETIME(AFGPlayer, RocketInstances);
}