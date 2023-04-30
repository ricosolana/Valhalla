#pragma once

#include <string>

#include <isteamhttp.h>
#include <dpp/dpp.h>

#include "VUtils.h"

class IDiscordManager {
private:
    void OnHTTPRequestCompleted(HTTPRequestCompleted_t* pCallback, bool failure);
    CCallResult<IDiscordManager, HTTPRequestCompleted_t> m_httpRequestCompletedCallResult;

    std::unique_ptr<dpp::cluster> m_bot;

public:
    // Linked account map
    UNORDERED_MAP_t<std::string, dpp::snowflake> m_linkedAccounts;

    // Every time an joins they will be sent a key
    //                  host,        key
    UNORDERED_MAP_t<std::string, std::string> m_tempLinkingKeys;

public:
    void Init();
    void PeriodUpdate();

    void SendSimpleMessage(std::string_view msg);

    void DispatchRequest(std::string_view webhook, BYTES_t payload);
};

#define VH_DISPATCH_WEBHOOK(msg) DiscordManager()->SendSimpleMessage((msg));

IDiscordManager* DiscordManager();