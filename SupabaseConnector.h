/*
Name: SupabaseConnector
Version: 0.1
Maintainer: Karulean Vermillion
Description: A connector for Supabase services. Handle authentication, database operations, and real-time updates.
*/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SupabaseConnector.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnLoginResponse, const FSupabaseLoginResponse&, Response);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAuthFailure, const FString&, ErrorMessage);


UCLASS()
class USupabaseAuthLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Supabase")
	static void LoginUserAsync(const FString& Email, const FString& Password, const FOnLoginResponse& OnSuccess, const FOnAuthFailure& OnFailure);

	UFUNCTION(BlueprintCallable, Category = "Supabase")
	static void RegisterUserAsync(const FString& Email, const FString& Password, const FOnLoginResponse& OnSuccess, const FOnAuthFailure& OnFailure);

};
