#pragma once
#include "CCEffectBase.hpp"
#include "ParameterEntry.hpp"
#include "DLLConfig.hpp"
#include <unordered_map>

class CCEffectInstanceParameters;

class CC_API CCEffectParameters : public CCEffectBase {
public:
	CCEffectParameters();

	std::unordered_map<std::string, ParameterEntry> parameterEntries;

	virtual EffectResult OnTriggerEffect(CCEffectInstanceParameters* effectInstance) = 0;
	virtual void SetupParams() = 0;

	nlohmann::json JSONManifest() override {
		nlohmann::json manifest = CCEffectBase::JSONManifest();

		for (const auto& pair : parameterEntries) {
			manifest["parameters"][pair.first] = pair.second.JSONManifest();
		}

		return manifest;
	}
};
