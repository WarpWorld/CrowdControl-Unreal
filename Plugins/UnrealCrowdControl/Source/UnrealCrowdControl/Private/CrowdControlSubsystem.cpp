#include "CrowdControlSubsystem.h"
#include "Windows/AllowWindowsPlatformTypes.h"

#include "CoreGlobals.h"
#include "CrowdControlDeveloperSettings.h"
#include "CrowdControlManifest.h"
#include "CrowdControlSessionStartError.h"
#include "Misc/Paths.h"
#include "CrowdControlRunable.h"
#include "CrowdControlEffectComponent.h"
#include "CrowdControlFunctionLibrary.h"
#include "CrowdControlLogChannels.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Logging/LogMacros.h"
#include "JsonUtilities.h"
#include "Widgets/Notifications/SNotificationList.h"

#include <memory>
#include <string>
#include <unordered_map>

// Owned by CrowdControl.dll. Only ever used through pointers here: the DLL is resolved at runtime in
// LoadDLL(), so nothing in this module may be statically linked against it.
class CCEffectBase;

namespace
{
	using FCCEffectMap = std::unordered_map<std::string, std::shared_ptr<CCEffectBase>>;

	// Decorated names of the CrowdControl.dll exports backing the effect toggles. Unlike the rest of the
	// API these are C++ members rather than extern "C" wrappers, so they have to be looked up decorated.
	const TCHAR* const CCEffectsMapExport = TEXT("?effects@CrowdControlRunner@@2V?$unordered_map@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$shared_ptr@VCCEffectBase@@@2@U?$hash@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@U?$equal_to@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$shared_ptr@VCCEffectBase@@@2@@std@@@2@@std@@A");
	const TCHAR* const CCToggleVisibleExport = TEXT("?ToggleVisible@CCEffectBase@@QEAAX_N@Z");
	const TCHAR* const CCToggleSellableExport = TEXT("?ToggleSellable@CCEffectBase@@QEAAX_N@Z");

	// Toggle is CCEffectBase::ToggleVisible or ::ToggleSellable, called with the effect passed explicitly
	// as the this pointer. On x64 MSVC member calls use the same convention with this in RCX.
	void ToggleEffectsByIDs(const TArray<FString>& EffectIDs, FCCEffectMap* Effects, void (*Toggle)(void*, bool), bool bValue, const TCHAR* ActionName)
	{
		if (Effects == nullptr || Toggle == nullptr)
		{
			UE_LOG(LogCrowdControl, Warning, TEXT("%s failed. The loaded CrowdControl.dll does not export the effect toggles."), ActionName);
			return;
		}

		if (EffectIDs.Num() == 0)
		{
			for (auto& EffectPair : *Effects)
			{
				if (EffectPair.second)
				{
					Toggle(EffectPair.second.get(), bValue);
				}
			}

			UE_LOG(LogCrowdControl, Log, TEXT("%s applied to all registered effects."), ActionName);
			return;
		}

		for (const FString& EffectID : EffectIDs)
		{
			auto EffectIt = Effects->find(TCHAR_TO_UTF8(*EffectID));
			if (EffectIt == Effects->end() || !EffectIt->second)
			{
				UE_LOG(LogCrowdControl, Warning, TEXT("%s failed. Effect ID '%s' was not found."), ActionName, *EffectID);
				continue;
			}

			Toggle(EffectIt->second.get(), bValue);
		}

		UE_LOG(LogCrowdControl, Log, TEXT("%s applied to %d requested effects."), ActionName, EffectIDs.Num());
	}

	FString GetMenuErrorKey(const FString& Type, const FString& ID, const FString& DisplayName)
	{
		return Type + TEXT(":") + (ID.IsEmpty() ? FString(TEXT("missing-id:")) + DisplayName : ID);
	}

	FString GetCloneErrorKey(const FString& SourceEffectID, const TArray<FString>& DestinationEffectIDs)
	{
		FString Key = GetMenuErrorKey(TEXT("clone"), SourceEffectID, FString());
		for (const FString& DestinationEffectID : DestinationEffectIDs)
		{
			Key += FString::Printf(TEXT(":%d:%s"), DestinationEffectID.Len(), *DestinationEffectID);
		}
		return Key;
	}
}


UCrowdControlSubsystem::CrowdControlConnectFunctionType UCrowdControlSubsystem::CC_ConnectFunction;
UCrowdControlSubsystem::CrowdControlDisconnectFunctionType UCrowdControlSubsystem::CC_DisconnectFunction;
UCrowdControlSubsystem::CrowdControlFunctionType UCrowdControlSubsystem::CC_CrowdControlFunction;
UCrowdControlSubsystem::FP_Command UCrowdControlSubsystem::CC_CommandFunction;
UCrowdControlSubsystem::ResetCommandType UCrowdControlSubsystem::CC_ResetCommand;

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

typedef void(*SetGameNameAndPackIDType)(char* name, char* packId);
SetGameNameAndPackIDType CC_SetGameNameAndPackID;

FString UCrowdControlSubsystem::BuildEffectContext(const FString& EffectID) const
{
	const TWeakObjectPtr<UCrowdControlEffectComponent>* ComponentPtr = EffectComponents.Find(EffectID);
	if (ComponentPtr != nullptr && ComponentPtr->IsValid())
	{
		const UCrowdControlEffectComponent* Component = ComponentPtr->Get();
		const AActor* Owner = Component->GetOwner();
		return FString::Printf(
			TEXT("effectID='%s', component='%s', owner='%s'"),
			*EffectID,
			*GetNameSafe(Component),
			*GetNameSafe(Owner));
	}

	return FString::Printf(TEXT("effectID='%s'"), *EffectID);
}

void UCrowdControlSubsystem::SetMenuGenerationError(const FString& Stage, const FString& ErrorKey, const FString& ErrorMessage)
{
	MenuGenerationErrors.Add(ErrorKey, TPair<FString, FString>(Stage, ErrorMessage));
	UE_LOG(LogCrowdControl, Error, TEXT("CCMenu generation failed during %s: %s"), *Stage, *ErrorMessage);
}

void UCrowdControlSubsystem::ReportMenuExportFailure(const FString& Stage, const FString& Detail)
{
	const FString Message = FString::Printf(TEXT("CCMenu export failed during %s: %s"), *Stage, *Detail);
	UE_LOG(LogCrowdControl, Error, TEXT("%s"), *Message);
#if WITH_EDITOR
	FNotificationInfo Notification(FText::FromString(Message));
	Notification.ExpireDuration = 6.0f;
	Notification.bUseSuccessFailIcons = true;
	Notification.bFireAndForget = true;
	if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Notification))
	{
		Item->SetCompletionState(SNotificationItem::CS_Fail);
	}
#endif
}

void UCrowdControlSubsystem::ReportMenuExportSuccess(const FString& FilePath, const int32 EffectCount) const
{
	UE_LOG(LogCrowdControl, Log, TEXT("Successfully wrote CCMenu export to '%s' (%d entries)."), *FilePath, EffectCount);
#if WITH_EDITOR
	const FString Message = FString::Printf(TEXT("CCMenu exported to %s (%d entries)."), *FilePath, EffectCount);
	FNotificationInfo Notification(FText::FromString(Message));
	Notification.ExpireDuration = 3.0f;
	Notification.bUseSuccessFailIcons = true;
	Notification.bFireAndForget = true;
	if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Notification))
	{
		Item->SetCompletionState(SNotificationItem::CS_Success);
	}
#endif
}

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
	MenuGenerationErrors.Reset();
	GameJsonObject = MakeShareable(new FJsonObject);
	GameEventsJsonObject = MakeShareable(new FJsonObject);
	
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
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	MenuJson = "";
	GameJsonObject.Reset();
	GameEventsJsonObject.Reset();
	if (Runnable) {
		Runnable->EnsureCompletion();
		Runnable.Reset();
	}

	
	Super::Deinitialize();
}


void UCrowdControlSubsystem::PrintEffectsToJsonFile()
{
	if (!GameJsonObject.IsValid() || !GameEventsJsonObject.IsValid())
	{
		ReportMenuExportFailure(TEXT("discovery/loading"), TEXT("Manifest containers are not initialized. Reconnect Crowd Control and register effects again."));
		return;
	}

	if (!MenuGenerationErrors.IsEmpty())
	{
		for (const TPair<FString, TPair<FString, FString>>& Error : MenuGenerationErrors)
		{
			UE_LOG(LogCrowdControl, Error, TEXT("CCMenu generation failure (%s): %s"), *Error.Value.Key, *Error.Value.Value);
		}
		FString Stage;
		FString Detail;
		if (CrowdControlManifest::GetGenerationFailure(MenuGenerationErrors, Stage, Detail))
		{
			ReportMenuExportFailure(Stage, Detail);
		}
		return;
	}

	if (GameJsonObject->Values.IsEmpty() && GameEventsJsonObject->Values.IsEmpty())
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("No effects or game events are registered. Nothing to export."));
		return;
	}

	const TSharedPtr<FJsonObject> RootJsonObject = CrowdControlManifest::CreateRoot(GameJsonObject, GameEventsJsonObject);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(RootJsonObject.ToSharedRef(), Writer))
	{
		ReportMenuExportFailure(TEXT("JSON formatting"), TEXT("Failed to serialize the final manifest JSON."));
		return;
	}

	MenuJson = JsonString;
	const FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CCMenu.json"));
	if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		ReportMenuExportFailure(TEXT("file writing"), FString::Printf(TEXT("Unable to write '%s'."), *FilePath));
		return;
	}

	ReportMenuExportSuccess(FilePath, GameJsonObject->Values.Num() + GameEventsJsonObject->Values.Num());
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

void UCrowdControlSubsystem::ShowEffectsByIDs(const TArray<FString>& EffectIDs)
{
	ToggleEffectsByIDs(EffectIDs, static_cast<FCCEffectMap*>(CC_EffectsMap), CC_ToggleVisible, true, TEXT("ShowEffectsByIDs"));
}

void UCrowdControlSubsystem::HideEffectsByIDs(const TArray<FString>& EffectIDs)
{
	ToggleEffectsByIDs(EffectIDs, static_cast<FCCEffectMap*>(CC_EffectsMap), CC_ToggleVisible, false, TEXT("HideEffectsByIDs"));
}

void UCrowdControlSubsystem::EnableEffectsByIDs(const TArray<FString>& EffectIDs)
{
	ToggleEffectsByIDs(EffectIDs, static_cast<FCCEffectMap*>(CC_EffectsMap), CC_ToggleSellable, true, TEXT("EnableEffectsByIDs"));
}

void UCrowdControlSubsystem::DisableEffectsByIDs(const TArray<FString>& EffectIDs)
{
	ToggleEffectsByIDs(EffectIDs, static_cast<FCCEffectMap*>(CC_EffectsMap), CC_ToggleSellable, false, TEXT("DisableEffectsByIDs"));
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

		if (parameter.type == ECrowdControlParamType::MinMax)
		{
			TSharedPtr<FJsonObject> MinMaxObject = MakeShareable(new FJsonObject);
			MinMaxObject->SetNumberField("min", parameter.min);
			MinMaxObject->SetNumberField("max", parameter.max);
			ParameterObject->SetObjectField("quantity", MinMaxObject);
			ParametersObject->SetObjectField(parameter._id, ParameterObject);
			continue;
		}

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

void UCrowdControlSubsystem::DeleteCustomEffects()
{
	DeleteCustomEffectsByJson(FString());
}

void UCrowdControlSubsystem::DeleteCustomEffectsByJson(const FString& EffectIDsJson)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl DeleteCustomEffects call failed! Currently not initialized!"));
		return;
	}

	if (CC_DeleteCustomEffects != nullptr)
	{
		// If EffectIDsJson is empty, pass empty string (which will be treated as "delete all")
		FString JsonToSend = EffectIDsJson.IsEmpty() ? FString() : EffectIDsJson;
		CC_DeleteCustomEffects(TCHAR_TO_UTF8(*JsonToSend));
		
		if (EffectIDsJson.IsEmpty())
		{
			UE_LOG(LogCrowdControl, Log, TEXT("DeleteCustomEffects called with empty JSON - will delete all custom effects"));
		}
		else
		{
			UE_LOG(LogCrowdControl, Log, TEXT("DeleteCustomEffects called with JSON: %s"), *EffectIDsJson);
		}
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_DeleteCustomEffects function pointer is null!"));
	}
}

void UCrowdControlSubsystem::DeleteCustomEffectsByIDs(const TArray<FString>& EffectIDs)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl DeleteCustomEffectsByIDs call failed! Currently not initialized!"));
		return;
	}

	if (EffectIDs.Num() == 0)
	{
		// Empty array means delete all
		DeleteCustomEffects();
		return;
	}

	// Convert array of strings to JSON array string
	FString JsonString = TEXT("[");
	for (int32 i = 0; i < EffectIDs.Num(); i++)
	{
		JsonString += TEXT("\"") + EffectIDs[i].Replace(TEXT("\""), TEXT("\\\"")) + TEXT("\"");
		if (i < EffectIDs.Num() - 1)
		{
			JsonString += TEXT(",");
		}
	}
	JsonString += TEXT("]");

	DeleteCustomEffectsByJson(JsonString);
	UE_LOG(LogCrowdControl, Log, TEXT("DeleteCustomEffectsByIDs called with %d effect IDs"), EffectIDs.Num());
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

void UCrowdControlSubsystem::RegisterEffectComponent(UCrowdControlEffectComponent* Component)
{
	if (Component == nullptr)
	{
		return;
	}

	const FString ComponentKey = GetMenuErrorKey(TEXT("component"), Component->GetPathName(), FString());
	if (Component->EffectID.IsEmpty())
	{
		const AActor* Owner = Component->GetOwner();
		SetMenuGenerationError(TEXT("metadata construction"), ComponentKey,
			FString::Printf(TEXT("Effect component '%s' on owner '%s' has no EffectID; set a unique ID before exporting the menu."),
				*GetNameSafe(Component),
				*GetNameSafe(Owner)));
		return;
	}

	EffectComponents.Add(Component->EffectID, Component);
	MenuGenerationErrors.Remove(ComponentKey);
}

void UCrowdControlSubsystem::UnregisterEffectComponent(UCrowdControlEffectComponent* Component)
{
	if (Component == nullptr)
	{
		return;
	}

	for (auto It = EffectComponents.CreateIterator(); It; ++It)
	{
		if (It.Value().Get() == Component)
		{
			It.RemoveCurrent();
		}
	}
}

void UCrowdControlSubsystem::StartThread() {
	if (CC_CrowdControlFunction != nullptr) {
        UE_LOG(LogTemp, Warning, TEXT("Run function loaded successfully"));
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
    	
		CC_CommandFunction = (FP_Command)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetCommandID"));
		
		CC_CrowdControlFunction = (CrowdControlFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("RunCrowdControl"));
		CC_ConnectFunction = (CrowdControlConnectFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("ConnectCrowdControl"));
		CC_DisconnectFunction = (CrowdControlDisconnectFunctionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("DisconnectCrowdControl"));
		
		CC_ResetCommand = (ResetCommandType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("ResetCommand"));
		CC_StringTest = (StringTestType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetQueuedMessage"));

    	CC_EffectSuccess = (EffectSuccessFailureType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectSuccess"));
		CC_EffectFailure = (EffectSuccessFailureType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectFailure"));
    	
    	CC_StopEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("StopEffectById"));
    	CC_ResetEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("ResetEffectById"));
    	CC_PauseEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("PauseEffectById"));
    	CC_ResumeEffect = (EffectStatusChangeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("ResumeEffectById"));
    	CC_IsRunning = (EffectIsRunningType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("IsEffectRunning"));
		ensure(CC_StopEffect && CC_ResetEffect && CC_PauseEffect && CC_ResumeEffect && CC_IsRunning);
    	
    	CC_SetGameNameAndPackID = (SetGameNameAndPackIDType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("SetGameNameAndPackId"));
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

		// Show/Hide/Enable/Disable effect toggles. These may be null when running against an older
		// CrowdControl.dll, or one built with a different toolchain, in which case the toggles no-op.
		CC_EffectsMap = FPlatformProcess::GetDllExport(DLLHandle, CCEffectsMapExport);
		CC_ToggleVisible = (ToggleEffectFlagType)FPlatformProcess::GetDllExport(DLLHandle, CCToggleVisibleExport);
		CC_ToggleSellable = (ToggleEffectFlagType)FPlatformProcess::GetDllExport(DLLHandle, CCToggleSellableExport);
		if (CC_EffectsMap == nullptr || CC_ToggleVisible == nullptr || CC_ToggleSellable == nullptr)
		{
			UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl.dll does not export the effect toggle symbols. Show/Hide/Enable/DisableEffectsByIDs will do nothing."));
		}
    	
		CC_SetEngine = (SetEngineType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("SetEngine"));
		CC_EngineEffect = (EngineEffectType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetEngineEffect"));
		CC_SetEngine();

		// Application auth-code flow, session control, effect reports, and metadata.
		// These may be null when running against an older CrowdControl.dll.
		CC_SetAppID = (SetAppIDType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("SetAppID"));
		CC_SetGameVersion = (SetGameVersionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("SetGameVersion"));
		CC_RequestAuthCode = (RequestAuthCodeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("RequestAuthCode"));
		CC_GetAuthCode = (GetAuthCodeType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetAuthCode"));
		CC_SetAutoStartSession = (SetAutoStartSessionType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("SetAutoStartSession"));
		CC_StartSession = (SessionControlType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("StartSession"));
		CC_StopSession = (SessionControlType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("StopSession"));
		CC_GetSessionStartError = (GetSessionStartErrorType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("GetSessionStartError"));
		CC_EffectSuccessWithMessage = (EffectResponseMessageType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectSuccessWithMessage"));
		CC_EffectFailureWithMessage = (EffectResponseMessageType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectFailureWithMessage"));
		CC_EffectFailTemporary = (EffectResponseMessageType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectFailTemporary"));
		CC_EffectFailPermanent = (EffectResponseMessageType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectFailPermanent"));
		CC_ReportEffectStatus = (ReportEffectStatusType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("ReportEffectStatus"));
		CC_EffectMetadata = (EffectMetadataType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("EffectMetadata"));
		CC_SendPackMetadata = (SendPackMetadataType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("SendPackMetadata"));
		CC_TriggerEvent = (TriggerEventType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("TriggerEvent"));
		CC_CloneEffect = (CloneEffectType)FPlatformProcess::GetDllExport(DLLHandle, TEXT("CloneEffect"));
		if (CC_GetSessionStartError == nullptr)
		{
			UE_LOG(LogCrowdControl, Warning, TEXT("The loaded CrowdControl.dll does not expose session-start failure details. Update the plugin DLL to receive OnSessionStartFailed events."));
		}

		// The legacy setter remains the compatibility entry point for GamePackID.
    	const UCrowdControlDeveloperSettings* Settings = GetDefault<UCrowdControlDeveloperSettings>();
    	if(Settings)
    	{
			CC_SetGameNameAndPackID(TCHAR_TO_UTF8(*Settings->GamePackID), TCHAR_TO_UTF8(*Settings->GamePackID));

			if (CC_SetGameVersion != nullptr)
			{
				CC_SetGameVersion(TCHAR_TO_UTF8(*Settings->GameVersion));
			}
			else if (!Settings->GameVersion.IsEmpty())
			{
				UE_LOG(LogCrowdControl, Warning, TEXT("GameVersion is configured, but the loaded CrowdControl.dll does not support the SetGameVersion export. Rebuild CrowdControl.dll to send X-CC-Version."));
			}

    		if (CC_SetAppID != nullptr && !Settings->ApplicationID.IsEmpty())
    		{
    			CC_SetAppID(TCHAR_TO_UTF8(*Settings->ApplicationID));
    		}

    		if (CC_SetAutoStartSession != nullptr)
    		{
    			CC_SetAutoStartSession(Settings->bStartSessionAutomatically);
    		}
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
	RequestAuthCode();
}

void UCrowdControlSubsystem::LoginYoutube()
{
	RequestAuthCode();
}

void UCrowdControlSubsystem::LoginDiscord()
{
	RequestAuthCode();
}

void UCrowdControlSubsystem::RequestAuthCode()
{
	if (CC_RequestAuthCode != nullptr)
	{
		CC_RequestAuthCode();
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("RequestAuthCode is not supported by the loaded CrowdControl.dll. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
	}
}

void UCrowdControlSubsystem::StartGameSession()
{
	if (CC_StartSession != nullptr)
	{
		CC_StartSession();
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("StartGameSession is not supported by the loaded CrowdControl.dll. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
	}
}

void UCrowdControlSubsystem::StopGameSession()
{
	if (CC_StopSession != nullptr)
	{
		CC_StopSession();
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("StopGameSession is not supported by the loaded CrowdControl.dll. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
	}
}

void UCrowdControlSubsystem::EffectFailureTemporary(FString id, FString Message)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl EffectFailureTemporary call failed! Currently not initialized!"))
		return;
	}

	if (CC_EffectFailTemporary != nullptr)
	{
		CC_EffectFailTemporary(TCHAR_TO_UTF8(*id), TCHAR_TO_UTF8(*Message));
	}
	else
	{
		EffectFailure(id);
	}
}

void UCrowdControlSubsystem::EffectFailurePermanent(FString id, FString Message)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl EffectFailurePermanent call failed! Currently not initialized!"))
		return;
	}

	if (CC_EffectFailPermanent != nullptr)
	{
		CC_EffectFailPermanent(TCHAR_TO_UTF8(*id), TCHAR_TO_UTF8(*Message));
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("EffectFailurePermanent is not supported by the loaded CrowdControl.dll - sending temporary failure instead. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
		EffectFailure(id);
	}
}

bool UCrowdControlSubsystem::ReportEffectStatus(FString EffectID, ECrowdControlEffectReport Status)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl ReportEffectStatus call failed! Currently not initialized!"))
		return false;
	}

	if (CC_ReportEffectStatus == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("ReportEffectStatus is not supported by the loaded CrowdControl.dll. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
		return false;
	}

	return CC_ReportEffectStatus(TCHAR_TO_UTF8(*EffectID), static_cast<int>(Status));
}

bool UCrowdControlSubsystem::SetEffectVisibility(FString EffectID, bool bVisible)
{
	return ReportEffectStatus(EffectID, bVisible ? ECrowdControlEffectReport::MenuVisible : ECrowdControlEffectReport::MenuHidden);
}

bool UCrowdControlSubsystem::SetEffectAvailability(FString EffectID, bool bAvailable)
{
	return ReportEffectStatus(EffectID, bAvailable ? ECrowdControlEffectReport::MenuAvailable : ECrowdControlEffectReport::MenuUnavailable);
}

void UCrowdControlSubsystem::EffectMetadata(const FString& Key, const FString& Value)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl EffectMetadata call failed! Currently not initialized!"))
		return;
	}

	if (CC_EffectMetadata != nullptr)
	{
		CC_EffectMetadata(TCHAR_TO_UTF8(*Key), TCHAR_TO_UTF8(*Value));
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("EffectMetadata is not supported by the loaded CrowdControl.dll. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
	}
}

void UCrowdControlSubsystem::SendPackMetadataJson(const FString& MetadataJson)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl SendPackMetadataJson call failed! Currently not initialized!"))
		return;
	}

	if (CC_SendPackMetadata != nullptr)
	{
		CC_SendPackMetadata(TCHAR_TO_UTF8(*MetadataJson));
	}
	else
	{
		UE_LOG(LogCrowdControl, Error, TEXT("SendPackMetadata is not supported by the loaded CrowdControl.dll. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
	}
}

void UCrowdControlSubsystem::SendPackMetadata(const FString& Key, const FString& Value)
{
	TSharedPtr<FJsonObject> MetadataObject = MakeShareable(new FJsonObject);
	MetadataObject->SetStringField(Key, Value);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(MetadataObject.ToSharedRef(), Writer))
	{
		SendPackMetadataJson(JsonString);
	}
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

void UCrowdControlSubsystem::FreeCategories(char** categoriesArray)
{
	if (categoriesArray == nullptr)
	{
		return;
	}

	for (int32 i = 0; categoriesArray[i] != nullptr; ++i)
	{
		delete[] categoriesArray[i];
	}

	delete[] categoriesArray;
}

void UCrowdControlSubsystem::SetupWorldTimer(UWorld* World, const FWorldInitializationValues IVS)
{
	// Use the core ticker rather than a world timer: it runs on the game thread but
	// keeps firing while the world is paused, so the DLL's message queue, auth-code
	// polling and effect dispatch never stall in pause menus.
	if (TickerHandle.IsValid())
		return;

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::OnCoreTick), 0.1f);
}

bool UCrowdControlSubsystem::OnCoreTick(float DeltaTime)
{
	Tick(DeltaTime);
	return true;
}

void UCrowdControlSubsystem::SetupEffect(const FCrowdControlEffectInfo& Info)
{
	if(!bIsInitialized)
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName),
			FString::Printf(TEXT("Effect registration failed (%s): Crowd Control is not initialized. Register effects after OnSessionReady."),
				*BuildEffectContext(Info.id)));
		return;
	}
	if (Info.id.IsEmpty() || Info.displayName.IsEmpty())
	{
		SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName),
			FString::Printf(TEXT("Effect registration failed (%s): effect ID and display name are required."),
				*BuildEffectContext(Info.id)));
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
	// Convert JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	}
	else
	{
	SetMenuGenerationError(TEXT("effect serialization"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
			TEXT("Effect registration failed (%s): effect serialization failed."),
			*BuildEffectContext(Info.id)));
		return;
	}

	if (CC_AddBasicEffect == nullptr)
	{
	SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
		TEXT("Effect registration failed (%s): the AddNewBasicEffect DLL export is unavailable. Reload the current CrowdControl DLL."),
		*BuildEffectContext(Info.id)));
	return;
	}

	char** Categories = SplitCategories(Info.category);
	CC_AddBasicEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.displayName), TCHAR_TO_UTF8(*Info.description), Info.price, 0, 0, 0, 0, 1, 0, 0, 0, Categories);
	FreeCategories(Categories);
	GameJsonObject->SetObjectField(Info.id, JsonObject);
	MenuJson += JsonString;
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName));
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("effect"), FString(), Info.displayName));
}

bool UCrowdControlSubsystem::CloneEffect(const FString& SourceEffectID, const FString& DestinationEffectID)
{
	return CloneEffectToIDs(SourceEffectID, { DestinationEffectID });
}

bool UCrowdControlSubsystem::SetupGameEvent(const FCrowdControlGameEventInfo& Info)
{
	if (!bIsInitialized)
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName),
			FString::Printf(TEXT("Game event registration failed (eventID='%s'): Crowd Control is not initialized. Register events after OnSessionReady."),
				*Info.id));
		return false;
	}

	if (Info.id.IsEmpty() || Info.displayName.IsEmpty())
	{
		SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName), TEXT("Game event registration failed: event ID and display name are required."));
		return false;
	}

	if (!GameEventsJsonObject.IsValid())
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName),
			FString::Printf(TEXT("Game event registration failed (eventID='%s'): event manifest storage is unavailable."),
				*Info.id));
		return false;
	}

	TSharedPtr<FJsonObject> EventJsonObject = MakeShared<FJsonObject>();
	EventJsonObject->SetStringField("name", Info.displayName);

	if (!Info.arguments.IsEmpty())
	{
		TSharedPtr<FJsonObject> ArgumentsJsonObject = MakeShared<FJsonObject>();
		TSet<FString> SeenArgumentIDs;
		for (const FCrowdControlGameEventArgument& Argument : Info.arguments)
		{
			if (Argument.id.IsEmpty() || Argument.displayName.IsEmpty())
			{
				SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName), FString::Printf(
					TEXT("Game event registration failed (eventID='%s'): an argument has an empty ID or display name."),
					*Info.id));
				return false;
			}
			if (SeenArgumentIDs.Contains(Argument.id))
			{
				SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName), FString::Printf(
					TEXT("Game event registration failed (eventID='%s'): duplicate argument ID '%s'."),
					*Info.id,
					*Argument.id));
				return false;
			}
			SeenArgumentIDs.Add(Argument.id);

			TSharedPtr<FJsonObject> ArgumentJsonObject = MakeShared<FJsonObject>();
			ArgumentJsonObject->SetStringField("name", Argument.displayName);
			if (Argument.type == ECrowdControlGameEventArgumentType::Number)
			{
				if ((Argument.bHasMinimum && Argument.bHasMaximum && Argument.minimum > Argument.maximum)
					|| (Argument.bHasMinimum && !FMath::IsFinite(Argument.minimum))
					|| (Argument.bHasMaximum && !FMath::IsFinite(Argument.maximum)))
				{
					SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName), FString::Printf(
						TEXT("Game event registration failed (eventID='%s', argID='%s'): numeric bounds are invalid."),
						*Info.id,
						*Argument.id));
					return false;
				}

				ArgumentJsonObject->SetStringField("type", "number");
				if (Argument.bHasMinimum)
				{
					ArgumentJsonObject->SetNumberField("min", Argument.minimum);
				}
				if (Argument.bHasMaximum)
				{
					ArgumentJsonObject->SetNumberField("max", Argument.maximum);
				}
				if (Argument.bInteger)
				{
					ArgumentJsonObject->SetBoolField("int", true);
				}
			}
			else
			{
				TSharedPtr<FJsonObject> LiteralsJsonObject = MakeShared<FJsonObject>();
				TSet<FString> SeenLiteralIDs;
				for (const FCrowdControlGameEventLiteral& Literal : Argument.literals)
				{
					if (Literal.id.IsEmpty() || Literal.displayName.IsEmpty())
					{
						SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName), FString::Printf(
							TEXT("Game event registration failed (eventID='%s', argID='%s'): a literal has an empty ID or display name."),
							*Info.id,
							*Argument.id));
						return false;
					}
					if (SeenLiteralIDs.Contains(Literal.id))
					{
						SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName), FString::Printf(
							TEXT("Game event registration failed (eventID='%s', argID='%s'): duplicate literal ID '%s'."),
							*Info.id,
							*Argument.id,
							*Literal.id));
						return false;
					}
					SeenLiteralIDs.Add(Literal.id);

					TSharedPtr<FJsonObject> LiteralJsonObject = MakeShared<FJsonObject>();
					LiteralJsonObject->SetStringField("name", Literal.displayName);
					LiteralsJsonObject->SetObjectField(Literal.id, LiteralJsonObject);
				}
				ArgumentJsonObject->SetStringField("type", "literal");
				ArgumentJsonObject->SetObjectField("literals", LiteralsJsonObject);
			}

			ArgumentsJsonObject->SetObjectField(Argument.id, ArgumentJsonObject);
		}
		EventJsonObject->SetObjectField("args", ArgumentsJsonObject);
	}

	GameEventsJsonObject->SetObjectField(Info.id, EventJsonObject);
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("event"), Info.id, Info.displayName));
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("event"), FString(), Info.displayName));
	return true;
}

bool UCrowdControlSubsystem::TriggerGameEvent(const FString& EventID)
{
	return TriggerGameEventJson(EventID, TEXT(""));
}

bool UCrowdControlSubsystem::TriggerGameEventJson(const FString& EventID, const FString& ArgsJson)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("TriggerGameEvent failed because Crowd Control is not initialized."));
		return false;
	}

	if (EventID.IsEmpty())
	{
		UE_LOG(LogCrowdControl, Error, TEXT("TriggerGameEvent requires a non-empty event ID."));
		return false;
	}

	if (CC_TriggerEvent == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("TriggerEvent is not supported by the loaded CrowdControl.dll. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
		return false;
	}

	if (!ArgsJson.IsEmpty())
	{
		TSharedPtr<FJsonObject> ArgsJsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ArgsJson);
		if (!FJsonSerializer::Deserialize(Reader, ArgsJsonObject) || !ArgsJsonObject.IsValid())
		{
			UE_LOG(LogCrowdControl, Error, TEXT("TriggerGameEventJson requires ArgsJson to be a valid JSON object."));
			return false;
		}
	}

	CC_TriggerEvent(TCHAR_TO_UTF8(*EventID), ArgsJson.IsEmpty() ? nullptr : TCHAR_TO_UTF8(*ArgsJson));
	return true;
}

bool UCrowdControlSubsystem::CloneEffectToIDs(const FString& SourceEffectID, const TArray<FString>& DestinationEffectIDs)
{
	const FString SourceContext = BuildEffectContext(SourceEffectID);
	if (!bIsInitialized || CC_CloneEffect == nullptr)
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetCloneErrorKey(SourceEffectID, DestinationEffectIDs),
			FString::Printf(TEXT("Cloning %s failed because Crowd Control is not initialized or the loaded DLL does not support cloning."),
				*SourceContext));
		return false;
	}

	if (SourceEffectID.IsEmpty() || DestinationEffectIDs.IsEmpty())
	{
		SetMenuGenerationError(TEXT("metadata construction"), GetCloneErrorKey(SourceEffectID, DestinationEffectIDs),
			FString::Printf(TEXT("Cloning %s requires a non-empty source ID and at least one destination ID."), *SourceContext));
		return false;
	}

	TSet<FString> SeenDestinationIDs;
	for (const FString& DestinationEffectID : DestinationEffectIDs)
	{
		if (DestinationEffectID.IsEmpty() || DestinationEffectID == SourceEffectID || SeenDestinationIDs.Contains(DestinationEffectID))
		{
			SetMenuGenerationError(TEXT("metadata construction"), GetCloneErrorKey(SourceEffectID, DestinationEffectIDs),
				FString::Printf(TEXT("Cloning %s requires non-empty, unique destination IDs different from the source ID."),
					*SourceContext));
			return false;
		}
		SeenDestinationIDs.Add(DestinationEffectID);
	}

	std::vector<std::string> DestinationStorage;
	DestinationStorage.reserve(DestinationEffectIDs.Num());
	for (const FString& DestinationEffectID : DestinationEffectIDs)
	{
		DestinationStorage.emplace_back(TCHAR_TO_UTF8(*DestinationEffectID));
	}

	std::vector<const char*> DestinationPointers;
	DestinationPointers.reserve(DestinationStorage.size() + 1);
	for (const std::string& Destination : DestinationStorage)
	{
		DestinationPointers.push_back(Destination.c_str());
	}
	DestinationPointers.push_back(nullptr);

	if (!CC_CloneEffect(TCHAR_TO_UTF8(*SourceEffectID), DestinationPointers.data()))
	{
		SetMenuGenerationError(TEXT("metadata construction"), GetCloneErrorKey(SourceEffectID, DestinationEffectIDs),
			FString::Printf(TEXT("Failed to clone %s to the requested destination IDs. Verify the source is registered and destinations are unique and unused."),
				*SourceContext));
		return false;
	}

	if (const TWeakObjectPtr<UCrowdControlEffectComponent>* SourceComponent = EffectComponents.Find(SourceEffectID))
	{
		for (const FString& DestinationEffectID : DestinationEffectIDs)
		{
			EffectComponents.Add(DestinationEffectID, *SourceComponent);
		}
	}

	const TSharedPtr<FJsonObject>* SourceMenuEntry = nullptr;
	if (GameJsonObject.IsValid() && GameJsonObject->TryGetObjectField(SourceEffectID, SourceMenuEntry))
	{
		for (const FString& DestinationEffectID : DestinationEffectIDs)
		{
			GameJsonObject->SetObjectField(DestinationEffectID, *SourceMenuEntry);
		}
	}
	MenuGenerationErrors.Remove(GetCloneErrorKey(SourceEffectID, DestinationEffectIDs));

	return true;
}

void UCrowdControlSubsystem::SetupTimedEffect(const FCrowdControlTimedEffectInfo& Info)
{
	if(!bIsInitialized)
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName),
			FString::Printf(TEXT("Timed effect registration failed (%s): Crowd Control is not initialized. Register effects after OnSessionReady."),
				*BuildEffectContext(Info.id)));
		return;
	}
	if (Info.id.IsEmpty() || Info.displayName.IsEmpty())
	{
		SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName),
			FString::Printf(TEXT("Timed effect registration failed (%s): effect ID and display name are required."),
				*BuildEffectContext(Info.id)));
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
	
	// Convert JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	}
	else
	{
		SetMenuGenerationError(TEXT("effect serialization"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
			TEXT("Timed effect registration failed (%s): effect serialization failed."),
			*BuildEffectContext(Info.id)));
		return;
	}

	if (CC_AddTimedEffect == nullptr)
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
			TEXT("Timed effect registration failed (%s): the AddNewTimedEffect DLL export is unavailable. Reload the current CrowdControl DLL."),
			*BuildEffectContext(Info.id)));
		return;
	}

	char** Categories = SplitCategories(Info.category);
	CC_AddTimedEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.displayName), TCHAR_TO_UTF8(*Info.description), Info.price, 0, 0, 0, 0, 1, 0, 0, 0, Categories, Info.duration);
	FreeCategories(Categories);
	GameJsonObject->SetObjectField(Info.id, JsonObject);
	MenuJson += JsonString;
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName));
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("effect"), FString(), Info.displayName));
}

void UCrowdControlSubsystem::SetupParameterEffect(const FCrowdControlParameterEffectInfo& Info)
{
	if(!bIsInitialized)
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName),
			FString::Printf(TEXT("Parameter effect registration failed (%s): Crowd Control is not initialized. Register effects after OnSessionReady."),
				*BuildEffectContext(Info.id)));
		return;
	}

	if (Info.id.IsEmpty() || Info.displayName.IsEmpty())
	{
		SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName),
			TEXT("Parameter effect registration failed: effect ID and display name are required."));
		return;
	}

	TSet<FString> SeenParameterIDs;
	for (const FCrowdControlParameter& Parameter : Info.parameters)
	{
		if (Parameter._id.IsEmpty())
		{
			SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
				TEXT("Parameter effect registration failed (%s): a parameter ID is empty."),
				*BuildEffectContext(Info.id)));
			return;
		}
		if (Parameter.name.IsEmpty())
		{
			SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
				TEXT("Parameter effect registration failed (%s): parameter '%s' has an empty display name."),
				*BuildEffectContext(Info.id),
				*Parameter._id));
			return;
		}

		if (SeenParameterIDs.Contains(Parameter._id))
		{
			SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
				TEXT("Parameter effect registration failed (%s): duplicate parameter ID '%s'."),
				*BuildEffectContext(Info.id),
				*Parameter._id));
			return;
		}
		SeenParameterIDs.Add(Parameter._id);

		if (Parameter.type == ECrowdControlParamType::MinMax && Parameter.min > Parameter.max)
		{
			SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
				TEXT("Parameter effect registration failed (%s): parameter '%s' has min (%d) greater than max (%d)."),
				*BuildEffectContext(Info.id),
				*Parameter._id,
				Parameter.min,
				Parameter.max));
			return;
		}

		if (Parameter.type == ECrowdControlParamType::OPTIONS)
		{
			TSet<FString> SeenOptionIDs;
			for (const FCrowdControlParamOption& Option : Parameter._options)
			{
				if (Option.id.IsEmpty())
				{
					SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
						TEXT("Parameter effect registration failed (%s): parameter '%s' contains an option with an empty ID."),
						*BuildEffectContext(Info.id),
						*Parameter._id));
					return;
				}
				if (Option.DisplayName.IsEmpty())
				{
					SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
						TEXT("Parameter effect registration failed (%s): option '%s' on parameter '%s' has an empty display name."),
						*BuildEffectContext(Info.id),
						*Option.id,
						*Parameter._id));
					return;
				}

				if (SeenOptionIDs.Contains(Option.id))
				{
					SetMenuGenerationError(TEXT("metadata construction"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
						TEXT("Parameter effect registration failed (%s): parameter '%s' has duplicate option ID '%s'."),
						*BuildEffectContext(Info.id),
						*Parameter._id,
						*Option.id));
					return;
				}
				SeenOptionIDs.Add(Option.id);
			}
		}
	}

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

	if (Info.RequiresQuantity)
	{
		TSharedPtr<FJsonObject> QuantityObject = MakeShareable(new FJsonObject);
		QuantityObject->SetNumberField("min", Info.quantity.GetLowerBoundValue());
		QuantityObject->SetNumberField("max", Info.quantity.GetUpperBoundValue());
		JsonObject->SetObjectField("quantity", QuantityObject);
	}

	TSharedPtr<FJsonObject> ParametersObject = MakeShareable(new FJsonObject);
	for (const FCrowdControlParameter& Parameter : Info.parameters)
	{
		TSharedPtr<FJsonObject> ParameterObject = MakeShareable(new FJsonObject);
		ParameterObject->SetStringField("name", Parameter.name);

		if (Parameter.type == ECrowdControlParamType::MinMax)
		{
			TSharedPtr<FJsonObject> MinMaxObject = MakeShareable(new FJsonObject);
			MinMaxObject->SetNumberField("min", Parameter.min);
			MinMaxObject->SetNumberField("max", Parameter.max);
			ParameterObject->SetObjectField("quantity", MinMaxObject);
			ParametersObject->SetObjectField(Parameter._id, ParameterObject);
			continue;
		}

		ParameterObject->SetStringField("type", Parameter.type == ECrowdControlParamType::OPTIONS ? "options" : "hex-color");
		if (Parameter.type == ECrowdControlParamType::OPTIONS)
		{
			TSharedPtr<FJsonObject> ParameterOptionsObject = MakeShareable(new FJsonObject);
			for (const FCrowdControlParamOption& Option : Parameter._options)
			{
				TSharedPtr<FJsonObject> ParameterOptionObject = MakeShareable(new FJsonObject);
				ParameterOptionObject->SetStringField("name", Option.DisplayName);
				ParameterOptionsObject->SetObjectField(Option.id, ParameterOptionObject);
			}
			ParameterObject->SetObjectField("options", ParameterOptionsObject);
		}
		ParametersObject->SetObjectField(Parameter._id, ParameterObject);
	}
	JsonObject->SetObjectField("parameters", ParametersObject);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		SetMenuGenerationError(TEXT("effect serialization"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
			TEXT("Parameter effect registration failed (%s): effect serialization failed."),
			*BuildEffectContext(Info.id)));
		return;
	}

	const bool bNeedsMinMaxExport = Info.RequiresQuantity || Info.parameters.ContainsByPredicate(
		[](const FCrowdControlParameter& Parameter) { return Parameter.type == ECrowdControlParamType::MinMax; });
	const bool bNeedsOptionExport = Info.parameters.ContainsByPredicate(
		[](const FCrowdControlParameter& Parameter) { return Parameter.type == ECrowdControlParamType::OPTIONS; });
	if (CC_AddParameterEffect == nullptr
		|| (bNeedsMinMaxExport && CC_AddParameterMinMax == nullptr)
		|| (bNeedsOptionExport && CC_AddParameterOption == nullptr))
	{
		SetMenuGenerationError(TEXT("discovery/loading"), GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName), FString::Printf(
			TEXT("Parameter effect registration failed (%s): one or more required parameter-effect DLL exports are unavailable. Reload the current CrowdControl DLL."),
			*BuildEffectContext(Info.id)));
		return;
	}

	char** Categories = SplitCategories(Info.category);
	CC_AddParameterEffect(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Info.displayName), TCHAR_TO_UTF8(*Info.description), Info.price, 0, 0, 0, 0, 1, 0, 0, 0, Categories);
	FreeCategories(Categories);

	if (Info.RequiresQuantity)
	{
		CC_AddParameterMinMax(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*FString("quantity")), Info.quantity.GetLowerBoundValue(), Info.quantity.GetUpperBoundValue());
	}

	for (const FCrowdControlParameter& Parameter : Info.parameters)
	{
		if (Parameter.type == ECrowdControlParamType::MinMax)
		{
			CC_AddParameterMinMax(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Parameter._id), Parameter.min, Parameter.max);
		}
		else if (Parameter.type == ECrowdControlParamType::OPTIONS)
		{
			TArray<FString> OptionIDs;
			for (const FCrowdControlParamOption& Option : Parameter._options)
			{
				OptionIDs.Add(Option.id);
			}
			char** OptionIDArray = SplitCategories(OptionIDs);
			CC_AddParameterOption(TCHAR_TO_UTF8(*Info.id), TCHAR_TO_UTF8(*Parameter._id), OptionIDArray);
			FreeCategories(OptionIDArray);
		}
	}

	UE_LOG(LogCrowdControl, Log, TEXT("SetupEffect Parameters: %s"), *JsonString);
	GameJsonObject->SetObjectField(Info.id, JsonObject);
	MenuJson += JsonString;
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("effect"), Info.id, Info.displayName));
	MenuGenerationErrors.Remove(GetMenuErrorKey(TEXT("effect"), FString(), Info.displayName));
}

void UCrowdControlSubsystem::EffectSuccess(FString id)
{
	if(!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect success call failed! Currently not initialized!"))
		return;
	}
	
	CC_EffectSuccess(TCHAR_TO_UTF8(*id));
}

void UCrowdControlSubsystem::EffectSuccessWithMessage(FString id, FString Message)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect success call failed! Currently not initialized!"))
		return;
	}

	if (CC_EffectSuccessWithMessage != nullptr)
	{
		CC_EffectSuccessWithMessage(TCHAR_TO_UTF8(*id), TCHAR_TO_UTF8(*Message));
	}
	else
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("EffectSuccessWithMessage is not supported by the loaded CrowdControl.dll - sending success without a message. Please update CrowdControl.dll in Plugins/UnrealCrowdControl/Binaries/Win64 to the latest version."));
		EffectSuccess(id);
	}
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

void UCrowdControlSubsystem::EffectFailureWithMessage(FString id, FString Message)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogCrowdControl, Warning, TEXT("CrowdControl Effect failure call failed! Currently not initialized!"))
		return;
	}

	if (CC_EffectFailureWithMessage != nullptr)
	{
		CC_EffectFailureWithMessage(TCHAR_TO_UTF8(*id), TCHAR_TO_UTF8(*Message));
	}
	else
	{
		EffectFailureTemporary(id, Message);
	}
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

	if (CC_GetSessionStartError != nullptr)
	{
		char* SessionStartErrorJson = CC_GetSessionStartError();
		if (SessionStartErrorJson != nullptr && SessionStartErrorJson[0] != '\0')
		{
			const FString ErrorJson = UTF8_TO_TCHAR(SessionStartErrorJson);
			CrowdControlSessionStartError::FDetails Details;
			if (!CrowdControlSessionStartError::Parse(ErrorJson, Details))
			{
				UE_LOG(LogCrowdControl, Error, TEXT("Failed to parse Crowd Control session-start failure details: %s"), *ErrorJson);
				Details.HttpStatusCode = -1;
				Details.TransportError = TEXT("The CrowdControl.dll returned malformed session-start failure details.");
			}

			const FString UserMessage = CrowdControlSessionStartError::BuildUserMessage(
				Details.GamePackID,
				Details.HttpStatusCode,
				Details.ResponseBody,
				Details.TransportError);
			const FString Diagnostics = FString::Printf(
				TEXT("gamePackID='%s', HTTP status=%d, transportError='%s', responseBody=%s"),
				*Details.GamePackID,
				Details.HttpStatusCode,
				*Details.TransportError,
				Details.ResponseBody.IsEmpty() ? TEXT("<empty>") : *Details.ResponseBody);
			UE_LOG(LogCrowdControl, Error, TEXT("Crowd Control session start failed: %s Diagnostics: %s"), *UserMessage, *Diagnostics);
			OnSessionStartFailed.Broadcast(UserMessage, Diagnostics, Details.HttpStatusCode);
		}
	}
	
	if (CC_CommandFunction == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_CommandFunction function pointer is null!"));
		return;
	}
	
	int32 CurrentResult = CC_CommandFunction();
	if(CurrentResult != CommandID)
	{
		const bool bWasInitialized = bIsInitialized;

		CommandID = CurrentResult;
		bIsConnected = CommandID >= 2;
		bIsInitialized = CommandID > 2;
		OnCommandIDChanged.Broadcast(CommandID);

		if (CommandID >= 0 && CommandID <= 3)
		{
			OnConnectionStateChanged.Broadcast(static_cast<ECrowdControlConnectionState>(CommandID));
		}

		if (bIsInitialized && !bWasInitialized)
		{
			OnSessionReady.Broadcast();
		}
	}

	// Check for a freshly generated application auth code (one-shot from the DLL)
	if (CC_GetAuthCode != nullptr)
	{
		char* authCodeStr = CC_GetAuthCode();
		if (authCodeStr != nullptr && authCodeStr[0] != '\0')
		{
			FString AuthCodeJson = FString(UTF8_TO_TCHAR(authCodeStr));
			TSharedPtr<FJsonObject> AuthCodeObject;
			TSharedRef<TJsonReader<>> AuthCodeReader = TJsonReaderFactory<>::Create(AuthCodeJson);

			FString Code, URL;
			if (FJsonSerializer::Deserialize(AuthCodeReader, AuthCodeObject) && AuthCodeObject.IsValid()
				&& AuthCodeObject->TryGetStringField(TEXT("code"), Code)
				&& AuthCodeObject->TryGetStringField(TEXT("url"), URL))
			{
				UE_LOG(LogCrowdControl, Log, TEXT("Auth code received: %s (%s)"), *Code, *URL);
				OnAuthCodeReceived.Broadcast(Code, URL);

				const UCrowdControlDeveloperSettings* Settings = GetDefault<UCrowdControlDeveloperSettings>();
				const bool bAutoOpenEnabled = Settings == nullptr || Settings->bOpenAuthURLWhenUnhandled;
				if (CrowdControlManifest::ShouldOpenAuthURL(OnAuthCodeReceived, bAutoOpenEnabled))
				{
					if (URL.IsEmpty())
					{
						UE_LOG(LogCrowdControl, Error, TEXT("Auth URL fallback was requested, but the auth response did not include a URL."));
					}
					else
					{
						FString LaunchError;
						FPlatformProcess::LaunchURL(*URL, nullptr, &LaunchError);
						if (!LaunchError.IsEmpty())
						{
							UE_LOG(LogCrowdControl, Error, TEXT("Failed to launch auth URL '%s': %s"), *URL, *LaunchError);
						}
						else
						{
							UE_LOG(LogCrowdControl, Log, TEXT("No auth-code listeners were bound; opened auth URL automatically."));
						}
					}
				}
			}
			else
			{
				UE_LOG(LogCrowdControl, Error, TEXT("Failed to parse auth code JSON: %s"), *AuthCodeJson);
			}
		}
	}

	// Check Queued events
	if (CC_EngineEffect == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_EngineEffect function pointer is null!"));
		return;
	}
	
	char * effectManifest = CC_EngineEffect();
	if (effectManifest != nullptr && effectManifest[0] != '\0')
	{
		UE_LOG(LogCrowdControl, Verbose, TEXT("%s"), *FString(effectManifest));
		FString JsonString = FString(effectManifest);

		// Parse JSON string
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()) 
		{
			// Extract "id" (request ID), "name", and "effectID" (the actual effect ID like "lowgravity_1234")
			FString Name;
			FString Id;
			FString EffectID;
			
			
			if (JsonObject->TryGetStringField(TEXT("name"), Name) && JsonObject->TryGetStringField(TEXT("id"), Id)) 
			{
				// Extract effectID if available (for backwards compatibility with older DLL versions)
				if (!JsonObject->TryGetStringField(TEXT("effectID"), EffectID))
				{
					EffectID = FString(); // Empty string if not available
				}
				
				UE_LOG(LogCrowdControl, Log, TEXT("Effect Name: %s   Request ID: %s   Effect ID: %s"), *Name, *Id, *EffectID);

				// Effects with a registered component are dispatched to it directly
				// (the component sends its own response); everything else goes to
				// the global delegates.
				UCrowdControlEffectComponent* TargetComponent = nullptr;
				if (!EffectID.IsEmpty())
				{
					if (const TWeakObjectPtr<UCrowdControlEffectComponent>* Found = EffectComponents.Find(EffectID))
					{
						TargetComponent = Found->Get();
					}
				}

				FString ViewerName;
				JsonObject->TryGetStringField(TEXT("viewer"), ViewerName);

				FString Duration, ParameterValue;
				const TSharedPtr<FJsonObject>* ParamsObject;
				int32 Quantity = 1;
				const bool bIsTimed = JsonObject->TryGetStringField(TEXT("duration"), Duration) && Duration.IsNumeric();
				const float DurationValue = bIsTimed ? FCString::Atof(*Duration) : 0.f;
				const bool bHasParams = JsonObject->TryGetObjectField(TEXT("params"), ParamsObject);
				const bool bHasQuantity = JsonObject->TryGetNumberField(TEXT("quantity"), Quantity);

				// Notification hook: always fires, regardless of how the effect is handled.
				OnEffectRequestReceived.Broadcast(Id, EffectID, Name, ViewerName, DurationValue, bHasQuantity ? Quantity : 1);

				if (bIsTimed)
				{
					if (TargetComponent != nullptr)
					{
						TargetComponent->HandleTrigger(Id, EffectID, DurationValue, 1, FJsonObjectWrapper(), ViewerName);
					}
					else
					{
						OnTimedEffectTrigger.Broadcast(Id, Name, DurationValue, EffectID, ViewerName);
					}
				}
				else if (bHasParams || bHasQuantity)
				{
					FJsonObjectWrapper ParamsWrapped;
					if (bHasParams)
					{
						ParamsWrapped.JsonObject = *ParamsObject;
					}

					if (TargetComponent != nullptr)
					{
						TargetComponent->HandleTrigger(Id, EffectID, 0.f, Quantity, ParamsWrapped, ViewerName);
					}
					else
					{
						OnParameterEffectTrigger.Broadcast(Id, Name, FString::FromInt(Quantity), ParamsWrapped, EffectID, ViewerName);
					}
				}
				else if (TargetComponent != nullptr)
				{
					TargetComponent->HandleTrigger(Id, EffectID, 0.f, 1, FJsonObjectWrapper(), ViewerName);
				}
				else
				{
					OnEffectTrigger.Broadcast(Id, Name, EffectID, ViewerName);
				}

				// Note: the game is responsible for responding to every request with
				// EffectSuccess / EffectFailureTemporary / EffectFailurePermanent — responses
				// may be sent asynchronously, so no automatic failure is issued here.
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

	if (CC_StringTest == nullptr)
	{
		UE_LOG(LogCrowdControl, Error, TEXT("CC_StringTest function pointer is null!"));
		return;
	}
	
	char * chrStr = CC_StringTest();
	
	// Check for null pointer
	if (chrStr == nullptr)
	{
		return;
	}
	
	// Check if string is empty
	if (chrStr[0] == '\0')
	{
		return;
	}
	
	int firstCharAsInt = static_cast<unsigned char>(chrStr[0]);
	const char* messageStr = chrStr + 1;
	
	if (messageStr != nullptr && messageStr[0] != '\0')
	{
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
}

void UCrowdControlSubsystem::Tick(float DeltaTime) {
    Update();
}

UCrowdControlSubsystem::~UCrowdControlSubsystem()
{
	MenuJson = "";
    Disconnect();
}
