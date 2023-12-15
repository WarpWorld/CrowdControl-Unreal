#include "CrowdControl.h"
#include "CrowdControlParameter.h"

UCrowdControlParameter::UCrowdControlParameter(const class FPostConstructInitializeProperties& PCIP)
    : Super(PCIP)
{
    // Initial values for your properties
    _name = TEXT("");
}

UCrowdControlParameter* UCrowdControlParameter::OptionParameter(FString name, TArray<FString> options) {
	UCrowdControlParameter* parameter = NewObject<UCrowdControlParameter>(GetTransientPackage(), UCrowdControlParameter::StaticClass());
    parameter->_name = name;
	parameter->_options = options;
	return parameter;
}


UCrowdControlParameter* UCrowdControlParameter::MinMaxParameter(FString name, int32 min, int32 max) {
	UCrowdControlParameter* parameter = NewObject<UCrowdControlParameter>(GetTransientPackage(), UCrowdControlParameter::StaticClass());
    parameter->_name = name;
	parameter->_min = min;
	parameter->_max = max;
	return parameter;
}