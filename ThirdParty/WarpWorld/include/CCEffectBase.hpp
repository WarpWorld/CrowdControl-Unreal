#pragma once
#include <string>
#include <vector>
#include <cctype>
#include <iostream>
#include <nlohmann/json.hpp>
#include "Morality.hpp"
#include "EffectResult.hpp"
#include "DLLConfig.hpp"

class CCEffectInstance;

class CC_API CCEffectBase {
public:
	virtual EffectResult OnTriggerEffect(CCEffectInstance* effectInstance) {
		return EffectResult::Failure;
	}
	// Constructors, destructors, and public member functions here
	float delayUntilUnscaledTime = 0.0f;

	CCEffectBase();
	void ToggleSellable(bool sellable);
	void ToggleVisible(bool visible);
	void UpdatePrice(unsigned int newPrice);
	//void UpdateNonPoolable(bool newNonPoolable);
	//void UpdateSessionMax(unsigned int newSessionMax);
	virtual bool CanBeRan();
	virtual bool HasParameterID(const std::string& id);

	std::string id;
	std::string displayName;

	void AssignName(std::string name) {
		std::cout << name << "\n";
		displayName = name;
		id = "";

		for (char c : displayName) {
			if (std::isalnum(c)) { 
				id += std::tolower(c); 
			}
		}
	}

	virtual nlohmann::json JSONManifest() {
		nlohmann::json manifest;
		manifest["disabled"] = !sellable;
		manifest["inactive"] = !visible;
		manifest["unpoolable"] = noPooling;
		manifest["name"] = displayName;
		manifest["category"] = categories;
		manifest["description"] = description;
		manifest["price"] = price;
		manifest["allignment"]["orderliness"] = static_cast<float>(orderliness) / 100;
		manifest["allignment"]["morality"] = static_cast<float>(morality) / 100;

		return manifest;
	}

private:
	bool noPooling = false;
	bool sellable = true;
	bool visible = true;
	Morality morality = Morality::Neutral;
	Orderliness orderliness = Orderliness::Neutral;
	std::string description;
	unsigned int price = 10;
	int maxRetries = 3;
	float retryDelay = 5.0f;
	float pendingDelay = 0.5f;
	unsigned int sessionMax = 0;
	std::vector<std::string> categories;
};
