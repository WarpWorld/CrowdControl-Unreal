// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CrowdControlDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "CrowdControlSettings"))
class UNREALCROWDCONTROL_API UCrowdControlDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString GamePackID = "UnrealDemo";

	UPROPERTY(config, EditAnywhere, Category=Config, meta=(Categories="Config"))
	FString GameName = "Unreal Demo";

};
