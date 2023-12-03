#pragma once
#include "CCEffectBase.hpp"
#include "DLLConfig.hpp"

class CC_API CCEffect : public CCEffectBase {
public:
	CCEffect();

	// Changed to take a pointer
	virtual EffectResult OnTriggerEffect(CCEffectInstance* effectInstance) override {
		return EffectResult::Failure;
	}
};

