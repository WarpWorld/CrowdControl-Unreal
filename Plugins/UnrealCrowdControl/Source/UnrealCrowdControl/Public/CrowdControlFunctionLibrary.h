#pragma once

#include "CrowdControlTypes.h"
#include "CrowdControlFunctionLibrary.generated.h"




UCLASS(Blueprintable)
class UNREALCROWDCONTROL_API UCrowdControlFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crowd Control")
	static FCrowdControlParameter MakeOptionParameter(FString Id, TArray<FString> options);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crowd Control")
	static FCrowdControlParameter MakeMinMaxParameter(FString Id, int32 min, int32 max);
};
