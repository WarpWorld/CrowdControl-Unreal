#pragma once

#include "CoreMinimal.h"

#include "CrowdControlTypes.generated.h"

UENUM(BlueprintType)
enum class ECrowdControlParamType : uint8 {
	OPTIONS = 0 UMETA(DisplayName = "options"),
	HexColor = 1  UMETA(DisplayName = "hex-color"),
};

USTRUCT(BlueprintType)
struct FCrowdControlParamOption
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	FString id;

	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	FString DisplayName;
};

USTRUCT(BlueprintType)
struct FCrowdControlParameter
{
	GENERATED_BODY()

	FCrowdControlParameter()
	{}

	FCrowdControlParameter(FString inId, FString inName, ECrowdControlParamType inType, TArray<FCrowdControlParamOption> inOptions)
		: _id(inId), name(inName), type(inType), _options(inOptions)
	{}

	FCrowdControlParameter(FString name)
		: _id(name)
	{}
	
	bool IsValid() const { return !_id.IsEmpty(); }
	
	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	FString _id;

	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	FString name;

	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	ECrowdControlParamType type;
	
	UPROPERTY(BlueprintReadWrite, Category = "Crowd Control Parameter")
	TArray<FCrowdControlParamOption> _options;
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
	int32 price = 1;

	
	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	TArray<FString> category;
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
	bool RequiresQuantity = false;

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	FInt32Range quantity = FInt32Range(1, 99);

	UPROPERTY(BlueprintReadWrite, Category = CrowdControlParameterEffectInfo)
	TArray<FCrowdControlParameter> parameters;
};
