#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace CrowdControlManifest
{
	template <typename TDelegate>
	bool HasInvokableListeners(const TDelegate& Delegate)
	{
		return Delegate.GetAllObjects().Num() > 0;
	}

	template <typename TDelegate>
	bool ShouldOpenAuthURL(const TDelegate& Delegate, const bool bEnabled)
	{
		return bEnabled && !HasInvokableListeners(Delegate);
	}

	inline TSharedPtr<FJsonObject> CreateRoot(
		const TSharedPtr<FJsonObject>& GameEffects,
		const TSharedPtr<FJsonObject>& GameEvents)
	{
		TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetStringField(TEXT("kind"), TEXT("built-in"));
		Root->SetNumberField(TEXT("format"), 1);

		TSharedPtr<FJsonObject> Effects = MakeShared<FJsonObject>();
		Effects->SetObjectField(TEXT("game"), GameEffects);
		Root->SetObjectField(TEXT("effects"), Effects);

		if (GameEvents.IsValid() && !GameEvents->Values.IsEmpty())
		{
			TSharedPtr<FJsonObject> Meta = MakeShared<FJsonObject>();
			Meta->SetObjectField(TEXT("events"), GameEvents);
			Root->SetObjectField(TEXT("meta"), Meta);
		}

		return Root;
	}

	inline bool GetGenerationFailure(
		const TMap<FString, TPair<FString, FString>>& Errors,
		FString& OutStage,
		FString& OutDetail)
	{
		bool bFoundError = false;
		for (const TPair<FString, TPair<FString, FString>>& Error : Errors)
		{
			if (!bFoundError)
			{
				OutStage = Error.Value.Key;
				OutDetail = Error.Value.Value;
				bFoundError = true;
			}
		}

		if (!bFoundError)
		{
			return false;
		}

		if (Errors.Num() > 1)
		{
			OutDetail = FString::Printf(TEXT("%s (and %d additional generation error(s); see Output Log)"),
				*OutDetail,
				Errors.Num() - 1);
		}
		return true;
	}
}
