#pragma once
#include "CCEffectBase.hpp"
#include "DLLConfig.hpp"

class CC_API ConcreteEffect : public CCEffectBase {
public:
	ConcreteEffect() {}

	EffectResult OnTriggerEffect(const CCEffectInstance& effectInstance) override {
		return EffectResult::Running;
	}
};
