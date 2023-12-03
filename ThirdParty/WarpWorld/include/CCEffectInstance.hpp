#pragma once
#include <string>
#include <vector>
#include <memory>
#include "StreamUser.hpp"
#include "CCEffect.hpp"  
#include "DLLConfig.hpp"

class CC_API CCEffectInstance {
public:
	virtual ~CCEffectInstance() = default;

	std::shared_ptr<CCEffectBase> effect;
	std::shared_ptr<StreamUser> sender;
	std::string id;
	int retryCount = 0;
	long unscaledStartTime;
};
