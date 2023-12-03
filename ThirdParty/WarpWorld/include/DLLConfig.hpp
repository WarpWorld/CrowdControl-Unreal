#pragma once

#ifdef CC_EXPORTS
#define CC_API __declspec(dllexport)
#else
#define CC_API __declspec(dllimport)
#endif
