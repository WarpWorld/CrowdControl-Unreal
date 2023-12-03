#pragma once
#include "CCEffectInstance.hpp"
#include "CCEffectTimed.hpp"
#include "DLLConfig.hpp"
#include <chrono>

class CC_API CCEffectInstanceTimed : public CCEffectInstance {
public:
	CCEffectTimed* timedEffect;
	float runTime;

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point pauseTime;
	std::chrono::milliseconds pausedDuration = std::chrono::milliseconds(0);

	int TimeRemaining();
	void Pause();
	void Resume();
	void Reset();
	void Stop(bool force);
};

