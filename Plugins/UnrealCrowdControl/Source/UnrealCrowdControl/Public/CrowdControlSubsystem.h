#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include "CoreMinimal.h"
#include "CrowdControlFunctionLibrary.h"
#include "CrowdControlRunable.h"
#include "Runtime/JsonUtilities/Public/JsonObjectWrapper.h"
#include "CrowdControlSubsystem.generated.h"

class UWorld;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTriggerEffect, FString, ID, FString, DisplayName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTriggerTimedEffect, FString, ID, FString, DisplayName, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTriggerParameterEffect, FString, ID, FString, DisplayName, FString, OptionalQuantity, FJsonObjectWrapper, Params);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandIDChanged, int32, Value);


UCLASS(BlueprintType)
class UNREALCROWDCONTROL_API UCrowdControlSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY() 

public:
	virtual ~UCrowdControlSubsystem();
	
	static UCrowdControlSubsystem& Get(const UObject* WorldContextObject);
	
	// UGameInstanceSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// UGameInstanceSubsystem
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
    void LoadDLL();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crowd Control")
	int32 GetCommandID() const { return CommandID; }
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void Connect();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void Disconnect();
	
	// Resets login and connection
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void ResetConnection();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void LoginTwitch();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void LoginYoutube();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void LoginDiscord();
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SetupEffect(const FCrowdControlEffectInfo& Info);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SetupTimedEffect(const FCrowdControlTimedEffectInfo& Info);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SetupParameterEffect(const FCrowdControlParameterEffectInfo& Info);
	
	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
    FOnTriggerEffect OnEffectTrigger;

	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	FOnTriggerTimedEffect OnTimedEffectTrigger;

	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	FOnTriggerParameterEffect OnParameterEffectTrigger;

	// UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	// FOnTriggerEffect OnEffectEnd;

	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	FOnCommandIDChanged OnCommandIDChanged;

	// Send RPC "Success" to confirm this event worked
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectSuccess(FString id);

	// Send RPC "FailTemporarily" to signal this event failed
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectFailure(FString id);

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool IsEffectRunning(FString name);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void PauseEffect(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void ResumeEffect(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void ResetEffect(FString id);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void StopEffect(FString id);

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool IsConnected() const { return bIsConnected; }

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool IsInitialized() const { return bIsInitialized; }

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void PrintEffectsToJsonFile();

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	FString GetOriginID();

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	FString GetProfileType();

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	FString GetInteractionURL();

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	FString GetStreamerName();

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	bool GetIsJWTTokenValid();
	
	void StartThread();
	
	typedef int (*CrowdControlFunctionType)();
	static CrowdControlFunctionType CC_CrowdControlFunction;
	
	typedef void (*CrowdControlConnectFunctionType)();
	static CrowdControlConnectFunctionType CC_ConnectFunction;
	
	typedef void (*CrowdControlDisconnectFunctionType)();
	static CrowdControlDisconnectFunctionType CC_DisconnectFunction;
	
	typedef void (*SetEngineType)();
	SetEngineType CC_SetEngine;

	typedef int (*FP_Command)();
	FP_Command CC_CommandFunction;
	
	typedef void (*ResetCommandType)();
	ResetCommandType CC_ResetCommand;
	
	typedef void (*LoginTwitchType)();
	static LoginTwitchType CC_LoginTwitchFunction;
	
	typedef void (*LoginDiscordType)();
	static LoginDiscordType CC_LoginDiscordFunction;
	
	typedef void (*LoginYoutubeType)();
	static LoginYoutubeType CC_LoginYoutubeFunction;
	
	typedef char* (*StringTestType)();
	StringTestType CC_StringTest;
	
	typedef char* (*EngineEffectType)();
	EngineEffectType CC_EngineEffect;

	typedef char* (*GetOriginIDType)();
	GetOriginIDType CC_GetOriginID;

	typedef char* (*GetProfileTypeType)();
	GetProfileTypeType CC_GetProfileType;

	typedef char* (*GetInteractionURLType)();
	GetInteractionURLType CC_GetInteractionURL;

	typedef char* (*GetStreamerNameType)();
	GetStreamerNameType CC_GetStreamerName;

	typedef bool (*IsJWTTokenValidType)();
	IsJWTTokenValidType CC_IsJWTTokenValid;
	
	TUniquePtr<FCrowdControlRunnable> Runnable = nullptr;
	
	static char * StringToSend;  // Declare as a static array with a fixed size

	static char** SplitCategories(TArray<FString> categories);

	FTimerHandle TickTimerHandle;

protected:
	
	void SetupWorldTimer(UWorld* World, const UWorld::InitializationValues);
		
	UFUNCTION()
	void OnTimerManagerTick();

	bool bIsConnected = false;
	bool bIsInitialized = false;

	FString LastSuccessfulEffectID;
	FString MenuJson;
	TSharedPtr<FJsonObject> GameJsonObject;

	void* DLLHandle = nullptr;
	std::mutex QueueMutex;
    std::queue<std::function<void()>> MainThreadTasks;
	
	void Update();
	void Tick(float DeltaTime);
	int32 CommandID; 
};
