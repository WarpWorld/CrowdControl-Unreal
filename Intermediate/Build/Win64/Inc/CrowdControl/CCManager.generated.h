// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	C++ class header boilerplate exported from UnrealHeaderTool.
	This is automatically generated by the tools.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "ObjectBase.h"

#ifdef CROWDCONTROL_CCManager_generated_h
#error "CCManager.generated.h already included, missing '#pragma once' in CCManager.h"
#endif
#define CROWDCONTROL_CCManager_generated_h

#define ACCManager_EVENTPARMS
#define ACCManager_RPC_WRAPPERS \
	DECLARE_FUNCTION(execLoadDLL) \
	{ \
		P_GET_PROPERTY(UStrProperty,GameKey); \
		P_FINISH; \
		this->LoadDLL(GameKey); \
	}


#define ACCManager_CALLBACK_WRAPPERS
#define ACCManager_INCLASS \
	private: \
	static void StaticRegisterNativesACCManager(); \
	friend CROWDCONTROL_API class UClass* Z_Construct_UClass_ACCManager(); \
	public: \
	DECLARE_CLASS(ACCManager, AActor, COMPILED_IN_FLAGS(0), 0, CrowdControl, NO_API) \
	/** Standard constructor, called after all reflected properties have been initialized */    NO_API ACCManager(const class FPostConstructInitializeProperties& PCIP); \
	DECLARE_SERIALIZER(ACCManager) \
	/** Indicates whether the class is compiled into the engine */    enum {IsIntrinsic=COMPILED_IN_INTRINSIC};


#undef UCLASS_CURRENT_FILE_NAME
#define UCLASS_CURRENT_FILE_NAME ACCManager


#undef UCLASS
#undef UINTERFACE
#if UE_BUILD_DOCS
#define UCLASS(...)
#else
#define UCLASS(...) \
ACCManager_EVENTPARMS
#endif


#undef GENERATED_UCLASS_BODY
#undef GENERATED_IINTERFACE_BODY
#define GENERATED_UCLASS_BODY() \
public: \
	ACCManager_RPC_WRAPPERS \
	ACCManager_CALLBACK_WRAPPERS \
	ACCManager_INCLASS \
public:

