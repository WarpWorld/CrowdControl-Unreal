#pragma once

#include "CoreMinimal.h"

#include "CrowdControlTypes.generated.h"



USTRUCT(BlueprintType)
struct FCrowdControlParameter
{
	GENERATED_BODY()

	FCrowdControlParameter()
	{}
	FCrowdControlParameter(FString name, TArray<FString> options)
		: _id(name), _options(options), _min(0), _max(0)
	{}

	FCrowdControlParameter(FString name, int32 min, int32 max)
		: _id(name), _min(min), _max(max)
	{}
	
	bool IsValid() const { return !_id.IsEmpty(); }
	
	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	FString _id;
	
	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	TArray<FString> _options;
	
	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	int32 _min = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	int32 _max = 0;
};


USTRUCT(BlueprintType)
struct FCrowdControlEffectInfo
{
	GENERATED_BODY()

	//Must be lowercase, no spaces
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	FString id;

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	FString displayName;

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	FString description;

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	int32 price = 0;

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	int32 maxRetries = 0;
	
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	float retryDelay = 0.f;
	
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	float pendingDelay = 0.f;
	
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	bool sellable = false;
	
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	bool visible = false;
	
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	bool nonPoolable = false;
	
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	TArray<FString> categories;
	
	
};

USTRUCT(BlueprintType)
struct FCrowdControlTimedEffectInfo : public FCrowdControlEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	float duration = 1.f;
};

USTRUCT(BlueprintType)
struct FCrowdControlParameterEffectInfo : public FCrowdControlEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	TArray<FCrowdControlParameter> parameters;
};
