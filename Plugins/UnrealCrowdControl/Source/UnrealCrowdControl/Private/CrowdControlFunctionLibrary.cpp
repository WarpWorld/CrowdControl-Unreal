#include "CrowdControlFunctionLibrary.h"


FCrowdControlParameter UCrowdControlFunctionLibrary::MakeOptionParameter(FString Id, FString name, ECrowdControlParamType type, TArray<FCrowdControlParamOption> options)
{
	return FCrowdControlParameter(Id, name, type, options);	
}


FCrowdControlParameter UCrowdControlFunctionLibrary::MakeMinMaxParameter(FString Id, int32 min, int32 max)
{
	return FCrowdControlParameter(Id);
}