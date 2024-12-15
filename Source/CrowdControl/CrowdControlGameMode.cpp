// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrowdControlGameMode.h"
#include "CrowdControlCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACrowdControlGameMode::ACrowdControlGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
