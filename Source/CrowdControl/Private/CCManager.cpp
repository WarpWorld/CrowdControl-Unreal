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

ACCManager::AddBasicEffectType ACCManager::AddBasicEffect;

char * ACCManager::StringToSend = new char[100];


typedef char*(*_getCharArray)(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);

_getCharArray m_getCharArrayFromDll;

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
	
	m_getCharArrayFromDll = (_getCharArray)FPlatformProcess::GetDllExport(ACCManager::DLLHandle, TEXT("AddNewBasicEffect"));

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

		SetEngine();
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Failed to load DLL"));
    }

	ACCManager::EffectSuccess("Force Jump");
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

std::string ACCManager::FStringToUtf8String(const FString& InString)
{
    FTCHARToUTF8 Utf8Converter(*InString); // Convert FString (UTF-16) to UTF-8
    return std::string(Utf8Converter.Get(), Utf8Converter.Length());
}

void ACCManager::Effect(TArray<FString> categories, FString name, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable) {
	char** categoriesArray = new char*[categories.Num()];

    for (int32 i = 0; i < categories.Num(); ++i) {
        std::string tempString(TCHAR_TO_UTF8(*categories[i]));
        categoriesArray[i] = new char[tempString.length() + 1]; 
        strcpy(categoriesArray[i], tempString.c_str());
    }

	m_getCharArrayFromDll(TCHAR_TO_UTF8(*name), TCHAR_TO_UTF8(*description), static_cast<int>(price), static_cast<int>(maxRetries), retryDelay, pendingDelay, sellable, visible, nonPoolable, 0, 0, categoriesArray);
}

void ACCManager::TimedEffect(FString id, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, TArray<FString> categories, int32 duration) {
	
}

void ACCManager::ParameterEffect(FString id, FString description, int32 price, int32 maxRetries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, TArray<FString> categories, TArray<UCrowdControlParameter*> parameters) {
    
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


