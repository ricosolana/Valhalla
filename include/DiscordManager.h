#pragma once

#include <string>

#include <isteamhttp.h>
#include <dpp/dpp.h>

#include "VUtils.h"

class IDiscordManager {
private:
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

    void SendSimpleMessage(std::string msg);
};

#define VH_DISPATCH_WEBHOOK(msg) DiscordManager()->SendSimpleMessage((msg));

IDiscordManager* DiscordManager();