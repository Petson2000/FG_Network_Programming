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

	CurrentHealth = PlayerSettings->MaxHealth;

	MovementComponent->SetUpdatedComponent(CollisionComponent);

	CreateDebugWidget();
	if (DebugMenuInstance != nullptr)
	{
		DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	SpawnRockets();

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


void AFGPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ensure(PlayerSettings != nullptr))
	{
		return;
	}

	FireCooldownElapsed -= DeltaTime;

	FFGFrameMovement FrameMovement = MovementComponent->CreateFrameMovement();

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
	
		UE_LOG(LogTemp, Warning, TEXT("Movement Velocity: %f"), MovementVelocity);

		MovementComponent->ApplyGravity();
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);
		MovementComponent->Move(FrameMovement);
		
		Server_SendMovement(GetActorLocation(), ClientTimeStamp, Forward, GetActorRotation().Yaw);
	}

	else
	{
		const float Friction = IsBraking() ? PlayerSettings->BrakingFriction : PlayerSettings->Friction;
		MovementVelocity *= FMath::Pow(Friction, DeltaTime);
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);
		MovementComponent->Move(FrameMovement);

		if (bPerformNetworkSmoothing)
		{
			const FVector NewRelativeLocation = FMath::VInterpTo(MeshComponent->GetRelativeLocation(), OriginalMeshOffset, LastCorrectionDelta, 1.75f);
			MeshComponent->SetRelativeLocation(NewRelativeLocation, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

float AFGPlayer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	CurrentHealth -= DamageAmount;
	Server_OnHealthChanged(DamageAmount);

	return CurrentHealth;
}

void AFGPlayer::SpawnRockets()
{
	if (HasAuthority() && RocketClass != nullptr)
	{
		const int32 RocketCache = 8;

		for (int32 Index = 0; Index < RocketCache; Index++)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.ObjectFlags = RF_Transient;
			SpawnParams.Instigator = this;
			SpawnParams.Owner = this;
			AFGRocket* NewRocketInstance = GetWorld()->SpawnActor<AFGRocket>(RocketClass, GetActorLocation(), GetActorRotation(), SpawnParams);
			RocketInstances.Add(NewRocketInstance);
		}
	}
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
}

void AFGPlayer::OnPickup(AFGPickup* Pickup)
{
	if (IsLocallyControlled())
	{
		Server_OnPickup(Pickup);
	}
}

void AFGPlayer::Server_OnHealthChanged_Implementation(float DamageAmount)
{
	if (IsLocallyControlled())
	{
		BP_OnHealthChanged(CurrentHealth);
	}

	Multicast_OnHealthChanged();
}

void AFGPlayer::Multicast_OnHealthChanged_Implementation(float DamageAmount)
{
	BP_OnHealthChanged(CurrentHealth);
}

void AFGPlayer::Server_FireRocket_Implementation(AFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation)
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
	}
}

void AFGPlayer::Client_RemoveRocket_Implementation(AFGRocket* RocketToRemove)
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

void AFGPlayer::Server_SendMovement_Implementation(const FVector& ClientLocation, float TimeStamp, float ClientForward, float ClientYaw)
{
	Multicast_SendMovement(ClientLocation, TimeStamp, ClientForward, ClientYaw);
}

void AFGPlayer::Multicast_SendMovement_Implementation(const FVector& InClientLocation, float TimeStamp, float ClientForward, float ClientYaw)
{
	if (!IsLocallyControlled())
	{
		Forward = ClientForward;
		const float DeltaTime = FMath::Min(TimeStamp - ClientTimeStamp, MaxMoveDeltaTime);
		ClientTimeStamp = TimeStamp;
		AddMovementVelocity(DeltaTime);
		MovementComponent->SetFacingRotation(FRotator(0.0f, ClientYaw, 0.0f));

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

void AFGPlayer::Multicast_FireRocket_Implementation(AFGRocket* NewRocket, const FVector& RocketStartLocation, const FRotator& RocketFacingRotation)
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

AFGRocket* AFGPlayer::GetFreeRocket() const
{
	for (AFGRocket* Rocket : RocketInstances)
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

void AFGPlayer::Server_OnPickup_Implementation(AFGPickup* Pickup)
{
	ServerNumRockets += Pickup->NumRockets;
	Client_OnPickupRockets(Pickup->NumRockets);
}

void AFGPlayer::Multicast_SendLocation_Implementation(const FVector& LocationToSend, float DeltaTime)
{
	if (!IsLocallyControlled())
	{
		prevPingedLocation = LocationToSend;
		PrevPingedTime = DeltaTime;
	}
}

void AFGPlayer::Multicast_SendRotation_Implementation(const FRotator& RotationToSend, float DeltaTime)
{
	if (!IsLocallyControlled())
	{
		prevPingedRotation = RotationToSend;
		PrevPingedTime = DeltaTime;
	}
}

void AFGPlayer::Server_SendLocation_Implementation(const FVector& LocationToSend, float DeltaTime)
{
	ReplicatedLocation = LocationToSend;
}

void AFGPlayer::Server_SendRotation_Implementation(const FRotator& RotationToSend, float DeltaTime)
{
	Multicast_SendRotation(RotationToSend, DeltaTime);
}

void AFGPlayer::Server_SendYaw_Implementation(float NewYaw)
{
	ReplicatedYaw = NewYaw;
}

void AFGPlayer::Handle_Acceleration(float Value)
{
	GEngine->AddOnScreenDebugMessage(-10, 1.f, FColor::Green, FString::Printf(TEXT("Forward: %f"), Forward));
	Forward = Value;
}

void AFGPlayer::Handle_Turn(float Value)
{
	GEngine->AddOnScreenDebugMessage(-10, 1.f, FColor::Green, FString::Printf(TEXT("Turn: %f"), Forward));
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
	FDamageEvent E;
	TakeDamage(5.0f, E, GetInstigatorController(), this);
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

	AFGRocket* NewRocket = GetFreeRocket();

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
	//UE_LOG(LogTemp, Warning, TEXT("MovementVelocity: %f"), MovementVelocity);
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