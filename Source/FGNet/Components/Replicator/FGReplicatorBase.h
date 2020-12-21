#pragma once

#include "UObject/Object.h"
#include "Tickable.h"
#include "FGReplicatorBase.generated.h"

UENUM()
enum class EFGSmoothReplicatorMode : uint8
{
	ConstantVelocity
};

template<typename ValueType>
struct TFGSmoothReplicatorOperation
{
	static void InterpConstantVelocity(ValueType& CurrentValue, const ValueType& FrameTarget, float Alpha)
	{
		CurrentValue = CurrentValue + (FrameTarget - CurrentValue) * Alpha;
	}
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class FGNET_API UFGReplicatorBase : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Init() {}

	//UObject
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack) override;
	virtual bool IsSupportedForNetworking() const override;
	virtual bool IsNameStableForNetworking() const override;
	//UObject

	//FTickableGameobject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//FTickableGameObject

	void SetShouldTick(bool bInShouldTick);
	bool IsTicking() const;

	bool IsLocallyControlled() const;
	bool HasAuthority() const;

private:

	bool bShouldTick = false;

};