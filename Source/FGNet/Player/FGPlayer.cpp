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

	MovementComponent->SetUpdatedComponent(CollisionComponent);

	CreateDebugWidget();
	if (DebugMenuInstance != nullptr)
	{
		DebugMenuInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AFGPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Accelerate"), this, &AFGPlayer::Handle_Acceleration);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFGPlayer::Handle_Turn);

	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &AFGPlayer::Handle_BrakePressed);
	PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &AFGPlayer::Handle_BrakeReleased);

	PlayerInputComponent->BindAction(TEXT("DebugMenu"), IE_Pressed, this, &AFGPlayer::Handle_DebugMenuPressed);
}

void AFGPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ensure(PlayerSettings != nullptr))
	{
		return;
	}

	if (IsLocallyControlled())
	{
		const float MaxVelocity = PlayerSettings->MaxVelocity;
		const float Acceleration = PlayerSettings->Acceleration;
		const float Friction = IsBraking() ? PlayerSettings->BrakingFriction : PlayerSettings->Friction;
		const float Alpha = FMath::Clamp(FMath::Abs(MovementVelocity / (PlayerSettings->MaxVelocity * 0.75f)), 0.0f, 1.0f);
	
		const float TurnSpeed = FMath::InterpEaseOut(0.0f, PlayerSettings->TurnSpeedDefault, Alpha, 5.0f);
		const float MovementDirection = MovementVelocity > 0.0f ? Turn : -Turn;
	
		Yaw += (MovementDirection * TurnSpeed) * DeltaTime;
		FQuat WantedFacingDirection = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
		MovementComponent->SetFacingRotation(WantedFacingDirection);
	
		FFGFrameMovement FrameMovement = MovementComponent->CreateFrameMovement();
	
		MovementVelocity += Forward * Acceleration * DeltaTime;
		MovementVelocity = FMath::Clamp(MovementVelocity, -MaxVelocity, MaxVelocity);
		MovementVelocity *= FMath::Pow(Friction, DeltaTime);
	
		MovementComponent->ApplyGravity();
		FrameMovement.AddDelta(GetActorForwardVector() * MovementVelocity * DeltaTime);
		MovementComponent->Move(FrameMovement);
	
		Server_SendRotation(GetActorRotation(), DeltaTime);
		Server_SendLocation(GetActorLocation(), DeltaTime);

		Server_SendYaw(MovementComponent->GetFacingRotation().Yaw);
	}

	else
	{
		if (GetActorLocation() != prevPingedLocation)
		{
			//const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), ReplicatedLocation, PrevPingedTime, TransitionTime);
			//
			//SetActorLocation(NewLocation);

			SetActorLocation(FMath::VInterpTo(GetActorLocation(), prevPingedLocation, PrevPingedTime, TransitionTime));
		}
	
		if (GetActorRotation() != prevPingedRotation)
		{
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), prevPingedRotation, PrevPingedTime, TransitionTime));
		}

		MovementComponent->SetFacingRotation(FRotator(0.0f, ReplicatedYaw, 0.0f), 7.0f);
		SetActorRotation(MovementComponent->GetFacingRotation());
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
	//ReplicatedLocation = LocationToSend;
	Multicast_SendLocation(LocationToSend, DeltaTime);
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
	DOREPLIFETIME(AFGPlayer, ReplicatedLocation);
}