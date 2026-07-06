#include "CrowdControlEffectComponent.h"

#include "CrowdControlSubsystem.h"
#include "CrowdControlLogChannels.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UCrowdControlEffectComponent::UCrowdControlEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UCrowdControlSubsystem* UCrowdControlEffectComponent::GetCrowdControl() const
{
	const UWorld* World = GetWorld();
	if (!World || !World->GetGameInstance())
	{
		return nullptr;
	}

	return World->GetGameInstance()->GetSubsystem<UCrowdControlSubsystem>();
}

void UCrowdControlEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoRegister)
	{
		Register();
	}
}

void UCrowdControlEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bRunning)
	{
		FinishTimedEffect(true);
	}

	Unregister();
	Super::EndPlay(EndPlayReason);
}

void UCrowdControlEffectComponent::Register()
{
	UCrowdControlSubsystem* CrowdControl = GetCrowdControl();
	if (!CrowdControl)
	{
		return;
	}

	if (EffectID.IsEmpty())
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CrowdControlEffectComponent on %s has no EffectID set; not registering."), *GetNameSafe(GetOwner()));
		return;
	}

	if (Duration > 0.f)
	{
		FCrowdControlTimedEffectInfo Info;
		Info.id = EffectID;
		Info.displayName = DisplayName;
		Info.description = Description;
		Info.price = Price;
		Info.category = Categories;
		Info.duration = Duration;
		CrowdControl->SetupTimedEffect(Info);
	}
	else
	{
		FCrowdControlEffectInfo Info;
		Info.id = EffectID;
		Info.displayName = DisplayName;
		Info.description = Description;
		Info.price = Price;
		Info.category = Categories;
		CrowdControl->SetupEffect(Info);
	}

	CrowdControl->RegisterEffectComponent(this);
	bRegistered = true;
}

void UCrowdControlEffectComponent::Unregister()
{
	if (!bRegistered)
	{
		return;
	}

	if (UCrowdControlSubsystem* CrowdControl = GetCrowdControl())
	{
		CrowdControl->UnregisterEffectComponent(this);
	}

	bRegistered = false;
}

ECrowdControlEffectResult UCrowdControlEffectComponent::OnEffectTriggered_Implementation(const FString& RequestID, int32 Quantity, const FJsonObjectWrapper& Parameters, const FString& ViewerName)
{
	// Default: accept the effect. Blueprint/C++ overrides replace this.
	return ECrowdControlEffectResult::Success;
}

void UCrowdControlEffectComponent::HandleTrigger(const FString& RequestID, float InDuration, int32 Quantity, const FJsonObjectWrapper& Parameters, const FString& ViewerName)
{
	if (bRunning && Duration > 0.f)
	{
		// A timed effect is already running; refuse so the request can be retried.
		RespondWith(ECrowdControlEffectResult::FailTemporary, RequestID);
		return;
	}

	CurrentRequestID = RequestID;

	const ECrowdControlEffectResult Result = OnEffectTriggered(RequestID, Quantity, Parameters, ViewerName);

	if (Result == ECrowdControlEffectResult::Success && Duration > 0.f)
	{
		TimeRemaining = InDuration > 0.f ? InDuration : Duration;
		bRunning = true;
		bPaused = false;
		SetComponentTickEnabled(true);
	}

	RespondWith(Result, RequestID);
}

void UCrowdControlEffectComponent::RespondWith(ECrowdControlEffectResult Result, const FString& RequestID)
{
	UCrowdControlSubsystem* CrowdControl = GetCrowdControl();
	if (!CrowdControl)
	{
		return;
	}

	switch (Result)
	{
	case ECrowdControlEffectResult::Success:
		CrowdControl->EffectSuccess(RequestID);
		break;
	case ECrowdControlEffectResult::FailTemporary:
		CrowdControl->EffectFailureTemporary(RequestID, FString());
		break;
	case ECrowdControlEffectResult::FailPermanent:
		CrowdControl->EffectFailurePermanent(RequestID, FString());
		break;
	case ECrowdControlEffectResult::Pending:
		// The component owner responds later via Complete*.
		break;
	}
}

void UCrowdControlEffectComponent::CompleteSuccess()
{
	if (CurrentRequestID.IsEmpty())
	{
		return;
	}

	if (UCrowdControlSubsystem* CrowdControl = GetCrowdControl())
	{
		CrowdControl->EffectSuccess(CurrentRequestID);
	}

	if (Duration > 0.f && !bRunning)
	{
		TimeRemaining = Duration;
		bRunning = true;
		bPaused = false;
		SetComponentTickEnabled(true);
	}
}

void UCrowdControlEffectComponent::CompleteFailTemporary(const FString& Message)
{
	if (CurrentRequestID.IsEmpty())
	{
		return;
	}

	if (UCrowdControlSubsystem* CrowdControl = GetCrowdControl())
	{
		CrowdControl->EffectFailureTemporary(CurrentRequestID, Message);
	}

	CurrentRequestID.Empty();
}

void UCrowdControlEffectComponent::CompleteFailPermanent(const FString& Message)
{
	if (CurrentRequestID.IsEmpty())
	{
		return;
	}

	if (UCrowdControlSubsystem* CrowdControl = GetCrowdControl())
	{
		CrowdControl->EffectFailurePermanent(CurrentRequestID, Message);
	}

	CurrentRequestID.Empty();
}

void UCrowdControlEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bRunning || bPaused)
	{
		return;
	}

	TimeRemaining -= DeltaTime;
	if (TimeRemaining <= 0.f)
	{
		FinishTimedEffect(true);
	}
}

void UCrowdControlEffectComponent::Pause()
{
	if (!bRunning || bPaused)
	{
		return;
	}

	bPaused = true;

	if (UCrowdControlSubsystem* CrowdControl = GetCrowdControl())
	{
		CrowdControl->PauseEffect(EffectID);
	}

	OnEffectPaused();
}

void UCrowdControlEffectComponent::Resume()
{
	if (!bRunning || !bPaused)
	{
		return;
	}

	bPaused = false;

	if (UCrowdControlSubsystem* CrowdControl = GetCrowdControl())
	{
		CrowdControl->ResumeEffect(EffectID);
	}

	OnEffectResumed();
}

void UCrowdControlEffectComponent::Stop()
{
	if (!bRunning)
	{
		return;
	}

	FinishTimedEffect(true);
}

void UCrowdControlEffectComponent::FinishTimedEffect(bool bNotifyServer)
{
	bRunning = false;
	bPaused = false;
	TimeRemaining = 0.f;
	SetComponentTickEnabled(false);

	if (bNotifyServer)
	{
		if (UCrowdControlSubsystem* CrowdControl = GetCrowdControl())
		{
			CrowdControl->StopEffect(EffectID);
		}
	}

	CurrentRequestID.Empty();
	OnEffectStopped();
}

bool UCrowdControlEffectComponent::SetVisibility(bool bVisible)
{
	UCrowdControlSubsystem* CrowdControl = GetCrowdControl();
	return CrowdControl != nullptr && CrowdControl->SetEffectVisibility(EffectID, bVisible);
}

bool UCrowdControlEffectComponent::SetAvailability(bool bAvailable)
{
	UCrowdControlSubsystem* CrowdControl = GetCrowdControl();
	return CrowdControl != nullptr && CrowdControl->SetEffectAvailability(EffectID, bAvailable);
}
