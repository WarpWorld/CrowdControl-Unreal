#pragma once
#include <mutex>
#include <queue>
#include "GameFramework/Actor.h"
#include "CCManager.generated.h"


class FCrowdControlRunnable;


UCLASS()
class CROWDCONTROL_API ACCManager : public AActor
{
	GENERATED_UCLASS_BODY()
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
    void LoadDLL(FString GameKey);
	typedef int (*CrowdControlFunctionType)();
	CrowdControlFunctionType CrowdControlFunction;
	
	typedef void (*CrowdControlDisconnectFunctionType)();
	CrowdControlDisconnectFunctionType DisconnectFunction;
	
	typedef void (*SetEngineType)();
	SetEngineType SetEngine;

	typedef int (*FP_Command)();
	FP_Command CommandFunction;
	
	typedef void (*ResetCommandType)();
	ResetCommandType ResetCommand;
	
	typedef void (*LoginTwitchType)();
	LoginTwitchType LoginTwitch;
	
	typedef void (*LoginDiscordType)();
	LoginDiscordType LoginDiscord;
	
	typedef void (*LoginYoutubeType)();
	LoginYoutubeType LoginYoutube;
	
	typedef char* (*StringTestType)();
	StringTestType StringTest;
	
	FCrowdControlRunnable* Runnable = nullptr;
	
	virtual ~ACCManager();
	
protected:
	void* DLLHandle;
	std::mutex QueueMutex;
    std::queue<std::function<void()>> MainThreadTasks;
	void Update();
	void Tick(float DeltaTime);
};
