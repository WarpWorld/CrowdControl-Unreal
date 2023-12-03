#pragma once
#include "CCEffectInstance.hpp"
#include "CCEffectParameters.hpp"
#include "DLLConfig.hpp"
#include <unordered_map>

class CC_API CCEffectInstanceParameters : public CCEffectInstance {
public:
	CCEffectParameters* timedEffect;

	std::unordered_map<std::string, std::string> parameters;

	int quantity = 0;

	void SetParam(std::string key, std::string value) {
		parameters[key] = value;
	}

	std::string GetParam(std::string key) {
		return parameters[key];
	}
};
