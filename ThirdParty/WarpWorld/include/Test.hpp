#pragma once 

#include "DLLConfig.hpp"
#include <string> 
#include <functional>

class CC_API Test {
public:
	int Run();
	void TestNum(int test);
	void SetSiteCallback(void(*callback)());  // Function to set the callback

private:
	void(*siteCallback)();  // Function pointer for the callback
};


