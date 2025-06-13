#pragma once

#include "CompileSettings.h"

#if VH_IS_ON(VH_DISCORD_INTEGRATION)
#include <string>

#include <dpp/dpp.h>

#include "Peer.h"

class IDiscordManager {
private:
    std::unique_ptr<dpp::cluster> m_bot;

public:
    // Linked account map
    avledet::util::Map<std::string, dpp::snowflake, ankerl::unordered_dense::string_hash, std::equal_to<>> m_linked_accounts;

    // Every time an joins they will be sent a key
    //                  host,        key
    avledet::util::Map<std::string, std::pair<std::string, std::chrono::nanoseconds>, ankerl::unordered_dense::string_hash, std::equal_to<>> m_temp_linking_keys;

public:
    void init();
    void period_update();

    // Dont use, method shouldnt return a blank string
    //[[deprecated]] std::string GetHostBySnowflake(dpp::snowflake id) {
    //    for (auto&& pair : m_linkedAccounts) {
    //        if (pair.second == id) {
    //            return pair.first;
    //        }
    //    }
    //    return "";
    //}

    Peer* find_peer(dpp::snowflake id);

    Peer* unlink_peer(dpp::snowflake id);

    void send_webhook_message(std::string_view msg);
};

#define VH_DISPATCH_WEBHOOK(msg) DiscordManager()->send_webhook_message((msg));

IDiscordManager* DiscordManager();
#else
#define VH_DISPATCH_WEBHOOK(msg) {}
#endif