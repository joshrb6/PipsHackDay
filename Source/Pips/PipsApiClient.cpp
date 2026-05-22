#include "PipsApiClient.h"
#include "PipsJsonParser.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

UPipsApiClient::UPipsApiClient()
{
    // Config defaults are loaded automatically via UPROPERTY(Config).
}

void UPipsApiClient::FetchPuzzleForDate(const FString& Date, FOnPipsApiResult Callback)
{
    const FString ConfigPath = FPaths::ProjectConfigDir() / TEXT("PipsConfig.ini");
    GConfig->GetString(TEXT("/Script/Pips.PipsApiClient"), TEXT("ApiBearerToken"),
                       ApiBearerToken, ConfigPath);
    GConfig->GetString(TEXT("/Script/Pips.PipsApiClient"), TEXT("BaseUrl"),
                       BaseUrl, ConfigPath);
    
    if (ApiBearerToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("PipsApiClient: ApiBearerToken is empty. Set it in DefaultGame.ini under [/Script/Pips.PipsApiClient]."));
        Callback.ExecuteIfBound(false, FPipsDailyData());
        return;
    }

    const FString Url = FString::Printf(TEXT("%s/%s"), *BaseUrl, *Date);
    UE_LOG(LogTemp, Display, TEXT("PipsApiClient: GET %s"), *Url);

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiBearerToken));
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

    Request->OnProcessRequestComplete().BindLambda(
        [Callback, Date](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
        {
            if (!bSuccess || !Resp.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("PipsApiClient: request failed for %s (network error)"), *Date);
                Callback.ExecuteIfBound(false, FPipsDailyData());
                return;
            }

            const int32 Code = Resp->GetResponseCode();
            if (Code != 200)
            {
                UE_LOG(LogTemp, Error, TEXT("PipsApiClient: HTTP %d for %s. Body: %s"),
                    Code, *Date, *Resp->GetContentAsString());
                Callback.ExecuteIfBound(false, FPipsDailyData());
                return;
            }

            const FString Body = Resp->GetContentAsString();
            FPipsDailyData Parsed;
            if (!UPipsJsonParser::ParseDaily(Body, Parsed))
            {
                UE_LOG(LogTemp, Error, TEXT("PipsApiClient: parse failed for %s"), *Date);
                Callback.ExecuteIfBound(false, FPipsDailyData());
                return;
            }

            UE_LOG(LogTemp, Display, TEXT("PipsApiClient: success for %s"), *Date);
            Callback.ExecuteIfBound(true, Parsed);
        });

    Request->ProcessRequest();
}