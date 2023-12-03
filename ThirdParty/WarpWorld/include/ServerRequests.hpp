#pragma once
#include "DLLConfig.hpp"
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>  
#include <cpprest/json.h>  
#include <cpprest/uri.h>  
#include <cpprest/ws_client.h>  
#include <cpprest/containerstream.h>  
#include <cpprest/interopstream.h>  
#include <cpprest/rawptrstream.h>  
#include <cpprest/producerconsumerstream.h>
#include <iostream>
#include <functional>

class CC_API ServerRequests {
public:
	static pplx::task<void> RequestGet(const std::wstring& getType, std::function<void(const std::wstring&)> callback);
	static pplx::task<void> SendPost(const std::wstring& postType, std::function<void(const std::wstring&)> callback, web::json::value json = web::json::value(), bool gameSession = true);

	static bool makingRequest;
};
