#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "CrowdControlFunctionLibrary.h"
#include "CrowdControlRunable.h"
#include "Runtime/JsonUtilities/Public/JsonObjectWrapper.h"
#include "CrowdControlSubsystem.generated.h"

class UWorld;
class UCrowdControlEffectComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTriggerEffect, FString, ID, FString, DisplayName, FString, EffectID, FString, ViewerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnTriggerTimedEffect, FString, ID, FString, DisplayName, float, Duration, FString, EffectID, FString, ViewerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnTriggerParameterEffect, FString, ID, FString, DisplayName, FString, OptionalQuantity, FJsonObjectWrapper, Params, FString, EffectID, FString, ViewerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandIDChanged, int32, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthCodeReceived, FString, Code, FString, URL);

// Fired for every incoming effect request, regardless of how it is handled
// (component or global delegates). Intended for notifications/HUD.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnEffectRequestReceived, FString, RequestID, FString, EffectID, FString, DisplayName, FString, ViewerName, float, Duration, int32, Quantity);

UENUM(BlueprintType)
enum class ECrowdControlConnectionState : uint8
{
	Connecting = 0,
	Disconnected = 1,
	WaitingForLogin = 2,
	Connected = 3,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStateChanged, ECrowdControlConnectionState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionReady);

UENUM(BlueprintType)
enum class ECrowdControlEffectReport : uint8
{
	MenuVisible = 0,
	MenuHidden = 1,
	MenuAvailable = 2,
	MenuUnavailable = 3,
};


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
	
	// Requests a fresh application auth code (requires ApplicationID in settings).
	// The code/URL arrives via OnAuthCodeReceived.
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void RequestAuthCode();

	// Starts a game session explicitly (only needed when Start Session Automatically is off).
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void StartGameSession();

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void StopGameSession();

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SetupEffect(const FCrowdControlEffectInfo& Info);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SetupTimedEffect(const FCrowdControlTimedEffectInfo& Info);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SetupParameterEffect(const FCrowdControlParameterEffectInfo& Info);

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool CloneEffect(const FString& SourceEffectID, const FString& DestinationEffectID);

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool CloneEffectToIDs(const FString& SourceEffectID, const TArray<FString>& DestinationEffectIDs);
	
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

	// Typed connection state (same information as OnCommandIDChanged, without magic numbers).
	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	FOnConnectionStateChanged OnConnectionStateChanged;

	// Fired once each time the connection becomes fully ready for effects.
	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	FOnSessionReady OnSessionReady;

	// Fired for every incoming effect request, before it is routed to a component or
	// the trigger delegates. Bind notification/HUD widgets here ("Viewer sent Effect!").
	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	FOnEffectRequestReceived OnEffectRequestReceived;

	// Fired when an application auth code is generated. Show the code and/or open
	// the URL so the user can authorize the game.
	UPROPERTY(BlueprintAssignable, Category = "Crowd Control")
	FOnAuthCodeReceived OnAuthCodeReceived;

	// Send RPC "Success" to confirm this event worked
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectSuccess(FString id);

	// Send RPC "Success" with an optional message for the purchasing user
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectSuccessWithMessage(FString id, FString Message);

	// Send RPC "FailTemporarily" to signal this event failed
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectFailure(FString id);

	// Send RPC "FailTemporarily" with an optional message for the purchasing user
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectFailureWithMessage(FString id, FString Message);

	// Send RPC "failTemporary" with an explanation message (effect can be retried/refunded)
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectFailureTemporary(FString id, FString Message);

	// Send RPC "failPermanent" with an explanation message (effect will not be retried)
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void EffectFailurePermanent(FString id, FString Message);

	// Reports an effect's menu state to Crowd Control. Note: visibility and availability
	// are independent - set both when changing availability.
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool ReportEffectStatus(FString EffectID, ECrowdControlEffectReport Status);

	// Convenience wrappers over ReportEffectStatus
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool SetEffectVisibility(FString EffectID, bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	bool SetEffectAvailability(FString EffectID, bool bAvailable);

	// Sends a packMetadataChanged RPC with a JSON object of key/value game state
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SendPackMetadataJson(const FString& MetadataJson);

	// Convenience: send a single key/value pair of game state metadata
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	void SendPackMetadata(const FString& Key, const FString& Value);

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

	// Show effects by providing an array of effect ID strings
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Show effects by providing an array of effect IDs. Leave empty to show all."))
	void ShowEffectsByIDs(const TArray<FString>& EffectIDs);

	// Hide effects by providing an array of effect ID strings
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Hide effects by providing an array of effect IDs. Leave empty to hide all."))
	void HideEffectsByIDs(const TArray<FString>& EffectIDs);

	// Enable effects by providing an array of effect ID strings
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Enable effects by providing an array of effect IDs. Leave empty to enable all."))
	void EnableEffectsByIDs(const TArray<FString>& EffectIDs);

	// Disable effects by providing an array of effect ID strings
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Disable effects by providing an array of effect IDs. Leave empty to disable all."))
	void DisableEffectsByIDs(const TArray<FString>& EffectIDs);

	// JSON string version - C++ only, not exposed to Blueprint (internal use)
	void UploadCustomEffectsJson(const FString& EffectsJson);

	// Simple Blueprint function - just pass your effect info struct and it uploads it
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", DisplayName = "Upload Custom Effect"))
	void UploadCustomEffect(const FCrowdControlEffectInfo& EffectInfo);

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void UploadCustomTimedEffect(const FCrowdControlTimedEffectInfo& EffectInfo);

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void UploadCustomParameterEffect(const FCrowdControlParameterEffectInfo& EffectInfo);

	// Upload multiple effects in a single PUT request
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Upload multiple basic effects in a single PUT request"))
	void UploadCustomEffectsArray(const TArray<FCrowdControlEffectInfo>& Effects);

	// Upload multiple timed effects in a single PUT request
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Upload multiple timed effects in a single PUT request"))
	void UploadCustomTimedEffectsArray(const TArray<FCrowdControlTimedEffectInfo>& Effects);

	// Upload multiple parameter effects in a single PUT request
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Upload multiple parameter effects in a single PUT request"))
	void UploadCustomParameterEffectsArray(const TArray<FCrowdControlParameterEffectInfo>& Effects);

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	void ClearCustomEffects();

	// Delete all custom effects
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Delete all custom effects."))
	void DeleteCustomEffects();

	// Delete specific custom effects by their IDs (JSON array string)
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Delete custom effects by IDs (JSON array)."))
	void DeleteCustomEffectsByJson(const FString& EffectIDsJson);

	// Delete custom effects by providing an array of effect ID strings
	UFUNCTION(BlueprintCallable, Category = "Crowd Control", meta = (CallInEditor = "true", ToolTip = "Delete custom effects by providing an array of effect IDs. Leave empty to delete all."))
	void DeleteCustomEffectsByIDs(const TArray<FString>& EffectIDs);

	UFUNCTION(BlueprintCallable, Category = "Crowd Control")
	FString GetCustomEffects();

	// Effect components register here; triggers whose EffectID matches a registered
	// component are dispatched to that component instead of the global delegates.
	void RegisterEffectComponent(UCrowdControlEffectComponent* Component);
	void UnregisterEffectComponent(UCrowdControlEffectComponent* Component);

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
	static FP_Command CC_CommandFunction;
	
	typedef void (*ResetCommandType)();
	static ResetCommandType CC_ResetCommand;
	
	typedef void (*SetAppIDType)(const char* appID);
	SetAppIDType CC_SetAppID = nullptr;

	typedef void (*RequestAuthCodeType)();
	RequestAuthCodeType CC_RequestAuthCode = nullptr;

	typedef char* (*GetAuthCodeType)();
	GetAuthCodeType CC_GetAuthCode = nullptr;

	typedef void (*SetAutoStartSessionType)(bool autoStart);
	SetAutoStartSessionType CC_SetAutoStartSession = nullptr;

	typedef void (*SessionControlType)();
	SessionControlType CC_StartSession = nullptr;
	SessionControlType CC_StopSession = nullptr;

	typedef void (*EffectResponseMessageType)(const char* id, const char* message);
	EffectResponseMessageType CC_EffectSuccessWithMessage = nullptr;
	EffectResponseMessageType CC_EffectFailureWithMessage = nullptr;
	EffectResponseMessageType CC_EffectFailTemporary = nullptr;
	EffectResponseMessageType CC_EffectFailPermanent = nullptr;

	typedef bool (*ReportEffectStatusType)(const char* effectID, int status);
	ReportEffectStatusType CC_ReportEffectStatus = nullptr;

	typedef void (*SendPackMetadataType)(const char* metadataJson);
	SendPackMetadataType CC_SendPackMetadata = nullptr;

	typedef bool (*CloneEffectType)(const char* sourceEffectID, const char** destEffectIDs);
	CloneEffectType CC_CloneEffect = nullptr;

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

	typedef void (*UploadCustomEffectsType)(const char* effectsJson);
	UploadCustomEffectsType CC_UploadCustomEffects;

	typedef void (*ClearCustomEffectsType)();
	ClearCustomEffectsType CC_ClearCustomEffects;

	typedef void (*DeleteCustomEffectsType)(const char* effectIDsJson);
	DeleteCustomEffectsType CC_DeleteCustomEffects;

	typedef char* (*GetCustomEffectsType)();
	GetCustomEffectsType CC_GetCustomEffects;

	// CCEffectBase::ToggleVisible / ToggleSellable are C++ member functions rather than part of the C
	// API, so they are resolved by their decorated names and called with the effect as the this pointer.
	typedef void (*ToggleEffectFlagType)(void* Effect, bool bValue);
	ToggleEffectFlagType CC_ToggleVisible = nullptr;
	ToggleEffectFlagType CC_ToggleSellable = nullptr;

	// Address of CrowdControlRunner::effects inside the DLL. Cast to the effect map type in the .cpp.
	void* CC_EffectsMap = nullptr;

	TUniquePtr<FCrowdControlRunnable> Runnable = nullptr;
	
	static char * StringToSend;  // Declare as a static array with a fixed size

	static char** SplitCategories(TArray<FString> categories);
	static void FreeCategories(char** categoriesArray);

	FTSTicker::FDelegateHandle TickerHandle;

protected:

	void SetupWorldTimer(UWorld* World, const UWorld::InitializationValues);

	bool OnCoreTick(float DeltaTime);

	bool bIsConnected = false;
	bool bIsInitialized = false;

	TMap<FString, TWeakObjectPtr<UCrowdControlEffectComponent>> EffectComponents;

	FString MenuJson;
	TSharedPtr<FJsonObject> GameJsonObject;

	void* DLLHandle = nullptr;
	std::mutex QueueMutex;
    std::queue<std::function<void()>> MainThreadTasks;
	
	void Update();
	void Tick(float DeltaTime);
	int32 CommandID; 
};


