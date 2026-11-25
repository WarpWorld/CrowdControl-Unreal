// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "CrowdControlDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "CrowdControlSettings"))
class UNREALCROWDCONTROL_API UCrowdControlDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString GamePackID = "UnrealDemo";

	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString GameName = "Unreal Demo";

	// Application ID issued by Crowd Control. When set, authentication uses the
	// appID auth-code flow (OnAuthCodeReceived fires with a code/URL for the user)
	// instead of the legacy Twitch/YouTube/Discord platform login.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString ApplicationID = "";

	// Public Client Key issued with your Application (also called the app secret).
	// Sent with the auth-code token exchange. Safe to ship in a client build.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString PublicClientKey = "";

	// Automatically start a game session once authenticated and subscribed.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	bool bStartSessionAutomatically = true;

	// Connect to the local dev server started with `crowdcontrol dev` instead of Crowd Control.
	// It runs on your own machine and gives you a simulated viewer menu to send effects from,
	// with no account and nothing reaching the live service. Ignored in Shipping builds.
	UPROPERTY(config, EditAnywhere, Category="Local Dev Server", meta=(DisplayName="Use Local Dev Server"))
	bool bUseLocalDevServer = false;

	// The host the local dev server listens on. Leave as 127.0.0.1 unless it runs on another machine.
	UPROPERTY(config, EditAnywhere, Category="Local Dev Server", meta=(EditCondition="bUseLocalDevServer"))
	FString LocalDevServerHost = "127.0.0.1";

	// The port the local dev server listens on. 4770 unless it was started with --port.
	UPROPERTY(config, EditAnywhere, Category="Local Dev Server", meta=(EditCondition="bUseLocalDevServer", ClampMin="1", ClampMax="65535"))
	int32 LocalDevServerPort = 4770;

};
