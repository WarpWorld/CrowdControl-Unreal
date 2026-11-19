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
	// Game pack ID issued by Crowd Control. This is used to identify the game and its effects.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString GamePackID = "UnrealDemo";

	// Optional game version sent as X-CC-Version. Leave empty to omit the header.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString GameVersion = "";

	// Application ID issued by Crowd Control. When set, authentication uses the
	// appID auth-code flow (OnAuthCodeReceived fires with a code/URL for the user)
	// instead of the legacy Twitch/YouTube/Discord platform login.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString ApplicationID = "";

	// Automatically start a game session once authenticated and subscribed.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	bool bStartSessionAutomatically = true;

	// When true, automatically opens the auth URL in a browser if no auth-code listeners are bound.
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	bool bOpenAuthURLWhenUnhandled = true;

};
