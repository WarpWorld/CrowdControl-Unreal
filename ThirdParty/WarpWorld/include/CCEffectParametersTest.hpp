#pragma once
#include "CCEffectParameters.hpp" 
#include "CCEffectInstanceParameters.hpp"
#include "DLLConfig.hpp"
#include <iostream>

class CC_API CCEffectParametersTest : public CCEffectParameters {
public:
	CCEffectParametersTest() = default;

	EffectResult OnTriggerEffect(CCEffectInstanceParameters* effectInstance) override;
	void SetupParams() override;
};