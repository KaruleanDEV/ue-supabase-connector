#pragma once

#include "CoreMinimal.h"
#include "SupabaseStructs.generated.h"

USTRUCT(BlueprintType)
struct FSupabaseUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString id;

	UPROPERTY(BlueprintReadOnly)
	FString email;
};

USTRUCT(BlueprintType)
struct FSupabaseLoginResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString access_token;

	UPROPERTY(BlueprintReadOnly)
	FString refresh_token;

	UPROPERTY(BlueprintReadOnly)
	int32 expires_in;

	UPROPERTY(BlueprintReadOnly)
	FString token_type;

	UPROPERTY(BlueprintReadOnly)
	FSupabaseUser user;
};
