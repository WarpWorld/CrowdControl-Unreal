// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	C++ class header boilerplate exported from UnrealHeaderTool.
	This is automatically generated by the tools.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "ObjectBase.h"

#ifdef CROWDCONTROL_MyStreambufClass_generated_h
#error "MyStreambufClass.generated.h already included, missing '#pragma once' in MyStreambufClass.h"
#endif
#define CROWDCONTROL_MyStreambufClass_generated_h

#define UMyStreambufClass_EVENTPARMS
#define UMyStreambufClass_RPC_WRAPPERS
#define UMyStreambufClass_CALLBACK_WRAPPERS
#define UMyStreambufClass_INCLASS \
	private: \
	static void StaticRegisterNativesUMyStreambufClass(); \
	friend CROWDCONTROL_API class UClass* Z_Construct_UClass_UMyStreambufClass(); \
	public: \
	DECLARE_CLASS(UMyStreambufClass, UObject, COMPILED_IN_FLAGS(0), 0, CrowdControl, NO_API) \
	/** Standard constructor, called after all reflected properties have been initialized */    NO_API UMyStreambufClass(const class FPostConstructInitializeProperties& PCIP); \
	DECLARE_SERIALIZER(UMyStreambufClass) \
	/** Indicates whether the class is compiled into the engine */    enum {IsIntrinsic=COMPILED_IN_INTRINSIC};


#undef UCLASS_CURRENT_FILE_NAME
#define UCLASS_CURRENT_FILE_NAME UMyStreambufClass


#undef UCLASS
#undef UINTERFACE
#if UE_BUILD_DOCS
#define UCLASS(...)
#else
#define UCLASS(...) \
UMyStreambufClass_EVENTPARMS
#endif


#undef GENERATED_UCLASS_BODY
#undef GENERATED_IINTERFACE_BODY
#define GENERATED_UCLASS_BODY() \
public: \
	UMyStreambufClass_RPC_WRAPPERS \
	UMyStreambufClass_CALLBACK_WRAPPERS \
	UMyStreambufClass_INCLASS \
public:

