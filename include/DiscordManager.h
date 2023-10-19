#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_DISCORD_INTEGRATION)
#include <string>

#include <isteamhttp.h>
#include <dpp/dpp.h>

#include "HashUtils.h"
#include "Peer.h"

class IDiscordManager {
private:
    std::unique_ptr<dpp::cluster> m_bot;

public:
    // Linked account map
    UNORDERED_MAP_t<std::string, dpp::snowflake, ankerl::unordered_dense::string_hash, std::equal_to<>> m_linkedAccounts;

    // Every time an joins they will be sent a key
    //                  host,        key
    UNORDERED_MAP_t<std::string, std::pair<std::string, nanoseconds>, ankerl::unordered_dense::string_hash, std::equal_to<>> m_tempLinkingKeys;

public:
    void init();
    void periodic_update();

    // Dont use, method shouldnt return a blank string
    //[[deprecated]] std::string GetHostBySnowflake(dpp::snowflake id) {
    //    for (auto&& pair : m_linkedAccounts) {
    //        if (pair.second == id) {
    //            return pair.first;
    //        }
    //    }
    //    return "";
    //}

    Peer* GetPeerBySnowflake(dpp::snowflake id);

    Peer* UnlinkPeerBySnowflake(dpp::snowflake id);

    void SendSimpleMessage(std::string_view msg);
};

#define VH_DISPATCH_WEBHOOK(msg) DiscordManager()->SendSimpleMessage((msg));

IDiscordManager* DiscordManager();
#else
#define VH_DISPATCH_WEBHOOK(msg) {}
#endif