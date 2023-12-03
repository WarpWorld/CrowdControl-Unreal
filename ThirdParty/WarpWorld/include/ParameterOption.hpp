#pragma once
#include <string>
#include <unordered_map>
#include "DLLConfig.hpp"

class CC_API ParameterOption {
public:
	std::string id;
	std::string displayName;

	ParameterOption() = default;

	ParameterOption(const std::string& _id, const std::string& _displayName) : id(id), displayName(displayName) {
		id = _id;
		displayName = _displayName;
	}
};

