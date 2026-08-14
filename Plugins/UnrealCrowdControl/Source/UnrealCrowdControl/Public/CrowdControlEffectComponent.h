#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrowdControlTypes.h"
#include "Runtime/JsonUtilities/Public/JsonObjectWrapper.h"
#include "CrowdControlEffectComponent.generated.h"

class UCrowdControlSubsystem;

// How an effect component answered a trigger.
UENUM(BlueprintType)
enum class ECrowdControlEffectResult : uint8
{
	// Effect applied; success is reported to Crowd Control automatically.
	Success,
	// Effect can't run right now; reported as failTemporary (may be retried/refunded).
	FailTemporary,
	// Effect can never run; reported as failPermanent.
	FailPermanent,
	// The component will respond later via CompleteSuccess / CompleteFailTemporary /
	// CompleteFailPermanent (for async handling: latent loads, timelines, etc.).
	Pending,
};

/**
 * Self-contained Crowd Control effect. Add one per effect to any actor, fill in the
 * menu metadata, and override OnEffectTriggered (Blueprint or C++). The component
 * registers itself with the CrowdControl subsystem on BeginPlay and automatically
 * sends the success/failure response based on the callback's return value.
 *
 * Timed effects (Duration > 0) are tracked locally: the component ticks down the
 * remaining time, honors Pause/Resume, and calls OnEffectStopped when time expires.
 */
UCLASS(Blueprintable, ClassGroup=(CrowdControl), meta=(BlueprintSpawnableComponent))
class UNREALCROWDCONTROL_API UCrowdControlEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrowdControlEffectComponent();

	// Unique effect ID (lowercase, no spaces) - must match the effect pack.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control")
	FString EffectID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control", meta=(MultiLine="true"))
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control", meta=(ClampMin="1"))
	int32 Price = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control")
	TArray<FString> Categories;

	// Duration in seconds. 0 = instant effect; > 0 makes this a timed effect.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control", meta=(ClampMin="0.0", ClampMax="600.0"))
	float Duration = 0.f;

	// Register with the subsystem automatically on BeginPlay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crowd Control")
	bool bAutoRegister = true;

	// Called when a viewer triggers this effect. Return how it was handled; the
	// component sends the matching response. Return Pending to respond later via
	// the Complete* functions. ViewerName is the display name of the viewer who
	// bought the effect (may be empty for test effects).
	UFUNCTION(BlueprintNativeEvent, Category="Crowd Control")
	ECrowdControlEffectResult OnEffectTriggered(const FString& RequestID, int32 Quantity, const FJsonObjectWrapper& Parameters, const FString& ViewerName);
	virtual ECrowdControlEffectResult OnEffectTriggered_Implementation(const FString& RequestID, int32 Quantity, const FJsonObjectWrapper& Parameters, const FString& ViewerName);

	// Timed-effect lifecycle. Override to apply/remove the effect's gameplay state.
	UFUNCTION(BlueprintNativeEvent, Category="Crowd Control")
	void OnEffectPaused();
	virtual void OnEffectPaused_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category="Crowd Control")
	void OnEffectResumed();
	virtual void OnEffectResumed_Implementation() {}

	// Called when a timed effect ends (time expired or stopped explicitly).
	UFUNCTION(BlueprintNativeEvent, Category="Crowd Control")
	void OnEffectStopped();
	virtual void OnEffectStopped_Implementation() {}

	// Registers the effect with Crowd Control (menu setup + trigger routing).
	// Called automatically on BeginPlay when bAutoRegister is set.
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void Register();

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void Unregister();

	// Async completion for handlers that returned Pending.
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void CompleteSuccess();

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void CompleteFailTemporary(const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void CompleteFailPermanent(const FString& Message);

	// Timed-effect control.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crowd Control")
	bool IsRunning() const { return bRunning; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crowd Control")
	bool IsPaused() const { return bPaused; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crowd Control")
	float GetTimeRemaining() const { return TimeRemaining; }

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void Pause();

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void Resume();

	// Ends a running timed effect early.
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void Stop();

	// Report menu state for this effect. Visibility and availability are
	// independent states on the Crowd Control side.
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool SetVisibility(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool SetAvailability(bool bAvailable);

	// Internal: invoked by the subsystem when a trigger for this EffectID arrives.
	void HandleTrigger(const FString& RequestID, float InDuration, int32 Quantity, const FJsonObjectWrapper& Parameters, const FString& ViewerName);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UCrowdControlSubsystem* GetCrowdControl() const;
	void RespondWith(ECrowdControlEffectResult Result, const FString& RequestID);
	void FinishTimedEffect(bool bNotifyServer);

	FString CurrentRequestID;
	float TimeRemaining = 0.f;
	bool bRunning = false;
	bool bPaused = false;
	bool bRegistered = false;
};
