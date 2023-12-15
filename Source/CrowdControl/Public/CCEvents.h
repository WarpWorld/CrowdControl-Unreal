#pragma once
#include "Engine/GameInstance.h"
#include "CCEvents.generated.h"

UCLASS()
class CROWDCONTROL_API UCCEvents : public UObject
{
	GENERATED_UCLASS_BODY()
	
public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMyStaticEventSignature);
    static FMyStaticEventSignature MyStaticEvent;
};

