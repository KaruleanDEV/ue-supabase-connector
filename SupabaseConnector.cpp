#include "SupabaseConnector.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "SupabaseStructs.h"
#include "JsonObjectConverter.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"

static bool GetSupabaseSettings(FString& OutUrl, FString& OutApiKey)
{
	OutUrl = FPlatformMisc::GetEnvironmentVariable(TEXT("SUPABASE_URL"));
	OutApiKey = FPlatformMisc::GetEnvironmentVariable(TEXT("SUPABASE_APIKEY"));

	if (OutUrl.IsEmpty() || OutApiKey.IsEmpty())
	{
		FString IniUrl;
		FString IniApiKey;
		if (GConfig)
		{
			GConfig->GetString(TEXT("Supabase"), TEXT("SupabaseUrl"), IniUrl, GGameIni);
			GConfig->GetString(TEXT("Supabase"), TEXT("SupabaseApiKey"), IniApiKey, GGameIni);
		}

		if (OutUrl.IsEmpty())
		{
			OutUrl = IniUrl;
		}
		if (OutApiKey.IsEmpty())
		{
			OutApiKey = IniApiKey;
		}
	}

	if (OutUrl.IsEmpty() || OutApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Supabase credentials missing. Set SUPABASE_URL and SUPABASE_APIKEY as environment variables or in [Supabase] section of DefaultGame.ini."));
		return false;
	}

	if (!OutUrl.EndsWith(TEXT("/")))
	{
		OutUrl += TEXT("/");
	}

	return true;
}

static FString ParseSupabaseErrorMessage(const FString& ResponseBody)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		FString Error;
		if (JsonObject->TryGetStringField(TEXT("msg"), Error))
			return Error;
		if (JsonObject->TryGetStringField(TEXT("error_description"), Error))
			return Error;
		if (JsonObject->TryGetStringField(TEXT("error"), Error))
			return Error;
	}

	return ResponseBody;
}

void USupabaseAuthLibrary::LoginUserAsync(const FString& Email, const FString& Password, const FOnLoginResponse& OnSuccess, const FOnAuthFailure& OnFailure)
{
	TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject());
	JsonBody->SetStringField(TEXT("email"), Email);
	JsonBody->SetStringField(TEXT("password"), Password);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

	FString SupabaseUrl;
	FString SupabaseApiKey;
	if (!GetSupabaseSettings(SupabaseUrl, SupabaseApiKey))
	{
		if (OnFailure.IsBound())
		{
			OnFailure.Execute(TEXT("Supabase settings are not configured."));
		}
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(SupabaseUrl + TEXT("auth/v1/token?grant_type=password"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("apikey"), SupabaseApiKey);
	Request->SetContentAsString(RequestBody);

	// Shared pointers to safely bind delegates in lambda
	TSharedPtr<FOnLoginResponse> SuccessPtr = MakeShared<FOnLoginResponse>(OnSuccess);
	TSharedPtr<FOnAuthFailure> FailurePtr = MakeShared<FOnAuthFailure>(OnFailure);

	// Response callback
	Request->OnProcessRequestComplete().BindLambda([SuccessPtr, FailurePtr](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			if (!bSuccess || !Resp.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("Login request failed (connection or timeout)"));
				if (FailurePtr->IsBound())
				{
					FailurePtr->Execute(TEXT("Connection failed or timeout"));
				}
				return;
			}

			if (!EHttpResponseCodes::IsOk(Resp->GetResponseCode()))
			{
				const FString RawError = Resp->GetContentAsString();
				const FString Error = ParseSupabaseErrorMessage(RawError);
				UE_LOG(LogTemp, Error, TEXT("Login failed (%d): %s"), Resp->GetResponseCode(), *Error);
				if (FailurePtr->IsBound())
				{
					FailurePtr->Execute(Error);
				}
				return;
			}

			FSupabaseLoginResponse LoginData;
			const FString ResponseStr = Resp->GetContentAsString();

			if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseStr, &LoginData, 0, 0))
			{
				UE_LOG(LogTemp, Log, TEXT("Login success: %s"), *LoginData.access_token, *LoginData.user.email);
				if (SuccessPtr->IsBound())
				{
					SuccessPtr->Execute(LoginData);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to parse login JSON"));
				if (FailurePtr->IsBound())
				{
					FailurePtr->Execute(TEXT("Failed to parse login JSON"));
				}
			}
		});

	Request->ProcessRequest();
}

void USupabaseAuthLibrary::RegisterUserAsync(const FString& Email, const FString& Password, const FOnLoginResponse& OnSuccess, const FOnAuthFailure& OnFailure)
{
    TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject());
    JsonBody->SetStringField(TEXT("email"), Email);
    JsonBody->SetStringField(TEXT("password"), Password);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

	FString SupabaseUrl;
	FString SupabaseApiKey;
	if (!GetSupabaseSettings(SupabaseUrl, SupabaseApiKey))
	{
		if (OnFailure.IsBound())
		{
			OnFailure.Execute(TEXT("Supabase settings are not configured."));
		}
		return;
	}

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SupabaseUrl + TEXT("auth/v1/signup"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("apikey"), SupabaseApiKey);
    Request->SetContentAsString(RequestBody);

    // Shared pointers to safely bind delegates in lambda
    TSharedPtr<FOnLoginResponse> SuccessPtr = MakeShared<FOnLoginResponse>(OnSuccess);
    TSharedPtr<FOnAuthFailure> FailurePtr = MakeShared<FOnAuthFailure>(OnFailure);

    // Response callback
    Request->OnProcessRequestComplete().BindLambda([SuccessPtr, FailurePtr](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            if (!bSuccess || !Resp.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Registration request failed."));
                if (FailurePtr->IsBound())
                {
                    FailurePtr->Execute(TEXT("Connection failed or timeout."));
                }
                return;
            }

            if (!EHttpResponseCodes::IsOk(Resp->GetResponseCode()))
            {
                const FString RawError = Resp->GetContentAsString();
                const FString Error = ParseSupabaseErrorMessage(RawError);
                UE_LOG(LogTemp, Error, TEXT("Registration failed (%d): %s"), Resp->GetResponseCode(), *Error);
                if (FailurePtr->IsBound())
                {
                    FailurePtr->Execute(Error);
                }
                return;
            }

            FSupabaseLoginResponse RegisterData;
            const FString ResponseStr = Resp->GetContentAsString();

            if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseStr, &RegisterData, 0, 0))
            {
                UE_LOG(LogTemp, Log, TEXT("Registration success: User ID = %s"), *RegisterData.user.id);
                if (SuccessPtr->IsBound())
                {
                    SuccessPtr->Execute(RegisterData);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to parse registration JSON."));
                if (FailurePtr->IsBound())
                {
                    FailurePtr->Execute(TEXT("Failed to parse registration JSON."));
                }
            }
        });

    Request->ProcessRequest();
}