#pragma once

#include "CoreMinimal.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace CrowdControlSessionStartError
{
	struct FDetails
	{
		FString GamePackID;
		FString ResponseBody;
		FString TransportError;
		int32 HttpStatusCode = 0;
	};

	inline bool Parse(const FString& ResultJson, FDetails& OutDetails)
	{
		TSharedPtr<FJsonObject> Result;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResultJson);
		double StatusCode = 0.0;
		if (!FJsonSerializer::Deserialize(Reader, Result)
			|| !Result.IsValid()
			|| !Result->TryGetNumberField(TEXT("httpStatus"), StatusCode)
			|| !Result->TryGetStringField(TEXT("gamePackID"), OutDetails.GamePackID)
			|| !Result->TryGetStringField(TEXT("responseBody"), OutDetails.ResponseBody)
			|| !Result->TryGetStringField(TEXT("transportError"), OutDetails.TransportError))
		{
			return false;
		}

		OutDetails.HttpStatusCode = static_cast<int32>(StatusCode);
		return true;
	}

	inline bool IsMissingGamePackResponse(const int32 HttpStatusCode, const FString& ResponseBody)
	{
		if (HttpStatusCode != 500)
		{
			return false;
		}

		TSharedPtr<FJsonObject> Response;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
		if (!FJsonSerializer::Deserialize(Reader, Response) || !Response.IsValid())
		{
			return false;
		}

		FString ErrorCode;
		const TSharedPtr<FJsonObject>* Data = nullptr;
		if (!Response->TryGetStringField(TEXT("code"), ErrorCode)
			|| ErrorCode != TEXT("INTERNAL_SERVER_ERROR")
			|| !Response->TryGetObjectField(TEXT("data"), Data)
			|| Data == nullptr
			|| !Data->IsValid())
		{
			return false;
		}

		FString Path;
		FString Stack;
		return (*Data)->TryGetStringField(TEXT("path"), Path)
			&& Path == TEXT("gameSession.startSession")
			&& (*Data)->TryGetStringField(TEXT("stack"), Stack)
			&& Stack.StartsWith(TEXT("Error: Game not found"), ESearchCase::CaseSensitive);
	}

	inline FString BuildUserMessage(
		const FString& GamePackID,
		const int32 HttpStatusCode,
		const FString& ResponseBody,
		const FString& TransportError)
	{
		if (IsMissingGamePackResponse(HttpStatusCode, ResponseBody))
		{
			return FString::Printf(
				TEXT("Cannot start Crowd Control session for game pack '%s'. Verify the Game ID and upload the effect/menu JSON in the Crowd Control developer portal, then connect again."),
				*GamePackID);
		}

		if (HttpStatusCode == 0 && !TransportError.IsEmpty())
		{
			return FString::Printf(
				TEXT("Could not start Crowd Control session for game pack '%s' because the request failed before a server response. Check your connection and try again."),
				*GamePackID);
		}

		const FString Status = HttpStatusCode > 0
			? FString::Printf(TEXT("HTTP %d"), HttpStatusCode)
			: FString(TEXT("no HTTP status"));
		return FString::Printf(
			TEXT("Could not start Crowd Control session for game pack '%s' (%s). See diagnostics for details."),
			*GamePackID,
			*Status);
	}
}
