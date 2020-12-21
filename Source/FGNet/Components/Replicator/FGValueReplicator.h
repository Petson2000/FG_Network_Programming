#pragma once

#include "FGReplicator.h"
#include "FGValueReplicator.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFGOnSmoothValueReplicationChanged);

class FGNET_API UFGValueReplicator : public UFGReplicatorBase
{
	GENERATED_BODY()

public:

	virtual void Tick(float DeltaTime) override;

	virtual void Init() override;

	UFUNCTION(Server, Reliable)
	void Server_SendTerminalValue(int32 SyncTag, float TerminalValue);

	UFUNCTION(Server, Unreliable)
	void Server_SendReplicatedValue(int32 SyncTag, float ReplicatedValue);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SendTerminalValue(int32 SyncTag, float ReplicatedValue);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SendReplicatedValue(int32 SyncTag, float ReplicatedValue);

	UFUNCTION(BlueprintCallable, Category = Network)
	void SetValue(float InValue);

	UFUNCTION(BlueprintPure, Category = Network)
	float GetValue() const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1))
	int32 NumberOfReplicationsPerSecond = 10;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EFGSmoothReplicatorMode SmoothMode = EFGSmoothReplicatorMode::ConstantVelocity;

	UPROPERTY(BlueprintAssignable)
	FFGOnSmoothValuereplicationChanged OnValueChanged;

	bool ShouldTick() const;

private:

	void BroadcastDelegate();

	struct FCrumb
	{
		float Value;
	};

	TArray<FCrumb, TInlineAllocator<10>> CrumbTrail;

	float ReplicatedValueTarget = 0.0f;
	float ReplicatedValueCurrent = 0.0f;
	float ReplicatedValuePerviouslySent = 0.0f;
	float StaticValueTimer = 0.0f;
	float SleepAfterDuration = 1.0f;

	int32 NextSyncTag = 0;
	int32 LastRecievedSyncTag = -1;
	int32 LastRecievedCrumbSyncTag = -1;

	float SyncTimer = 0.0f;
	float LerpSpeed = 0.0f;
	float CurrentCrumbTimeRemaining = 0.0f;

	bool bHasRevievedTerminalValue = false;
	bool bHasSentTerminalValue = false;
	bool bIsSleeping = false;
};