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

};
