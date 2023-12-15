#pragma once
#include <mutex>
#include <queue>
#include "GameFramework/Actor.h"
#include "CrowdControlParameter.h"
#include "CCManager.generated.h"

class FCrowdControlRunnable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMyOnTriggerEffect, FString, ID);

UENUM(BlueprintType)
enum class Morality {
	ExtremelyHarmful = 0,
	VeryHarmful = 1,
	Harmful = 2,
	SlightlyHarmful = 3,
	Neutral = 4,
	SlightlyHelpful = 5,
	Helpful = 6,
	VeryHelpful = 7,
	ExtremelyHelpful = 8
};

UENUM(BlueprintType)
enum class Orderliness {
	ExtremelyChaotic = 0,
	VeryChaotic = 1,
	Chaotic = 2,
	SlightlyChaotic = 3,
	Neutral_Orderliness = 4,
	SlightlyControlled = 5,
	Controlled = 6,
	VeryControlled = 7,
	ExtremelyControlled = 8
};


UCLASS()
class CROWDCONTROL_API ACCManager : public AActor {
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
    void LoadDLL(FString GameKey);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void Connect();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void Disconnect();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void LoginTwitch();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void LoginYoutube();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void LoginDiscord();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void Effect(TArray<FString> categories, FString name = "Effect", FString description = "Desc", int32 price = 3, int32 maxRetries = 3, float retryDelay = 1.0f, float pendingDelay = 3.0f, bool sellable = true, bool visible = true, bool nonPoolable = false);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void TimedEffect(FString id, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, TArray<FString> categories, int32 duration);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void ParameterEffect(FString id, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, TArray<FString> categories, TArray<UCrowdControlParameter*> parameters);
	
	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
    FMyOnTriggerEffect OnEffectTrigger;
	
	/*UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void OnEffectTrigger(const FString& Value);*/
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void EffectSuccess(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void EffectFailure(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void EffectPause(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void EffectResume(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void EffectReset(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static void EffectStop(FString id);
	
	void StartThread();
	
	typedef int (*CrowdControlFunctionType)();
	static CrowdControlFunctionType CrowdControlFunction;
	
	typedef void (*CrowdControlConnectFunctionType)();
	static CrowdControlConnectFunctionType ConnectFunction;
	
	typedef void (*CrowdControlDisconnectFunctionType)();
	static CrowdControlDisconnectFunctionType DisconnectFunction;
	
	typedef void (*SetEngineType)();
	SetEngineType SetEngine;

	typedef int (*FP_Command)();
	FP_Command CommandFunction;
	
	typedef void (*ResetCommandType)();
	ResetCommandType ResetCommand;
	
	typedef void (*LoginTwitchType)();
	static LoginTwitchType LoginTwitchFunction;
	
	typedef void (*LoginDiscordType)();
	static LoginDiscordType LoginDiscordFunction;
	
	typedef void (*LoginYoutubeType)();
	static LoginYoutubeType LoginYoutubeFunction;
	
	typedef char* (*StringTestType)();
	StringTestType StringTest;
	
	typedef void (*AddBasicEffectType)(char* name, char* desc, int price); 
	static AddBasicEffectType AddBasicEffect;
	
	UFUNCTION(BlueprintCallable, Category = "CrowdControl")
    static int32 CrowdControlResult();
	
	FCrowdControlRunnable* Runnable = nullptr;
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
    static ACCManager* CrowdControl();
	
	virtual ~ACCManager();
	
	static char * StringToSend;  // Declare as a static array with a fixed size
	
	static std::string ACCManager::FStringToUtf8String(const FString& InString);
	
protected:
	static ACCManager* Instance;

	static void* DLLHandle;
	std::mutex QueueMutex;
    std::queue<std::function<void()>> MainThreadTasks;
	void Update();
	void Tick(float DeltaTime);
	static int Result; 
};
