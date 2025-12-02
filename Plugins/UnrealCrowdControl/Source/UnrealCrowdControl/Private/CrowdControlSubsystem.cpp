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

typedef void(*BasicEffectType)(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
BasicEffectType CC_AddBasicEffect;
typedef void(*TimedEffectType)(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray, float duration);
TimedEffectType CC_AddTimedEffect;

typedef void(*ParameterEffectType)(char* id, char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray);
ParameterEffectType CC_AddParameterEffect;
typedef void(*AddParameterOptionType)(char* name, char* paramName, char** options);
AddParameterOptionType CC_AddParameterOption;
typedef void(*AddParameterMinMaxType)(char* name, char* paramName, int min, int max);
AddParameterMinMaxType CC_AddParameterMinMax;

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


FString UCrowdControlSubsystem::GetOriginID()
{
	return FString(UTF8_TO_TCHAR(CC_GetOriginID()));
}


FString UCrowdControlSubsystem::GetProfileType()
{
	return FString(UTF8_TO_TCHAR(CC_GetProfileType()));
}


FString UCrowdControlSubsystem::GetInteractionURL()
{
	return FString(UTF8_TO_TCHAR(CC_GetInteractionURL()));
}


FString UCrowdControlSubsystem::GetStreamerName()
{
	return FString(UTF8_TO_TCHAR(CC_GetStreamerName()));
}


bool UCrowdControlSubsystem::GetIsJWTTokenValid()
{
	return CC_IsJWTTokenValid();
}

// Helper function to convert FCrowdControlEffectInfo to JSON
static TSharedPtr<FJsonObject> EffectInfoToJson(const FCrowdControlEffectInfo& Info)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("name", Info.displayName);
	JsonObject->SetStringField("description", Info.description);
	JsonObject->SetNumberField("price", Info.price);
	
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FString& Category : Info.category)
	{
		CategoriesArray.Add(MakeShareable(new FJsonValueString(Category)));
	}
	JsonObject->SetArrayField("category", CategoriesArray);
	
	return JsonObject;
}

// Helper function to convert FCrowdControlTimedEffectInfo to JSON
static TSharedPtr<FJsonObject> TimedEffectInfoToJson(const FCrowdControlTimedEffectInfo& Info)
{
	TSharedPtr<FJsonObject> JsonObject = EffectInfoToJson(Info);
	
	TSharedPtr<FJsonObject> DurationObject = MakeShareable(new FJsonObject);
	DurationObject->SetNumberField("value", Info.duration);
	JsonObject->SetObjectField("duration", DurationObject);
	
	return JsonObject;
}

// Helper function to convert FCrowdControlParameterEffectInfo to JSON
static TSharedPtr<FJsonObject> ParameterEffectInfoToJson(const FCrowdControlParameterEffectInfo& Info)
{
	TSharedPtr<FJsonObject> JsonObject = EffectInfoToJson(Info);
	
	if (Info.RequiresQuantity)
	{
		TSharedPtr<FJsonObject> QuantityObject = MakeShareable(new FJsonObject);
		QuantityObject->SetNumberField("min", Info.quantity.GetLowerBoundValue());
		QuantityObject->SetNumberField("max", Info.quantity.GetUpperBoundValue());
		JsonObject->SetObjectField("quantity", QuantityObject);
	}
	
	TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
	for (const FCrowdControlParameter& parameter : Info.parameters)
	{
		TSharedPtr<FJsonObject> ParameterObject = MakeShareable(new FJsonObject);
		ParameterObject->SetStringField("name", parameter.name);
		ParameterObject->SetStringField("type", parameter.type == ECrowdControlParamType::OPTIONS ? "options" : "hex-color");
		
		if (parameter.type == ECrowdControlParamType::OPTIONS)
		{
			TSharedPtr<FJsonObject> ParameterOptionsObject = MakeShareable(new FJsonObject);
			for (const FCrowdControlParamOption& option : parameter._options)
			{
				TSharedPtr<FJsonObject> OptionObject = MakeShareable(new FJsonObject);
				OptionObject->SetStringField("name", option.DisplayName);
				ParameterOptionsObject->SetObjectField(option.id, OptionObject);
			}
			ParameterObject->SetObjectField("options", ParameterOptionsObject);
		}
		
		ParametersObject->SetObjectField(parameter._id, ParameterObject);
	}
	JsonObject->SetObjectField("parameters", ParametersObject);
	
	return JsonObject;
}

void UCrowdControlSubsystem::UploadCustomEffectsJson(const FString& EffectsJson)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomEffectsJson call failed! Currently not initialized!"));
		return;
	}

	if (CC_UploadCustomEffects != nullptr)
	{
		CC_UploadCustomEffects(TCHAR_TO_UTF8(*EffectsJson));
		UE_LOG(LogCrowdControl, Log, TEXT("UploadCustomEffectsJson called with JSON: %s"), *EffectsJson);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_UploadCustomEffects function pointer is null!"));
	}
}


void UCrowdControlSubsystem::UploadCustomEffect(const FCrowdControlEffectInfo& EffectInfo)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomEffect call failed! Currently not initialized!"));
		return;
	}

	TSharedPtr<FJsonObject> EffectsObject = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> EffectJson = EffectInfoToJson(EffectInfo);
	EffectsObject->SetObjectField(EffectInfo.id, EffectJson);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(EffectsObject.ToSharedRef(), Writer))
	{
		UploadCustomEffectsJson(JsonString);
		UE_LOG(LogCrowdControl, Log, TEXT("UploadCustomEffect: Uploaded effect with ID: %s"), *EffectInfo.id);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize effect to JSON for ID: %s"), *EffectInfo.id);
	}
}

void UCrowdControlSubsystem::UploadCustomTimedEffect(const FCrowdControlTimedEffectInfo& EffectInfo)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomTimedEffect call failed! Currently not initialized!"));
		return;
	}

	TSharedPtr<FJsonObject> EffectsObject = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> EffectJson = TimedEffectInfoToJson(EffectInfo);
	EffectsObject->SetObjectField(EffectInfo.id, EffectJson);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(EffectsObject.ToSharedRef(), Writer))
	{
		UploadCustomEffectsJson(JsonString);
		UE_LOG(LogCrowdControl, Log, TEXT("UploadCustomTimedEffect: Uploaded timed effect with ID: %s"), *EffectInfo.id);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize timed effect to JSON for ID: %s"), *EffectInfo.id);
	}
}

void UCrowdControlSubsystem::UploadCustomParameterEffect(const FCrowdControlParameterEffectInfo& EffectInfo)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomParameterEffect call failed! Currently not initialized!"));
		return;
	}

	TSharedPtr<FJsonObject> EffectsObject = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> EffectJson = ParameterEffectInfoToJson(EffectInfo);
	EffectsObject->SetObjectField(EffectInfo.id, EffectJson);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(EffectsObject.ToSharedRef(), Writer))
	{
		UploadCustomEffectsJson(JsonString);
		UE_LOG(LogCrowdControl, Log, TEXT("UploadCustomParameterEffect: Uploaded parameter effect with ID: %s"), *EffectInfo.id);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize parameter effect to JSON for ID: %s"), *EffectInfo.id);
	}
}

void UCrowdControlSubsystem::UploadCustomEffectsArray(const TArray<FCrowdControlEffectInfo>& Effects)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomEffectsArray call failed! Currently not initialized!"));
		return;
	}

	if (Effects.Num() == 0)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("UploadCustomEffectsArray called with empty array"));
		return;
	}

	TSharedPtr<FJsonObject> EffectsObject = MakeShareable(new FJsonObject);
	
	for (const FCrowdControlEffectInfo& Effect : Effects)
	{
		TSharedPtr<FJsonObject> EffectJson = EffectInfoToJson(Effect);
		EffectsObject->SetObjectField(Effect.id, EffectJson);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(EffectsObject.ToSharedRef(), Writer))
	{
		UploadCustomEffectsJson(JsonString);
		UE_LOG(LogCrowdControl, Log, TEXT("UploadCustomEffectsArray: Uploaded %d effects in a single PUT request"), Effects.Num());
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize effects array to JSON"));
	}
}

void UCrowdControlSubsystem::UploadCustomTimedEffectsArray(const TArray<FCrowdControlTimedEffectInfo>& Effects)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomTimedEffectsArray call failed! Currently not initialized!"));
		return;
	}

	if (Effects.Num() == 0)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("UploadCustomTimedEffectsArray called with empty array"));
		return;
	}

	TSharedPtr<FJsonObject> EffectsObject = MakeShareable(new FJsonObject);
	
	for (const FCrowdControlTimedEffectInfo& Effect : Effects)
	{
		TSharedPtr<FJsonObject> EffectJson = TimedEffectInfoToJson(Effect);
		EffectsObject->SetObjectField(Effect.id, EffectJson);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(EffectsObject.ToSharedRef(), Writer))
	{
		UploadCustomEffectsJson(JsonString);
		UE_LOG(LogCrowdControl, Log, TEXT("UploadCustomTimedEffectsArray: Uploaded %d timed effects in a single PUT request"), Effects.Num());
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize timed effects array to JSON"));
	}
}

void UCrowdControlSubsystem::UploadCustomParameterEffectsArray(const TArray<FCrowdControlParameterEffectInfo>& Effects)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl UploadCustomParameterEffectsArray call failed! Currently not initialized!"));
		return;
	}

	if (Effects.Num() == 0)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("UploadCustomParameterEffectsArray called with empty array"));
		return;
	}

	TSharedPtr<FJsonObject> EffectsObject = MakeShareable(new FJsonObject);
	
	for (const FCrowdControlParameterEffectInfo& Effect : Effects)
	{
		TSharedPtr<FJsonObject> EffectJson = ParameterEffectInfoToJson(Effect);
		EffectsObject->SetObjectField(Effect.id, EffectJson);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(EffectsObject.ToSharedRef(), Writer))
	{
		UploadCustomEffectsJson(JsonString);
		UE_LOG(LogCrowdControl, Log, TEXT("UploadCustomParameterEffectsArray: Uploaded %d parameter effects in a single PUT request"), Effects.Num());
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("Failed to serialize parameter effects array to JSON"));
	}
}

void UCrowdControlSubsystem::ClearCustomEffects()
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl ClearCustomEffects call failed! Currently not initialized!"));
		return;
	}

	if (CC_ClearCustomEffects != nullptr)
	{
		CC_ClearCustomEffects();
		UE_LOG(LogCrowdControl, Log, TEXT("ClearCustomEffects called"));
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_ClearCustomEffects function pointer is null!"));
	}
}

void UCrowdControlSubsystem::DeleteCustomEffects(const FString& EffectIDsJson)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl DeleteCustomEffects call failed! Currently not initialized!"));
		return;
	}

	if (CC_DeleteCustomEffects != nullptr)
	{
		CC_DeleteCustomEffects(TCHAR_TO_UTF8(*EffectIDsJson));
		UE_LOG(LogCrowdControl, Log, TEXT("DeleteCustomEffects called with JSON: %s"), *EffectIDsJson);
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_DeleteCustomEffects function pointer is null!"));
	}
}

FString UCrowdControlSubsystem::GetCustomEffects()
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl GetCustomEffects call failed! Currently not initialized!"));
		return FString();
	}

	if (CC_GetCustomEffects != nullptr)
	{
		char* result = CC_GetCustomEffects();
		if (result != nullptr && result[0] != '\0')
		{
			FString ResultString = FString(UTF8_TO_TCHAR(result));
			// Note: The DLL allocates memory for the result, but we don't have a way to free it from Unreal
			// This is a potential memory leak, but matches the pattern used by other functions like GetOriginID
			UE_LOG(LogCrowdControl, Log, TEXT("GetCustomEffects returned: %s"), *ResultString);
			return ResultString;
		}
		else
		{
			UE_LOG(LogCrowdControl, Warning, TEXT("GetCustomEffects returned empty result"));
			return FString();
		}
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_GetCustomEffects function pointer is null!"));
		return FString();
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
    	CC_AddParameterMinMax = (AddParameterMinMaxType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("AddParameterMinMax"));
    	ensure(CC_AddBasicEffect && CC_AddTimedEffect && CC_AddParameterEffect && CC_AddParameterOption && CC_AddParameterMinMax);
    	
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
    	CC_IsRunning = (EffectIsRunningType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?IsRunning@CrowdControlRunner@@SA_NV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"));
		ensure(CC_StopEffect && CC_ResetEffect && CC_PauseEffect && CC_ResumeEffect && CC_IsRunning);
    	
    	CC_SetGameNameAndPackID = (SetGameNameAndPackIDType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?SetGameNameAndPackID@CrowdControlRunner@@SAXPEAD0@Z"));
    	ensure(CC_SetGameNameAndPackID);

		CC_GetOriginID = (GetOriginIDType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetOriginID"));
		CC_GetProfileType = (GetProfileTypeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetProfileType"));
		CC_GetInteractionURL = (GetInteractionURLType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetInteractionURL"));
		CC_GetStreamerName = (GetStreamerNameType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetStreamerName"));
		CC_IsJWTTokenValid = (IsJWTTokenValidType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("IsJWTTokenValid"));
    	
		CC_UploadCustomEffects = (UploadCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("UploadCustomEffects"));
		CC_ClearCustomEffects = (ClearCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("ClearCustomEffects"));
		CC_DeleteCustomEffects = (DeleteCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("DeleteCustomEffects"));
		CC_GetCustomEffects = (GetCustomEffectsType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetCustomEffects"));
		ensure(CC_UploadCustomEffects && CC_ClearCustomEffects && CC_DeleteCustomEffects && CC_GetCustomEffects);
    	
		CC_SetEngine = (SetEngineType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?EngineSet@CrowdControlRunner@@QEAAXXZ"));
		CC_EngineEffect = (EngineEffectType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("?EngineEffect@CrowdControlRunner@@SAPEADXZ"));
		CC_SetEngine();

    	// Set GamePackID and GameName from developer settings
    	const UCrowdControlDeveloperSettings* Settings = GetDefault<UCrowdControlDeveloperSettings>();
    	if(Settings)
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
	// Assuming SplitCategories returns an array of strings
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FString& Category : Info.category)
	{
		CategoriesArray.Add(MakeShareable(new FJsonValueString(Category)));
	}
	JsonObject->SetArrayField("category", CategoriesArray);
	GameJsonObject->SetObjectField(Info.id, JsonObject);
	

	// Convert JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		MenuJson += JsonString;
		UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	}
	
	CC_AddBasicEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.displayName), TCHAR_TO_UTF8(*Info.description), Info.price, 0, 0, 0, 0, 1, 0, 0, 0, SplitCategories(Info.category));
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

	// Assuming SplitCategories returns an array of strings
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FString& Category : Info.category)
	{
		CategoriesArray.Add(MakeShareable(new FJsonValueString(Category)));
	}
	JsonObject->SetArrayField("category", CategoriesArray);
	TSharedPtr<FJsonObject> JsonDurationObject = MakeShareable(new FJsonObject);
	JsonDurationObject->SetNumberField("value", Info.duration);
	JsonObject->SetObjectField("duration", JsonDurationObject);
	
	GameJsonObject->SetObjectField(Info.id, JsonObject);
	// Convert JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		MenuJson += JsonString;
		UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	}
	CC_AddTimedEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.displayName), TCHAR_TO_UTF8(*Info.description), Info.price, 0, 0, 0, 0, 1, 0, 0, 0, SplitCategories(Info.category), Info.duration);
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

	// Assuming SplitCategories returns an array of strings
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	for (const FString& Category : Info.category)	
	{
		CategoriesArray.Add(MakeShareable(new FJsonValueString(Category)));
	}
	JsonObject->SetArrayField("category", CategoriesArray);
	
	
	CC_AddParameterEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.displayName), TCHAR_TO_UTF8(*Info.description), Info.price, 0, 0, 0, 0, 1, 0, 0, 0, SplitCategories(Info.category));

	if (Info.RequiresQuantity)
	{
		TSharedPtr<FJsonObject> QuantityObject = MakeShareable(new FJsonObject);
		QuantityObject->SetNumberField("min", Info.quantity.GetLowerBoundValue());
		QuantityObject->SetNumberField("max", Info.quantity.GetUpperBoundValue());
		JsonObject->SetObjectField("quantity", QuantityObject);
		CC_AddParameterMinMax(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*FString("quantity")), Info.quantity.GetLowerBoundValue(), Info.quantity.GetUpperBoundValue());
	}

	TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
	for (const FCrowdControlParameter& parameter : Info.parameters)
	{
		TSharedPtr<FJsonObject> ParameterObject = MakeShareable(new FJsonObject);
		ParameterObject->SetStringField("name", parameter.name);
		ParameterObject->SetStringField("type", parameter.type == ECrowdControlParamType::OPTIONS?"options":"hex-color");
		if (parameter.type == ECrowdControlParamType::OPTIONS)
		{
			TSharedPtr<FJsonObject> ParameterOptionsObject = MakeShareable(new FJsonObject);
			TArray<FString> parameterIDs;
			for (const FCrowdControlParamOption& ObjectChoice : parameter._options)
			{
				TSharedPtr<FJsonObject> ParameterOptionObject = MakeShareable(new FJsonObject);
				ParameterOptionObject->SetStringField("name", ObjectChoice.DisplayName);
				ParameterOptionsObject->SetObjectField(ObjectChoice.id, ParameterOptionObject);
				parameterIDs.Add(ObjectChoice.id);
			}
			ParameterObject->SetObjectField("options", ParameterOptionsObject);
			CC_AddParameterOption(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*parameter._id), SplitCategories(parameterIDs));
		}
		ParametersObject->SetObjectField(parameter._id, ParameterObject);
	}
	JsonObject->SetObjectField("parameters", ParametersObject);
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
		CommandID = CurrentResult;
		bIsConnected = CommandID >= 2;
		bIsInitialized = CommandID > 2;
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
				const TSharedPtr<FJsonObject>* ParamsObject;
				int32 Quantity = 1;
				if (JsonObject->TryGetStringField(TEXT("duration"), Duration) && Duration.IsNumeric())
				{
					OnTimedEffectTrigger.Broadcast(Id, Name, FCString::Atof(*Duration));
				}
				else if (JsonObject->TryGetObjectField(TEXT("params"), ParamsObject))
				{
					JsonObject->TryGetNumberField(TEXT("quantity"), Quantity);
					FJsonObjectWrapper ParamsWrapped;
					ParamsWrapped.JsonObject = *ParamsObject;
					OnParameterEffectTrigger.Broadcast(Id, Name, FString::FromInt(Quantity), ParamsWrapped);
				}
				else if (JsonObject->TryGetNumberField(TEXT("quantity"), Quantity))
				{
					FJsonObjectWrapper ParamsWrapped;
					OnParameterEffectTrigger.Broadcast(Id, Name, FString::FromInt(Quantity), ParamsWrapped);
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

	char* chrStr = CC_StringTest();
	
	// Check for null pointer
	if (chrStr == nullptr) {
		return;
	}
	
	// Check if string is empty
	if (chrStr[0] == '\0') {
		// Memory was allocated, but we should still free it
		// Note: The DLL allocates this, but we don't have a way to free it safely
		// This is a known limitation matching the pattern used by other functions
		return;
	}
	
	int firstCharAsInt = static_cast<unsigned char>(chrStr[0]);
	const char* messageStr = chrStr + 1;
	
	// Verify the message string is valid before using it
	if (messageStr != nullptr && messageStr[0] != '\0') {
		FString MessageString = FString(UTF8_TO_TCHAR(messageStr));
		
		if (firstCharAsInt == 65) {
			UE_LOG(LogCrowdControl, Log, TEXT("%s"), *MessageString);
		} else if (firstCharAsInt == 66) {
			UE_LOG(LogCrowdControl, Warning, TEXT("%s"), *MessageString);
		} else if (firstCharAsInt == 67) {
			UE_LOG(LogCrowdControl, Error, TEXT("%s"), *MessageString);
		} else if (firstCharAsInt == 68) {
			UE_LOG(LogCrowdControl, Log, TEXT("%s"), *MessageString);
		}
	}
	
	// Note: Memory allocated by DLL cannot be safely freed from Unreal
	// This matches the pattern used by other functions like GetOriginID
}

void UCrowdControlSubsystem::Tick(float DeltaTime) {
    Update();
}

UCrowdControlSubsystem::~UCrowdControlSubsystem()
{
	MenuJson = "";
    Disconnect();
}




