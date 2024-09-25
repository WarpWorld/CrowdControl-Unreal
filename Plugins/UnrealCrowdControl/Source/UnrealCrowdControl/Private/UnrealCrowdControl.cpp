// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealCrowdControl.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

#define LOCTEXT_NAMESPACE "FUnrealCrowdControlModule"

void FUnrealCrowdControlModule::StartupModule()
{
}

void FUnrealCrowdControlModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealCrowdControlModule, UnrealCrowdControl)
