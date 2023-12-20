#include "CrowdControl.h"
#include "CCManager.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include "Misc/Paths.h"
#include "CrowdControlRunable.h"
#include "CrowdControlParameter.h"

ACCManager::CrowdControlConnectFunctionType ACCManager::ConnectFunction;
ACCManager::CrowdControlDisconnectFunctionType ACCManager::DisconnectFunction;
ACCManager::CrowdControlFunctionType ACCManager::CrowdControlFunction;
void* ACCManager::DLLHandle;
ACCManager* ACCManager::Instance = nullptr;
int ACCManager::Result = 0;

ACCManager::LoginTwitchType ACCManager::LoginTwitchFunction;
ACCManager::LoginDiscordType ACCManager::LoginDiscordFunction;
ACCManager::LoginYoutubeType ACCManager::LoginYoutubeFunction;

typedef char*(*BasicEffectType)(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
BasicEffectType AddBasicEffect;
typedef char*(*TimedEffectType)(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration);
TimedEffectType AddTimedEffect;

typedef char*(*ParameterEffectType)(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
ParameterEffectType AddParameterEffect;
typedef char*(*AddParameterOptionType)(char* name, char* paramName, char** options);
AddParameterOptionType AddParameterOption;
typedef char*(*AddParamaterMinMaxType)(char* name, char* paramName, int min, int max);
AddParamaterMinMaxType AddParamaterMinMax;

ACCManager::ACCManager(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;

    Runnable = nullptr;
	ACCManager::Instance = this;
}

void ACCManager::StartThread() {
	if (CrowdControlFunction != nullptr) {
        UE_LOG(LogTemp, Warning, TEXT("Run fsunction loaded successfully"));
		Runnable = new FCrowdControlRunnable(this);
		Runnable->StartThread();
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Run function"));
    }
}

void ACCManager::LoadDLL(FString GameKey)
{
	/*if (ACCManager::DLLHandle != nullptr) {
		return;
	}*/
	
	FString DLLPath = FPaths::Combine(*FPaths::GameDir(), TEXT("Binaries/Win64/CrowdControl.dll"));
    ACCManager::DLLHandle = FPlatformProcess::GetDllHandle(*DLLPath);
	
	AddBasicEffect = (BasicEffectType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("AddNewBasicEffect"));
	AddTimedEffect = (TimedEffectType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("AddNewTimedEffect"));
	AddParameterEffect = (ParameterEffectType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("AddNewParameterEffect"));
	AddParameterOption = (AddParameterOptionType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("AddParameterOption"));
	AddParamaterMinMax = (AddParamaterMinMaxType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("AddParamaterMinMax"));

    if (ACCManager::DLLHandle != nullptr) { 
		CommandFunction = (FP_Command)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?CommandID@CrowdControlRunner@@QEAAHXZ"));
		
		ACCManager::CrowdControlFunction = (CrowdControlFunctionType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?Run@CrowdControlRunner@@QEAAHXZ"));
		ACCManager::ConnectFunction = (ACCManager::CrowdControlConnectFunctionType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?Connect@CrowdControlRunner@@SAXXZ"));
		ACCManager::DisconnectFunction = (ACCManager::CrowdControlDisconnectFunctionType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?Disconnect@CrowdControlRunner@@SAXXZ"));
		
		ResetCommand = (ResetCommandType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?ResetCommandCode@CrowdControlRunner@@QEAAXXZ"));
		ACCManager::LoginTwitchFunction = (ACCManager::LoginTwitchType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?LoginTwitch@CrowdControlRunner@@SAXXZ"));
		ACCManager::LoginDiscordFunction = (ACCManager::LoginDiscordType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?LoginDiscord@CrowdControlRunner@@SAXXZ"));
		ACCManager::LoginYoutubeFunction = (ACCManager::LoginYoutubeType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?LoginYoutube@CrowdControlRunner@@SAXXZ"));
		StringTest = (StringTestType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?TestCharArray@CrowdControlRunner@@QEAAPEADXZ"));
		
		SetEngine = (SetEngineType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?EngineSet@CrowdControlRunner@@QEAAXXZ"));
		EngineEffect = (EngineEffectType)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("?EngineEffect@CrowdControlRunner@@SAPEADXZ"));
		SetEngine();
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Failed to load DLL"));
    }
	
	//ACCManager::EffectSuccess("Force Jump");
}

void ACCManager::Connect() {
	ACCManager::Instance->StartThread();
}

void ACCManager::Disconnect() {
	Instance->Runnable->EnsureCompletion();
}

void ACCManager::LoginTwitch() {
	ACCManager::LoginTwitchFunction();
}

void ACCManager::LoginYoutube() {
	ACCManager::LoginYoutubeFunction();
}

void ACCManager::LoginDiscord() {
	ACCManager::LoginDiscordFunction();
}

char** ACCManager::SplitCategories(TArray<FString> categories) {
	char** categoriesArray = new char*[categories.Num() + 1];

	for (int32 i = 0; i < categories.Num();  ++i) {
		std::string tempString(TCHAR_TO_UTF8(*categories[i]));
		categoriesArray[i] = new char[tempString.length() + 1];
		strcpy_s(categoriesArray[i], tempString.length() + 1, tempString.c_str());
	}

	categoriesArray[categories.Num()] = nullptr;  

	return categoriesArray; 
}

void ACCManager::Effect(TArray<FString> categories, FString name, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable) {
  	AddBasicEffect(TCHAR_TO_UTF8(*name), TCHAR_TO_UTF8(*description), static_cast<int>(price), static_cast<int>(maxRetries), retryDelay, pendingDelay, sellable, visible, nonPoolable, 0, 0, ACCManager::SplitCategories(categories));
}

void ACCManager::TimedEffect(TArray<FString> categories, FString name, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, float duration) {
	AddTimedEffect(TCHAR_TO_UTF8(*name), TCHAR_TO_UTF8(*description), static_cast<int>(price), static_cast<int>(maxRetries), retryDelay, pendingDelay, sellable, visible, nonPoolable, 0, 0, ACCManager::SplitCategories(categories), duration);
}

void ACCManager::ParameterEffect(FString name, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, TArray<FString> categories, TArray<UCrowdControlParameter*> parameters) {
	AddParameterEffect(TCHAR_TO_UTF8(*name), TCHAR_TO_UTF8(*description), static_cast<int>(price), static_cast<int>(maxRetries), retryDelay, pendingDelay, sellable, visible, nonPoolable, 0, 0, ACCManager::SplitCategories(categories));
 
	for (UCrowdControlParameter* parameter : parameters) {
		if (parameter->_max != 0) {
			AddParamaterMinMax(TCHAR_TO_UTF8(*name), TCHAR_TO_UTF8(*parameter->_name), static_cast<int>(parameter->_min), static_cast<int>(parameter->_max));
		}
		else {
			AddParameterOption(TCHAR_TO_UTF8(*name), TCHAR_TO_UTF8(*parameter->_name), ACCManager::SplitCategories(parameter->_options));
		}
	}
}

void ACCManager::EffectSuccess(FString id) {
	ACCManager::Instance->OnEffectTrigger.Broadcast(id);
}

void ACCManager::EffectFailure(FString id) {
	
}

void ACCManager::EffectPause(FString id) {
	
}

void ACCManager::EffectResume(FString id) {
	
}

void ACCManager::EffectReset(FString id) {
	
}

void ACCManager::EffectStop(FString id) {
	
}

void ACCManager::Update() {
    std::lock_guard<std::mutex> lock(QueueMutex);
	
	ACCManager::Result = CommandFunction();
	
	char * effectManifest = EngineEffect();

	if (effectManifest[0] != '\0') {
		UE_LOG(LogTemp, Log, TEXT("%s"), *FString(effectManifest));
	}

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
}

void ACCManager::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    Update();
}

int32 ACCManager::CrowdControlResult() {
	if (ACCManager::Result != 0) {
		Instance->ResetCommand();
	}
	
    return Result;
}

ACCManager* ACCManager::CrowdControl() {
    return ACCManager::Instance;
}

ACCManager::~ACCManager()
{
    if (Runnable) {
        Runnable->EnsureCompletion();
        delete Runnable;
    }
}


