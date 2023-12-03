#pragma once
#include <string>
#include <unordered_map>
#include "ParameterOption.hpp"
#include <iostream>
#include "DLLConfig.hpp"

class CC_API ParameterEntry {
public: 
	ParameterEntry() = default;

	enum class Kind {
		Item,
		Quantity
	}; 

	Kind kind;

	std::string id;
	std::string displayName;

	std::unordered_map<std::string, ParameterOption> options;

	int min = 0;
	int max = 0;

	ParameterEntry(std::string name, std::string parentName, Kind _kind)  {
		displayName = name;
		id = parentName + "_" + SafeName(displayName);
		kind = _kind;
	}

	std::string SafeName(std::string name) {
		std::string safeName = "";

		for (char c : name) {
			if (std::isalnum(c)) {
				safeName += std::tolower(c);
			}
		}

		return safeName;
	}

	void AddOption(std::string name) {
		ParameterOption option(id + "_" + SafeName(name), name);
		options[option.id] = option;
	}

	void SetMinMax(int _min, int _max) {
		min = _min;
		max = _max;
	}

	nlohmann::json JSONManifest() const {
		nlohmann::json manifest;

		manifest["name"] = displayName;

		if (kind == Kind::Item) {
			manifest["type"] = "options";

			for (const auto& pair : options) {
				manifest[pair.first]["name"] = pair.second.displayName;
			}
		}
		else {
			manifest["quantity"]["min"] = min;
			manifest["quantity"]["max"] = max;
		}

		return manifest;
	}
};

