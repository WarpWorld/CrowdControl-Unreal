#pragma once
#include "CCEffectBase.hpp"
#include "DLLConfig.hpp"

class CCEffectInstanceTimed;

class CC_API CCEffectTimed : public CCEffectBase {
public:
	float duration = 30;
	bool paused = false;

	CCEffectTimed();
	bool ShouldBeRunning();

	virtual EffectResult OnTriggerEffect(CCEffectInstanceTimed* effectInstance) = 0;
	virtual bool OnStopEffect(bool force) = 0;
	virtual void OnPause() = 0;
	virtual void OnResume() = 0;
	virtual void OnReset() = 0;
	virtual void OnUpdate() = 0;
	virtual bool RunningCondition() = 0;

	void SetDuration(int durationTime);

	nlohmann::json JSONManifest() override {
		nlohmann::json manifest = CCEffectBase::JSONManifest();

		manifest["duration"]["value"] = duration;

		return manifest;
	}
};

