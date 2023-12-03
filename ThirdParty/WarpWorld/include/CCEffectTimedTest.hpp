#pragma once
#include "CCEffectTimed.hpp"
#include "DLLConfig.hpp"
#include <iostream>

class CC_API CCEffectTimedTest : public CCEffectTimed {
public:
	CCEffectTimedTest();

	EffectResult OnTriggerEffect(CCEffectInstanceTimed* effectInstance) override;
	bool OnStopEffect(bool force) override;
	void OnPause() override;
	void OnResume() override;
	void OnReset() override;
	void OnUpdate() override;
	bool RunningCondition() override;
};
