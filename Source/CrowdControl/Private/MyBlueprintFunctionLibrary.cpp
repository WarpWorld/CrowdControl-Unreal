// Fill out your copyright notice in the Description page of Project Settings.

#include "CrowdControl.h"
#include "MyBlueprintFunctionLibrary.h"


UMyBlueprintFunctionLibrary::UMyBlueprintFunctionLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

int32 UMyBlueprintFunctionLibrary::AddTwoIntegers(int32 A, int32 B)
{
    return A + B;
}



