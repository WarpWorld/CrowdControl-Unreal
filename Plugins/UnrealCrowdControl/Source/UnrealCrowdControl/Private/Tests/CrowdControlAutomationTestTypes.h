#pragma once

#include "CoreMinimal.h"
#include "CrowdControlAutomationTestTypes.generated.h"

UCLASS()
class UCrowdControlAuthDelegateTestObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void ReceiveAuthCode(FString Code, FString URL) {}
};
