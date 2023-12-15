#pragma once
#include "Object.h"
#include "CrowdControlParameter.generated.h"

UCLASS(BlueprintType)
class UCrowdControlParameter : public UObject
{
    GENERATED_UCLASS_BODY()

public:
	UCrowdControlParameter();
    FString _name;
	TArray<FString> _options;
	int32 _min;
	int32 _max;

	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static UCrowdControlParameter* OptionParameter(FString name, TArray<FString> options);
	
	UFUNCTION(BlueprintCallable, Category="Crowd Control")
	static UCrowdControlParameter* MinMaxParameter(FString name, int32 min, int32 max);
};
