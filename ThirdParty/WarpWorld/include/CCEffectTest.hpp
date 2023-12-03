#pragma once
#include "CCEffect.hpp"
#include "DLLConfig.hpp"
#include <iostream>

class CC_API CCEffectTest : public CCEffect {
public:
	CCEffectTest();

	EffectResult OnTriggerEffect(CCEffectInstance* effectInstance) override;
};
