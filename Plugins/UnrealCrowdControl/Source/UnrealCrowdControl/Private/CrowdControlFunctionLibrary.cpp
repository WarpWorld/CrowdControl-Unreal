#include "CrowdControlFunctionLibrary.h"


FCrowdControlParameter UCrowdControlFunctionLibrary::MakeOptionParameter(FString Id, TArray<FString> options)
{
	return FCrowdControlParameter(Id, options);	
}


FCrowdControlParameter UCrowdControlFunctionLibrary::MakeMinMaxParameter(FString Id, int32 min, int32 max)
{
	return FCrowdControlParameter(Id, min, max);
}