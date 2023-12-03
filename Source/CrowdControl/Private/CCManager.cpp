#include "CrowdControl.h"
#include "CCManager.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Misc/Paths.h"
#include "CrowdControlRunable.h"

ACCManager::ACCManager(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;

    // Initialize members
    DLLHandle = nullptr;
    CrowdControlFunction = nullptr;
    DisconnectFunction = nullptr;
	SetEngine = nullptr;

    Runnable = nullptr;
}

void ACCManager::LoadDLL(FString GameKey)
{
	FString DLLPath = FPaths::Combine(*FPaths::GameDir(), TEXT("Binaries/Win64/CrowdControl.dll"));
    DLLHandle = FPlatformProcess::GetDllHandle(*DLLPath); 

    if (DLLHandle != nullptr) {
		CrowdControlFunction = (CrowdControlFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?Run@CrowdControlRunner@@QEAAHXZ"));
		DisconnectFunction = (CrowdControlDisconnectFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?Disconnect@CrowdControlRunner@@SAXXZ"));
		SetEngine = (SetEngineType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?EngineSet@CrowdControlRunner@@QEAAXXZ"));
		
		CommandFunction = (FP_Command)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?CommandID@CrowdControlRunner@@QEAAHXZ"));
		
		ResetCommand = (ResetCommandType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?ResetCommandCode@CrowdControlRunner@@QEAAXXZ"));
		LoginTwitch = (LoginTwitchType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?LoginTwitch@CrowdControlRunner@@SAXXZ"));
		LoginDiscord = (LoginDiscordType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?LoginDiscord@CrowdControlRunner@@SAXXZ"));
		LoginYoutube = (LoginYoutubeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?LoginYoutube@CrowdControlRunner@@SAXXZ"));
		StringTest = (StringTestType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?TestCharArray@CrowdControlRunner@@QEAAPEADXZ"));

		SetEngine();
		
        if (CrowdControlFunction != nullptr) {
            UE_LOG(LogTemp, Warning, TEXT("Run fsunction loaded successfully"));
			Runnable = new FCrowdControlRunnable(this);
			Runnable->StartThread();
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("Failed to load Run function"));
        }
		
		//CommandFunction();
       // FString convertedResult = FString(result.c_str());
       // UE_LOG(LogTemp, Log, TEXT("Command Result: %s"), *convertedResult);
		//
		//SetEngine();
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Failed to load DLL"));
    }
}

void ACCManager::Update() {
    std::lock_guard<std::mutex> lock(QueueMutex);
	
	int result = CommandFunction();
	
	char * chrStr = StringTest();
	
	int firstCharAsInt = static_cast<int>(chrStr[0]);
	chrStr = chrStr + 1;
	
	if (firstCharAsInt == 65) {
		UE_LOG(LogTemp, Log, TEXT("%s"), *FString(chrStr));
	} else if (firstCharAsInt == 66) {
		UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(chrStr));
	} else if (firstCharAsInt == 67) {
		UE_LOG(LogTemp, Error, TEXT("%s"), *FString(chrStr));
	} else if (firstCharAsInt == 68) {
		UE_LOG(LogTemp, Log, TEXT("%s"), *FString(chrStr));
	}
	
	if (result == 1) {
		ResetCommand();
		LoginTwitch();
	}
}

void ACCManager::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    Update();
}

ACCManager::~ACCManager()
{
    if (Runnable) {
        Runnable->EnsureCompletion();
        delete Runnable;
    }
}


