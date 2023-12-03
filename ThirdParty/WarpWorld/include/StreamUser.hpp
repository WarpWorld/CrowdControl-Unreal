#pragma once
#include <string>
#include <vector>   // Include vector
#include <nlohmann/json.hpp>
#include <iostream>
#include "DLLConfig.hpp"

class CC_API StreamUser {
public:
	std::string name;
	std::string profileIconUrl;
	std::vector<std::string> roles;  
	std::vector<std::string> subscriptions; 
	unsigned int coinsSpent = 0;
	std::string displayName;
	std::string originSite;
	std::string email;
	std::string originID;

	// Constructors
	StreamUser();
	StreamUser(nlohmann::json);
	void Streamer(nlohmann::json);
	void StreamUserFromEffect(nlohmann::json);
	void LocalUser();
};
