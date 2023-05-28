#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_DISCORD_INTEGRATION)
#include <string>

#include <isteamhttp.h>
#include <dpp/dpp.h>

#include "HashUtils.h"

class IDiscordManager {
private:
    std::unique_ptr<dpp::cluster> m_bot;

public:
    // Linked account map
    UNORDERED_MAP_t<std::string, dpp::snowflake, ankerl::unordered_dense::string_hash, std::equal_to<>> m_linkedAccounts;

    // Every time an joins they will be sent a key
    //                  host,        key
    UNORDERED_MAP_t<std::string, std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_tempLinkingKeys;

public:
    void Init();
    void PeriodUpdate();

    void SendSimpleMessage(std::string_view msg);
};

#define VH_DISPATCH_WEBHOOK(msg) DiscordManager()->SendSimpleMessage((msg));

IDiscordManager* DiscordManager();
#else
#define VH_DISPATCH_WEBHOOK(msg) {}
#endif