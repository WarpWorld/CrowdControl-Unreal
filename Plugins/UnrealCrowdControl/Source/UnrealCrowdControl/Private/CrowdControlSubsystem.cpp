#include "CrowdControlSubsystem.h"
#include "Windows/AllowWindowsPlatformTypes.h"

#include "CoreGlobals.h"
#include "CrowdControlDeveloperSettings.h"
#include "Misc/Paths.h"
#include "CrowdControlRunable.h"
#include "CrowdControlFunctionLibrary.h"
#include "CrowdControlLogChannels.h"
#include "Logging/LogMacros.h"
#include "JsonUtilities.h"


UCrowdControlSubsystem::CrowdControlConnectFunctionType UCrowdControlSubsystem::CC_ConnectFunction;
UCrowdControlSubsystem::CrowdControlDisconnectFunctionType UCrowdControlSubsystem::CC_DisconnectFunction;
UCrowdControlSubsystem::CrowdControlFunctionType UCrowdControlSubsystem::CC_CrowdControlFunction;


UCrowdControlSubsystem::LoginTwitchType UCrowdControlSubsystem::CC_LoginTwitchFunction;
UCrowdControlSubsystem::LoginDiscordType UCrowdControlSubsystem::CC_LoginDiscordFunction;
UCrowdControlSubsystem::LoginYoutubeType UCrowdControlSubsystem::CC_LoginYoutubeFunction;

typedef void(*BasicEffectType)(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
BasicEffectType CC_AddBasicEffect;
typedef void(*TimedEffectType)(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration);
TimedEffectType CC_AddTimedEffect;

typedef void(*ParameterEffectType)(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
ParameterEffectType CC_AddParameterEffect;
typedef void(*AddParameterOptionType)(char* name, char* paramName, char** options);
AddParameterOptionType CC_AddParameterOption;
typedef void(*AddParamaterMinMaxType)(char* name, char* paramName, int min, int max);
AddParamaterMinMaxType CC_AddParamaterMinMax;

typedef void(*EffectSuccessFailureType)(char* name);
EffectSuccessFailureType CC_EffectSuccess;
EffectSuccessFailureType CC_EffectFailure;

typedef bool(*EffectStatusChangeType)(const char* effectID);
EffectStatusChangeType CC_StopEffect;
EffectStatusChangeType CC_ResetEffect;
EffectStatusChangeType CC_PauseEffect;
EffectStatusChangeType CC_ResumeEffect;

typedef bool(*EffectIsRunningType)(const char* name);
EffectIsRunningType CC_IsRunning;

typedef bool(*SetGameNameAndPackIDType)(char* name, char* packId);
SetGameNameAndPackIDType CC_SetGameNameAndPackID;

UCrowdControlSubsystem& UCrowdControlSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);
	UCrowdControlSubsystem* Router = UGameInstance::GetSubsystem<UCrowdControlSubsystem>(World->GetGameInstance());
	check(Router);
	return *Router;
}

void UCrowdControlSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Runnable = nullptr;
	MenuJson = "";
	GameJsonObject = MakeShareable(new FJsonObject);
	
	LoadDLL();
	
	// Defer setting the timer until after the world is initialized
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UCrowdControlSubsystem::SetupWorldTimer);
	if(GetWorld())
	{
		SetupWorldTimer(GetWorld(), UWorld::InitializationValues());
	}
}

void UCrowdControlSubsystem::Deinitialize()
{
	MenuJson = "";
	GameJsonObject.Reset();
	if (Runnable) {
		Runnable->EnsureCompletion();
		Runnable.Reset();
	}

	
	Super::Deinitialize();
}


void UCrowdControlSubsystem::PrintEffectsToJsonFile()
{
	// Check if MenuJson is not empty
	if (MenuJson.IsEmpty())
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("MenuJson is empty. Nothing to write to file."));
		return;
	}

	TSharedPtr<FJsonObject> RootJsonObject = MakeShareable(new FJsonObject);


	TSharedPtr<FJsonObject> EffectsJsonObject = MakeShareable(new FJsonObject);


	EffectsJsonObject->SetObjectField("game", GameJsonObject);


	RootJsonObject->SetObjectField("effects", EffectsJsonObject);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), Writer))
	{
		MenuJson = JsonString; 
		UE_LOG(LogCrowdControl, Log, TEXT("Effects JSON: %s"), *JsonString);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize effects JSON."));
	}

	// Get the file path in the Saved folder
	FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CCMenu.json"));

	// Overwrite the file if it already exists, or create it if it doesn't
	if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(LogCrowdControl, Log, TEXT("Successfully wrote MenuJson to file: %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to write MenuJson to file: %s"), *FilePath);
	}
}

void UCrowdControlSubsystem::StartThread() {
	if (CC_CrowdControlFunction != nullptr) {
        UE_LOG(LogTemp, Warning, TEXT("Run fsunction loaded successfully"));
		Runnable = MakeUnique<FCrowdControlRunnable>(this);
		Runnable->StartThread();
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Run function"));
    }
}

void UCrowdControlSubsystem::LoadDLL()
{
	FString DLLPath = FPaths::Combine(*FPaths::ProjectPluginsDir(), TEXT("UnrealCrowdControl/Binaries/Win64/CrowdControl.dll")); 
	UE_LOG(LogCrowdControl, Log, TEXT("Loading Crowd Control DLL from path: %s"), *DLLPath);
	
    DLLHandle = FPlatformProcess::GetDllHandle(*DLLPath);	
    if (DLLHandle != nullptr)
    {
    	CC_AddBasicEffect = (BasicEffectType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("AddNewBasicEffect"));
    	CC_AddTimedEffect = (TimedEffectType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("AddNewTimedEffect"));
    	CC_AddParameterEffect = (ParameterEffectType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("AddNewParameterEffect"));
    	CC_AddParameterOption = (AddParameterOptionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("AddParameterOption"));
    	CC_AddParamaterMinMax = (AddParamaterMinMaxType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("AddParamaterMinMax"));
    	ensure(CC_AddBasicEffect && CC_AddTimedEffect && CC_AddParameterEffect && CC_AddParameterOption && CC_AddParamaterMinMax);
    	
		CC_CommandFunction = (FP_Command)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?CommandID@CrowdControlRunner@@QEAAHXZ"));
		
		CC_CrowdControlFunction = (CrowdControlFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?Run@CrowdControlRunner@@QEAAHXZ"));
		CC_ConnectFunction = (CrowdControlConnectFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?Connect@CrowdControlRunner@@SAXXZ"));
		CC_DisconnectFunction = (CrowdControlDisconnectFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?Disconnect@CrowdControlRunner@@SAXXZ"));
		
		CC_ResetCommand = (ResetCommandType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?ResetCommandCode@CrowdControlRunner@@QEAAXXZ"));
		CC_LoginTwitchFunction = (LoginTwitchType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?LoginTwitch@CrowdControlRunner@@SAXXZ"));
		CC_LoginDiscordFunction = (LoginDiscordType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?LoginDiscord@CrowdControlRunner@@SAXXZ"));
		CC_LoginYoutubeFunction = (LoginYoutubeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?LoginYoutube@CrowdControlRunner@@SAXXZ"));
		CC_StringTest = (StringTestType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?TestCharArray@CrowdControlRunner@@QEAAPEADXZ"));

    	CC_EffectSuccess = (EffectSuccessFailureType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectSuccess"));
		CC_EffectFailure = (EffectSuccessFailureType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectFailure"));
    	
    	CC_StopEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?StopEffect@CrowdControlRunner@@SA_NV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"));
    	CC_ResetEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?ResetEffect@CrowdControlRunner@@SA_NV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"));
    	CC_PauseEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?PauseEffect@CrowdControlRunner@@SA_NV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"));
    	CC_ResumeEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?ResumeEffect@CrowdControlRunner@@SA_NV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"));
    	
    	CC_SetGameNameAndPackID = (SetGameNameAndPackIDType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?SetGameNameAndPackID@CrowdControlRunner@@SAXPEBD0@Z"));
    	
    	// Custom Effects API DLL exports
    	CC_UploadCustomEffects = (UCrowdControlSubsystem::UploadCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("UploadCustomEffects"));
    	CC_ClearCustomEffects = (UCrowdControlSubsystem::ClearCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("ClearCustomEffects"));
    	CC_DeleteCustomEffects = (UCrowdControlSubsystem::DeleteCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("DeleteCustomEffects"));
    	CC_GetCustomEffects = (UCrowdControlSubsystem::GetCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetCustomEffects"));
    	
    	const UCrowdControlDeveloperSettings* Settings = GetDefault<UCrowdControlDeveloperSettings>();
    	if (Settings && CC_SetGameNameAndPackID)
    	{
    		CC_SetGameNameAndPackID(TCHAR_TO_UTF8(*Settings->GameName), TCHAR_TO_UTF8(*Settings->GamePackID));
    	}
    }
    else
    {
        UE_LOG(LogCrowdControl, Error, TEXT("Failed to load DLL %s"), *DLLPath);
    }
	
	//ACCManager::EffectSuccess("Force Jump");
}

void UCrowdControlSubsystem::Connect()
{
	StartThread();
}

void UCrowdControlSubsystem::Disconnect()
{
	if(Runnable)
	{
		Runnable->EnsureCompletion();
		Runnable.Reset();
	}
}

void UCrowdControlSubsystem::ResetConnection()
{
	CC_ResetCommand();
}

void UCrowdControlSubsystem::LoginTwitch()
{
	CC_LoginTwitchFunction();
}

void UCrowdControlSubsystem::LoginYoutube()
{
	CC_LoginYoutubeFunction();
}

void UCrowdControlSubsystem::LoginDiscord()
{
	CC_LoginDiscordFunction();
}

char** UCrowdControlSubsystem::SplitCategories(TArray<FString> categories) {
	char** categoriesArray = new char*[categories.Num() + 1];

	for (int32 i = 0; i < categories.Num();  ++i) {
		std::string tempString(TCHAR_TO_UTF8(*categories[i]));
		categoriesArray[i] = new char[tempString.length() + 1];
		strcpy_s(categoriesArray[i], tempString.length() + 1, tempString.c_str());
	}

	categoriesArray[categories.Num()] = nullptr;  

	return categoriesArray; 
}

void UCrowdControlSubsystem::SetupWorldTimer(UWorld* World, const FWorldInitializationValues IVS)
{
	TickTimerHandle.Invalidate();

	if(!World)
		return;
	
	World->GetTimerManager().SetTimer(TickTimerHandle, this, &ThisClass::OnTimerManagerTick, 0.1f, true);
}

void UCrowdControlSubsystem::OnTimerManagerTick()
{
	if(GetWorld()->IsPaused())
		return;  // do nothing on pause

	const float Delta = GetWorld()->GetDeltaSeconds();
	if(Delta > UE_KINDA_SMALL_NUMBER)
	{
		Tick(Delta);
	}	
}

void UCrowdControlSubsystem::SetupEffect(const FCrowdControlEffectInfo& Info)
{
	if(!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect setup call failed! Currently not initialized!"))
		return;
	}

	// Convert Info to JSON
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("name", Info.displayName);
	JsonObject->SetStringField("description", Info.description);
	JsonObject->SetNumberField("price", Info.price);
	JsonObject->SetNumberField("maxRetries", Info.maxRetries);
	JsonObject->SetNumberField("retryDelay", Info.retryDelay);
	JsonObject->SetNumberField("pendingDelay", Info.pendingDelay);
	JsonObject->SetBoolField("sellable", Info.sellable);
	JsonObject->SetBoolField("visible", Info.visible);
	JsonObject->SetBoolField("nonPoolable", Info.nonPoolable);

	// Assuming SplitCategories returns an array of strings
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FString& Category : Info.categories)
	{
		CategoriesArray.Add(MakeShareable(new FJsonValueString(Category)));
	}
	JsonObject->SetArrayField("categories", CategoriesArray);

	GameJsonObject->SetObjectField(Info.id, JsonObject);

	// Convert JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		MenuJson += JsonString;
		UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	}
	
	CC_AddBasicEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.description), Info.price, Info.maxRetries, Info.retryDelay, Info.pendingDelay, Info.sellable, Info.visible, Info.nonPoolable, 0, 0, SplitCategories(Info.categories));
}

void UCrowdControlSubsystem::SetupTimedEffect(const FCrowdControlTimedEffectInfo& Info)
{
	if(!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect setup call failed! Currently not initialized!"))
		return;
	}
	// Convert Info to JSON
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("name", Info.displayName);
	JsonObject->SetStringField("description", Info.description);
	JsonObject->SetNumberField("price", Info.price);
	JsonObject->SetNumberField("maxRetries", Info.maxRetries);
	JsonObject->SetNumberField("retryDelay", Info.retryDelay);
	JsonObject->SetNumberField("pendingDelay", Info.pendingDelay);
	JsonObject->SetBoolField("sellable", Info.sellable);
	JsonObject->SetBoolField("visible", Info.visible);
	JsonObject->SetBoolField("nonPoolable", Info.nonPoolable);

	// Assuming SplitCategories returns an array of strings
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FString& Category : Info.categories)
	{
		CategoriesArray.Add(MakeShareable(new FJsonValueString(Category)));
	}
	JsonObject->SetArrayField("categories", CategoriesArray);
	JsonObject->SetNumberField("duration", Info.duration);
	GameJsonObject->SetObjectField(Info.id, JsonObject);
	// Convert JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		MenuJson += JsonString;
		UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	}
	CC_AddTimedEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.description), Info.price, Info.maxRetries, Info.retryDelay, Info.pendingDelay, Info.sellable, Info.visible, Info.nonPoolable, 0, 0, SplitCategories(Info.categories), Info.duration);
}

void UCrowdControlSubsystem::SetupParameterEffect(const FCrowdControlParameterEffectInfo& Info)
{
	if(!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect setup call failed! Currently not initialized!"))
		return;
	}
	// Convert Info to JSON
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("name", Info.displayName);
	JsonObject->SetStringField("description", Info.description);
	JsonObject->SetNumberField("price", Info.price);
	JsonObject->SetNumberField("maxRetries", Info.maxRetries);
	JsonObject->SetNumberField("retryDelay", Info.retryDelay);
	JsonObject->SetNumberField("pendingDelay", Info.pendingDelay);
	JsonObject->SetBoolField("sellable", Info.sellable);
	JsonObject->SetBoolField("visible", Info.visible);
	JsonObject->SetBoolField("nonPoolable", Info.nonPoolable);

	// Assuming SplitCategories returns an array of strings
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FString& Category : Info.categories)
	{
		CategoriesArray.Add(MakeShareable(new FJsonValueString(Category)));
	}
	JsonObject->SetArrayField("categories", CategoriesArray);
	
	CC_AddParameterEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.description), Info.price, Info.maxRetries, Info.retryDelay, Info.pendingDelay, Info.sellable, Info.visible, Info.nonPoolable, 0, 0, SplitCategories(Info.categories));
	TArray<TSharedPtr<FJsonValue>> ParametersArray;
	for (const FCrowdControlParameter& parameter : Info.parameters)
	{
		TSharedPtr<FJsonObject> ParameterObject = MakeShareable(new FJsonObject);
		ParameterObject->SetStringField("_id", parameter._id);
		if (parameter._max != 0)
		{
			ParameterObject->SetNumberField("_min", parameter._min);
			ParameterObject->SetNumberField("_max", parameter._max);
			CC_AddParamaterMinMax(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*parameter._id), parameter._min, parameter._max);
		}
		else
		{
			TArray<TSharedPtr<FJsonValue>> OptionsArray;
			for (const FString& Option : parameter._options)
			{
				OptionsArray.Add(MakeShareable(new FJsonValueString(Option)));
			}
			ParameterObject->SetArrayField("_options", OptionsArray);

			CC_AddParameterOption(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*parameter._id), SplitCategories(parameter._options));
		}
		ParametersArray.Add(MakeShareable(new FJsonValueObject(ParameterObject)));
	}
	JsonObject->SetArrayField("parameters", ParametersArray);
	GameJsonObject->SetObjectField(Info.id, JsonObject);
	// Convert JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		MenuJson += JsonString;
		UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	}
}

void UCrowdControlSubsystem::EffectSuccess(FString id)
{
	if(!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect success call failed! Currently not initialized!"))
		return;
	}
	
	CC_EffectSuccess(TCHAR_TO_UTF8(*id));
	LastSuccessfulEffectID = id;
}

void UCrowdControlSubsystem::EffectFailure(FString id)
{
	if(!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect failure call failed! Currently not initialized!"))
		return;
	}
	
	CC_EffectFailure(TCHAR_TO_UTF8(*id));
}

bool UCrowdControlSubsystem::IsEffectRunning(FString name)
{
	return CC_IsRunning(TCHAR_TO_UTF8(*name));
}

void UCrowdControlSubsystem::PauseEffect(FString id)
{
	CC_PauseEffect(TCHAR_TO_UTF8(*id));
}

void UCrowdControlSubsystem::ResumeEffect(FString id)
{
	CC_ResumeEffect(TCHAR_TO_UTF8(*id));
}

void UCrowdControlSubsystem::ResetEffect(FString id)
{
	CC_ResetEffect(TCHAR_TO_UTF8(*id));
}

void UCrowdControlSubsystem::StopEffect(FString id)
{
	CC_StopEffect(TCHAR_TO_UTF8(*id));
}

void UCrowdControlSubsystem::Update() {
    std::lock_guard<std::mutex> lock(QueueMutex);
	
	int32 CurrentResult = CC_CommandFunction();
	if(CurrentResult != CommandID)
	{
		bool bWasInitialized = bIsInitialized;
		CommandID = CurrentResult;
		bIsConnected = CommandID >= 2;
		bIsInitialized = CommandID > 2;
		
		// Clear custom effects when first authenticated to remove any leftover effects from previous sessions
		// This ensures a clean slate when a new session starts
		if (!bWasInitialized && bIsInitialized)
		{
			UE_LOG(LogCrowdControl, Log, TEXT("Crowd Control authenticated (Connected: %d, Initialized: %d). Clearing any leftover custom effects from previous session."), 
				bIsConnected, bIsInitialized);
			// if (CC_ClearCustomEffects != nullptr)
			// {
			// 	CC_ClearCustomEffects();
			// }
		}
		
		OnCommandIDChanged.Broadcast(CommandID);
	}

	// Check Queued events
	char * effectManifest = CC_EngineEffect();
	if (effectManifest[0] != '\0')
	{
		UE_LOG(LogCrowdControl, Verbose, TEXT("%s"), *FString(effectManifest));
		FString JsonString = FString(effectManifest);

		// Parse JSON string
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()) 
		{
			// Extract "id" and "name"
			FString Name;
			FString Id;
			
			if (JsonObject->TryGetStringField(TEXT("name"), Name) && JsonObject->TryGetStringField(TEXT("id"), Id)) 
			{
				UE_LOG(LogCrowdControl, Log, TEXT("Effect Name: %s   Effect ID: %s"), *Name, *Id);

				FString Duration, ParameterValue;
				if (JsonObject->TryGetStringField(TEXT("duration"), Duration) && Duration.IsNumeric())
				{
					OnTimedEffectTrigger.Broadcast(Id, Name, FCString::Atof(*Duration) / 1000);
				}
				else if (JsonObject->TryGetStringField(TEXT("value"), ParameterValue))
				{
					OnParameterEffectTrigger.Broadcast(Id, Name, ParameterValue);
				}
				else
				{
					OnEffectTrigger.Broadcast(Id, Name);
				}
				
				if(LastSuccessfulEffectID != Id)
				{
					// If no calls to EffectSuccess() was made due to OnEffectTrigger broadcast, we need to notify failure.
					EffectFailure(Id);
				}
			} 
			else 
			{
				UE_LOG(LogCrowdControl, Warning, TEXT("Failed to find 'name' or 'id' in JSON"));
			}
		} 
		else 
		{
			UE_LOG(LogCrowdControl, Error, TEXT("Failed to parse JSON: %s"), *JsonString);
		}
	}

	char * chrStr = CC_StringTest();
	
	int firstCharAsInt = chrStr[0];
	chrStr = chrStr + 1;
	
	if (firstCharAsInt == 65) {
		UE_LOG(LogCrowdControl, Log, TEXT("%s"), *FString(chrStr));
	} else if (firstCharAsInt == 66) {
		UE_LOG(LogCrowdControl, Warning, TEXT("%s"), *FString(chrStr));
	} else if (firstCharAsInt == 67) {
		UE_LOG(LogCrowdControl, Error, TEXT("%s"), *FString(chrStr));
	} else if (firstCharAsInt == 68) {
		UE_LOG(LogCrowdControl, Log, TEXT("%s"), *FString(chrStr));
	}
}

void UCrowdControlSubsystem::Tick(float DeltaTime) {
    Update();
}

void UCrowdControlSubsystem::UploadCustomEffects()
{
	if (!bIsConnected)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomEffects call failed! Not connected to Crowd Control. Call Connect() first."));
		return;
	}

	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomEffects call failed! Not authenticated. Please login to Crowd Control first."));
		return;
	}

	if (CC_UploadCustomEffects == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("UploadCustomEffects DLL function not loaded!"));
		return;
	}

	if (!GameJsonObject.IsValid())
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("GameJsonObject is not valid. No custom effects to upload."));
		return;
	}

	// Convert GameJsonObject to JSON string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(GameJsonObject.ToSharedRef(), Writer))
	{
		CC_UploadCustomEffects(TCHAR_TO_UTF8(*JsonString));
		UE_LOG(LogCrowdControl, Log, TEXT("Uploaded custom effects: %s"), *JsonString);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize effects JSON for upload."));
	}
}

void UCrowdControlSubsystem::ClearCustomEffects()
{
	if (!bIsConnected)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl ClearCustomEffects call failed! Not connected to Crowd Control. Call Connect() first."));
		return;
	}

	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl ClearCustomEffects call failed! Not authenticated. Please login to Crowd Control first."));
		return;
	}

	if (CC_ClearCustomEffects == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("ClearCustomEffects DLL function not loaded!"));
		return;
	}

	CC_ClearCustomEffects();
	UE_LOG(LogCrowdControl, Log, TEXT("Cleared all custom effects."));
}

void UCrowdControlSubsystem::DeleteCustomEffects(const TArray<FString>& EffectIDs)
{
	if (!bIsConnected)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl DeleteCustomEffects call failed! Not connected to Crowd Control. Call Connect() first."));
		return;
	}

	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl DeleteCustomEffects call failed! Not authenticated. Please login to Crowd Control first."));
		return;
	}

	if (CC_DeleteCustomEffects == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("DeleteCustomEffects DLL function not loaded!"));
		return;
	}

	// Convert TArray<FString> to JSON array string
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	for (const FString& EffectID : EffectIDs)
	{
		JsonArray.Add(MakeShareable(new FJsonValueString(EffectID)));
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonArray, Writer))
	{
		CC_DeleteCustomEffects(TCHAR_TO_UTF8(*JsonString));
		UE_LOG(LogCrowdControl, Log, TEXT("Deleted custom effects: %s"), *JsonString);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize effect IDs JSON for deletion."));
	}
}

void UCrowdControlSubsystem::RemoveCustomEffect(FString EffectID)
{
	TArray<FString> EffectIDs;
	EffectIDs.Add(EffectID);
	DeleteCustomEffects(EffectIDs);
}

FString UCrowdControlSubsystem::GetCustomEffects()
{
	if (!bIsConnected)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl GetCustomEffects call failed! Not connected to Crowd Control. Call Connect() first."));
		return FString();
	}

	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl GetCustomEffects call failed! Not authenticated. Please login to Crowd Control first."));
		return FString();
	}

	if (CC_GetCustomEffects == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("GetCustomEffects DLL function not loaded!"));
		return FString();
	}

	char* Result = CC_GetCustomEffects();
	if (Result != nullptr)
	{
		FString ResultString = FString(UTF8_TO_TCHAR(Result));
		UE_LOG(LogCrowdControl, Log, TEXT("Retrieved custom effects: %s"), *ResultString);
		return ResultString;
	}

	return FString();
}

UCrowdControlSubsystem::~UCrowdControlSubsystem()
{
	MenuJson = "";
    Disconnect();
}




