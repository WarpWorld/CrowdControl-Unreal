#pragma once
#include "DLLConfig.hpp"
#include "CCEffectInstance.hpp"
#include "CCEffectInstanceTimed.hpp"
#include <string>

class CC_API RPC {
public:
	static void Success(CCEffectInstance& instance);
	static void FailTemporarily(CCEffectInstance& instance);
	static void FailPermanently(CCEffectInstance& instance);
	static void TimedBegin(CCEffectInstanceTimed& instance);
	static void TimedPause(CCEffectInstanceTimed& instance);
	static void TimedResume(CCEffectInstanceTimed& instance);
	static void TimedEnd(CCEffectInstanceTimed& instance);
	static void MenuAvailable(CCEffectBase& instance);
	static void MenuUnavailable(CCEffectBase& instance);
	static void MenuVisible(CCEffectBase& instance);
	static void MenuHidden(CCEffectBase& instance);

private:
	static void Send(CCEffectBase& instance, const std::string& command);
	static void Send(CCEffectInstance& instance, const std::string& command);
};

