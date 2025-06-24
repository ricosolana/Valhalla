#include "DiscordManager.h"
#include "ModManager.h"
#include "WorldManager.h"
#include <dpp/intents.h>
#include <dpp/queues.h>

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)

    #include <dpp/appcommand.h>
    #include <dpp/dispatcher.h>
    #include <dpp/dpp.h>
    #include <dpp/restresults.h>
    #include <isteamgameserver.h>
    #include <range/v3/all.hpp>

    #include "NetManager.h"
    #include "Peer.h"
    #include "RandomEventManager.h"
    #include "ValhallaServer.h"
    #include "ZDOManager.h"

auto DISCORD_MANAGER(std::make_unique<IDiscordManager>());

IDiscordManager *DiscordManager()
{
    return DISCORD_MANAGER.get();
}

/*
class DiscordClientInterceptor : public dpp::discord_client {
public:

};*/

void IDiscordManager::init()
{
    if (!AVL_SETTINGS.discordEnabled || AVL_SETTINGS.discordToken.empty())
        return;

    LOG_INFO(AVL_LOGGER, "Initializing DiscordManager");

    // https://dpp.dev/slashcommands.html

    uint64_t intents = dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members;
    m_bot            = std::make_unique<dpp::cluster>(AVL_SETTINGS.discordToken, intents);

    m_bot->on_log([](dpp::log_t const &log) {
        switch (log.severity) {
        case dpp::loglevel::ll_trace: LOG_TRACE_L1(AVL_LOGGER, "{}", log.message); break;
        case dpp::loglevel::ll_debug: LOG_DEBUG(AVL_LOGGER, "{}", log.message); break;
        case dpp::loglevel::ll_info: LOG_INFO(AVL_LOGGER, "{}", log.message); break;
        case dpp::loglevel::ll_warning: LOG_WARNING(AVL_LOGGER, "{}", log.message); break;
        case dpp::loglevel::ll_error: LOG_ERROR(AVL_LOGGER, "{}", log.message); break;
        case dpp::loglevel::ll_critical: LOG_CRITICAL(AVL_LOGGER, "{}", log.message); break;
        }
    });

    m_bot->on_slashcommand([this](dpp::slashcommand_t const &event) {
        //event.thinking(true);

        auto label = event.command.get_command_name();

        if (label == "avlfreset") {
            event.reply("Deleting commands...");
            m_bot->global_commands_get([this, event](dpp::confirmation_callback_t const &cb) {
                if (!cb.is_error()) {
                    auto &&commands = std::get<dpp::slashcommand_map>(cb.value);
                    for (auto &&command : commands) {
                        if (command.second.name == "avlfreset" || command.second.name == "avlfreg")
                            continue;
                        m_bot->global_command_delete(command.first);
                    }
                    //event.reply("Deleted commands...");
                    m_bot->interaction_followup_create(event.command.token,
                                                       dpp::message("Deleted all commands!"),
                                                       [](dpp::confirmation_callback_t const &) {});
                }
            });
        } else {
            Avledet()->RunTask([this, label, event](Task &) {
                if (label == "avladmin") {
                    auto &&admin = Avledet()->m_admin;

                    auto param_variant = event.get_parameter("identifier");
                    auto flag_variant  = event.get_parameter("flag");
                    auto &&identifier  = std::get_if<std::string>(&param_variant);
                    auto &&flag        = std::get_if<bool>(&flag_variant);
                    if (identifier) {
                        if (auto &&peer = NetManager()->FindPeer(*identifier)) {
                            if (flag) {
                                peer->SetAdmin(*flag);
                                if (*flag)
                                    event.reply("Granted admin to player");
                                else
                                    event.reply("Revoked admin from player");
                            } else {
                                event.reply(std::string("Player is ") + (peer->IsAdmin() ? "" : "not ")
                                            + "an admin");
                            }
                        } else {
                            event.reply("Player not found");
                        }

                        /*
						if (flag) {
							if (*flag) {
								admin.insert(identifier)
							}
						}*/

                    } else {
                        // reply with a list of all admins
                        if (admin.empty()) {
                            event.reply("There are no players with admin privileges");
                        } else {
                            std::string msg = "Players with admin: \n";
                            for (auto &&host : admin) {
                                msg += " - " + host + "\n";
                            }
                            event.reply(msg);
                        }
                    }
                } else if (label == "avlban") {
                    auto &&identifier = std::get<std::string>(event.get_parameter("identifier"));
                    if (auto peer = NetManager()->Ban(identifier))
                        event.reply("Banned " + peer->m_name + " (" + peer->m_socket->get_host_name() + ")");
                    else
                        event.reply("Player not found");
                } else if (label == "avlbroadcast") {
                    auto &&message = std::get<std::string>(event.get_parameter("message"));
                    Avledet()->Broadcast(UIMsgType::Center, message);
                    event.reply("Broadcasted message to all players");
                } else if (label == "avlevent") {
                    if (auto &&e = RandomEventManager()->GetEvent(
                                std::get<std::string>(event.get_parameter("event")))) {
                        auto &&peer = NetManager()->FindPeer(
                                std::get<std::string>(event.get_parameter("identifier")));
                        //seconds duration = duration_cast<std::chrono::seconds>(e->m_duration);
                        auto dur_variant = event.get_parameter("duration");
                        auto &&dur       = std::get_if<std::int64_t>(&dur_variant);
                        RandomEventManager()->SetCurrentRandomEvent(
                                *e, peer->m_pos,
                                dur ? std::chrono::seconds(*dur)
                                    : duration_cast<std::chrono::seconds>(e->m_duration));
                        event.reply("Started event in world");
                    } else {
                        event.reply("Event does not exist");
                    }
                } else if (label == "avlkick") {
                    // TODO use get_if to get pointers and not newly allocated strings

                    auto &&identifier = std::get<std::string>(event.get_parameter("identifier"));
                    if (auto peer = NetManager()->Kick(identifier))
                        event.reply("Kicked " + peer->m_name + " (" + peer->m_socket->get_host_name() + ")");
                    else
                        event.reply("Player not found");
                } else if (label == "avllink") {
                    auto key_variant = event.get_parameter("key");
                    auto &&key       = std::get_if<std::string>(&key_variant);
                    if (key) {
                        // Verify the key
                        for (auto &&itr = m_temp_linking_keys.begin(); itr != m_temp_linking_keys.end();) {
                            auto &&host = itr->first;
                            auto &&vkey = itr->second.first;
                            if (vkey == *key) {
                                event.reply("Accounts successfully linked!");
                                //m_bot->interaction_followup_create(event.command.token, dpp::message("Accounts linked! Have fun!"), );
                                m_linked_accounts[host] = event.command.get_issuing_user().id;
                                if (auto &&peer = NetManager()->FindPeerByHost(host)) {
                                    peer->SetGated(false);
                                    peer->CenterMessage("Account verified");
                                }
                                itr = m_temp_linking_keys.erase(itr);
                                return;
                            } else {
                                ++itr;
                            }
                        }

                        //m_bot->interaction_followup_create(event.command.token, dpp::message("Invalid key"), [](const dpp::confirmation_callback_t&) {});
                        event.reply("Invalid key.");
                    } else {
                        event.reply("Join the in-game server and enter the provided key here to link your "
                                    "account");
                    }
                } else if (label == "avllist") {
                    if (NetManager()->GetPeers().empty()) {
                        event.reply("No players are online");
                    } else {
                        std::string msg
                                = std::to_string(NetManager()->GetPeers().size()) + " players are online\n";
                        for (auto &&peer : NetManager()->GetPeers()) {
                            msg += " - " + peer->m_name + "\n";
                        }
                        event.reply(msg);
                    }
                } else if (label == "avlmessage") {
                    auto &&message    = std::get<std::string>(event.get_parameter("message"));
                    auto &&identifier = std::get<std::string>(event.get_parameter("identifier"));
                    if (auto peer = NetManager()->FindPeer(identifier)) {
                        peer->CenterMessage(message);
                        event.reply("Sent message to player");
                    } else
                        event.reply("Player not found");
                } else if (label == "avlpardon") {
                    auto &&host = std::get<std::string>(event.get_parameter("host"));
                    if (Avledet()->m_blacklist.erase(host))
                        event.reply("Unbanned " + host);
                    else
                        event.reply("Player is not banned");
                } else if (label == "avlreload") {
                    Avledet()->LoadFiles(true);
                    event.reply("All files were reloaded");
                } else if (label == "avlsave") {
                    WorldManager()->GetWorld()->WriteFiles();
                    event.reply("Saved the world");
                } else if (label == "avlstop") {
                    Avledet()->Stop();
                    event.reply("Stopping the server!");
                } else if (label == "avlsummon") {
                    auto &&name = std::get<std::string>(event.get_parameter("prefab"));
                    auto &&peer = NetManager()->FindPeer(
                            std::get<std::string>(event.get_parameter("identifier")));
                    if (auto &&prefab = PrefabManager()->find_prefab(name); peer) {
                        ZDOManager()->Instantiate(*prefab, peer->m_pos);
                        event.reply("Object was summoned");
                    } else {
                        event.reply("Either prefab or peer are invalid");
                    }
                } else if (label == "avltime") {
                    event.reply("Server time is "
                                + std::to_string(
                                        duration_cast<std::chrono::seconds>(Avledet()->Elapsed()).count())
                                + "s");
                } else if (label == "avltod") {
                    auto time_variant = event.get_parameter("time");
                    auto &&time       = std::get_if<std::string>(&time_variant);
                    if (time) {
                        char ch = (*time)[0];
                        Avledet()->SetTimeOfDay(ch == 'M'   ? TIME_MORNING
                                                : ch == 'D' ? TIME_DAY
                                                : ch == 'A' ? TIME_AFTERNOON
                                                            : TIME_NIGHT);
                        event.reply("Set world time to " + *time);
                    } else {
                        event.reply(std::string("It is currently ")
                                    + (Avledet()->IsMorning()     ? "morning"
                                       : Avledet()->IsDay()       ? "day"
                                       : Avledet()->IsAfternoon() ? "afternoon"
                                                                  : "night"));
                    }
                } else if (label == "avlwhitelist") {
                    auto flag_variant = event.get_parameter("flag");
                    auto &&flag       = std::get_if<bool>(&flag_variant);
                    if (flag) {
                        AVL_SETTINGS.playerWhitelist = *flag;
                        event.reply(std::string("Whitelist is now ")
                                    + (AVL_SETTINGS.playerWhitelist ? "enabled" : "disabled"));
                    } else {
                        event.reply(std::string("The whitelist is ")
                                    + (AVL_SETTINGS.playerWhitelist ? "enabled" : "disabled"));
                    }
                } else if (label == "avlwhois") {
                    auto &&identifier = std::get<std::string>(event.get_parameter("identifier"));
                    if (auto peer = NetManager()->FindPeer(identifier)) {
                        event.reply("Name: " + peer->m_name + "\n"
                                    + "Uuid: " + std::to_string(peer->GetUserID()) + "\n"
                                    + "Host: " + peer->m_socket->get_host_name() + "\n"
                                    + "Address: " + peer->m_socket->get_address());
                    } else
                        event.reply("Player not found");
                } else if (label == "avlworldtime") {
                    auto time_variant = event.get_parameter("time");
                    auto &&time       = std::get_if<double>(&time_variant);
                    if (time) {
                        Avledet()->SetWorldTime(*time);
                        event.reply("Set world time to " + std::to_string(*time));
                    } else {
                        event.reply("World time is " + std::to_string(Avledet()->GetWorldTime()));
                    }
                }
    #if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
                else if (label == "avlscript" || label == "avlscriptf") {
                    event.thinking(true);

                    std::string code, script_name, chunk_name;
                    if (label == "avlscriptf") {
                        assert(false);
                        // script from file is a bit more complicated I guess...
                        // script
                        //dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("file"));
                        //dpp::attachment const &attachment = event.command.get_resolved_attachment(file_id);
                        //attachment.download([](http_request_completion_t const &http) {
                        //    Avledet()->RunTask([http](Task &) {
                        //        auto idx = attachment.filename.find_last_of('.');
                        //        if (idx != std::string::npos) {
                        //            script_name = attachment.filename.substr(0, idx);
                        //        } else {
                        //            script_name = attachment.filename;
                        //        }
                        //        chunk_name = attachment.filename;
                        //    });
                        //})
                    } else {
                        //auto &&script_var = event.get_parameter("script");
                        //code              = std::get<std::string>(script_var);
                        code = std::get<std::string>(event.get_parameter("script"));

                        auto &&chunk_var = event.get_parameter("chunk");
                        auto &&chunk_ptr = std::get_if<std::string>(&chunk_var);
                        chunk_name       = (!chunk_ptr || chunk_ptr->empty())
                                                   ? avledet::util::generate("abcdefghijklmnopqrstuvwxyz", 12)
                                                   : *chunk_ptr;

                        script_name = chunk_name;//TODO tmp lazy name
                    }

                    auto user_id = event.command.member.user_id.str();

                    LOG_INFO(AVL_LOGGER, "dpp / {}: {}", user_id, code);

                    auto script_info = IScriptManager::ScriptInfo(script_name, chunk_name);
                    script_info.m_authors.push_back(user_id);

                    try {
                        ScriptManager()->execute(script_info, code);
                        m_bot->interaction_followup_create(
                                event.command.token,
                                dpp::message("Script '" + chunk_name + "' was successful"),
                                [](dpp::confirmation_callback_t const &) {});
                    } catch (std::exception const &e) {
                        LOG_INFO(AVL_LOGGER, "Script failure / {}: {}", user_id, e.what());
                        m_bot->interaction_followup_create(
                                event.command.token, dpp::message(std::string("Script error: \n") + e.what()),
                                [](dpp::confirmation_callback_t const &) {});
                    }
                }
    #endif
                else {
                    event.reply("Sorry, command is not implemented");
                }
            });
        }
    });

    m_bot->on_autocomplete([this](dpp::autocomplete_t const &evt) {
        for (auto &opt : evt.options) {
            if (opt.focused) {
                Avledet()->RunTask([this, evt, opt](Task &) {
                    auto &&base    = std::get<std::string>(opt.value);
                    auto irsp      = dpp::interaction_response(dpp::ir_autocomplete_reply);
                    auto &&choices = irsp.autocomplete_choices;

                    // Custom autocomplete reply lambda
                    //	Automatically sorts response based off base string and sends to client
                    auto &&reply = [&]() {
                        if (choices.size() > AUTOCOMPLETE_MAX_CHOICES)
                            choices.resize(AUTOCOMPLETE_MAX_CHOICES);

                        m_bot->interaction_response_create(evt.command.id, evt.command.token, irsp);
                    };

                    auto &&add_choice = [&](std::string_view choice, bool force) -> bool {
                        if (choices.size() < AUTOCOMPLETE_MAX_CHOICES && (choice.contains(base) || force)) {
                            choices.push_back(
                                    dpp::command_option_choice(std::string(choice), std::string(choice)));
                        }

                        return choices.size() < AUTOCOMPLETE_MAX_CHOICES;
                    };

                    //auto&& add_choices = [&](auto&& view) {
                    //	//if (view.size() > AUTOCOMPLETE_MAX_CHOICES) {
                    //	for (auto&& s : view) {
                    //		if (!add_choice(s, view.size() <= AUTOCOMPLETE_MAX_CHOICES))
                    //			break;
                    //	}
                    //	//}
                    //};

                    if (opt.name == "identifier") {
                        bool const has_num = std::any_of(base.begin(), base.end(), ::isdigit);

                        // Populate choices
                        for (auto &&peer : NetManager()->GetPeers()) {
                            auto &&kw = peer->m_name;
                            choices.emplace_back(dpp::command_option_choice(
                                    has_num ? peer->m_socket->get_host_name() : peer->m_name,
                                    peer->m_socket->get_host_name()));
                        }
                    } else if (opt.name == "event") {
                        //add_choices(ranges::views::keys(RandomEventManager()->m_events));
                        for (auto &&e : ranges::views::keys(RandomEventManager()->m_events)) {
                            choices.emplace_back(dpp::command_option_choice(std::string(e), std::string(e)));
                            add_choice(e, false);
                        }
                    } else if (opt.name == "prefab") {
                        //add_choices(ranges::views ranges::views::values(PrefabManager()->m_prefabs));
                        for (auto &&prefab : PrefabManager()->m_prefabs) {
                            if (!add_choice(prefab.m_name, false))
                                break;
                            //choices.emplace_back(dpp::command_option_choice(prefab->m_name, prefab->m_name));
                        }
                    } else {
                        LOG_WARNING(AVL_LOGGER, "autocomplete not registered");
                        return;
                    }

                    reply();
                });
                break;
            }
        }
    });

    m_bot->on_guild_member_remove([this](dpp::guild_member_remove_t const &event) {
        if (AVL_SETTINGS.TEST_discordSyncLeaves) {
            // Try kicking player off Valheim server

            if (auto &&peer = unlink_peer(event.removed.id)) {
                peer->Kick();

                LOG_INFO(AVL_LOGGER, "Kicked {} due to guild leave", peer->m_name);
            }
        }
    });

    m_bot->on_ready([this](dpp::ready_t const &evt) {
        if (dpp::run_once<struct register_bot_commands>()) {
            m_bot->guild_bulk_command_create(
                    {
                            // Async commands
                            dpp::slashcommand("avlfreset", "Delete all commands", m_bot->me.id)
                                    .set_default_permissions(0),

                            // Sync commands
                            dpp::slashcommand("avladmin", "See which players are admin", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "identifier",
                                                                    "name/uuid/host")
                                                        .set_auto_complete(true))
                                    .add_option(dpp::command_option(dpp::co_boolean, "flag",
                                                                    "grant/revoke admin"))
                                    .set_default_permissions(0),// 0 is admins only

                            dpp::slashcommand("avlban", "Ban a player", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "identifier",
                                                                    "name/uuid/host", true)
                                                        .set_auto_complete(true))
                                    .set_default_permissions(
                                            dpp::permissions::p_ban_members),// 0 is admins only

                            dpp::slashcommand("avlbroadcast", "Broadcast a message to all players",
                                              m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "message",
                                                                    "the message to broadcast", true))
                                    .set_default_permissions(
                                            dpp::permissions::p_manage_messages),// 0 is admins only

                            dpp::slashcommand("avlevent", "Set event in world", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "event", "Name of event",
                                                                    true)
                                                        .set_auto_complete(true))
                                    .add_option(dpp::command_option(dpp::co_string, "identifier",
                                                                    "Player to start event nearby", true)
                                                        .set_auto_complete(true))
                                    .add_option(dpp::command_option(dpp::co_integer, "seconds",
                                                                    "Duration in seconds"))
                                    .set_default_permissions(0),// 0 is admins only

                            dpp::slashcommand("avlkick", "Kick a player", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "identifier",
                                                                    "name/uuid/host", true)
                                                        .set_auto_complete(true))
                                    .set_default_permissions(
                                            dpp::permissions::p_kick_members),// 0 is admins only

                            dpp::slashcommand("avllink", "Links your Steam-id to Discord", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "key",
                                                                    "Verification key from server")),

                            dpp::slashcommand("avllist", "List currently online players", m_bot->me.id),

                            dpp::slashcommand("avlmessage", "Message a player", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "identifier",
                                                                    "name/uuid/host", true)
                                                        .set_auto_complete(true))
                                    .add_option(dpp::command_option(dpp::co_string, "message",
                                                                    "the message to send", true))
                                    .set_default_permissions(
                                            dpp::permissions::p_manage_messages),// 0 is admins only

                            dpp::slashcommand("avlpardon", "Unban a player", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "host", "host", true))
                                    .set_default_permissions(
                                            dpp::permissions::p_ban_members),// 0 is admins only

                            dpp::slashcommand("avlreload", "Reload config files from disk", m_bot->me.id)
                                    .set_default_permissions(0),             // 0 is admins only

                            dpp::slashcommand("avlsave", "Save the world", m_bot->me.id)
                                    .set_default_permissions(0),             // 0 is admins only

                            dpp::slashcommand("avlstop", "Shutdown the server", m_bot->me.id)
                                    .set_default_permissions(0),             // 0 is admins only

                            dpp::slashcommand("avlsummon", "Spawns an object into the world", m_bot->me.id)
                                    .add_option(
                                            dpp::command_option(dpp::co_string, "prefab", "prefab name", true)
                                                    .set_auto_complete(true))
                                    .add_option(dpp::command_option(dpp::co_string, "identifier",
                                                                    "name/uuid/host of player to spawn at",
                                                                    true)
                                                        .set_auto_complete(true))
                                    .set_default_permissions(0),// 0 is admins only

                            dpp::slashcommand("avltime", "Get the server time", m_bot->me.id),

                            dpp::slashcommand("avltod", "Get or set time of day", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "time", "time of day")
                                                        .add_choice(dpp::command_option_choice(
                                                                "Morning", std::string("Morning")))
                                                        .add_choice(dpp::command_option_choice(
                                                                "Day", std::string("Day")))
                                                        .add_choice(dpp::command_option_choice(
                                                                "Afternoon", std::string("Afternoon")))
                                                        .add_choice(dpp::command_option_choice(
                                                                "Night", std::string("Night"))))
                                    .set_default_permissions(0),// 0 is admins only

                            dpp::slashcommand("avlwhitelist", "Whitelist information", m_bot->me.id)
                                    .add_option(
                                            dpp::command_option(dpp::co_boolean, "flag", "enable/disable"))
                                    .set_default_permissions(
                                            dpp::permissions::p_ban_members),// 0 is admins only

                            dpp::slashcommand("avlwhois", "Get player information", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "identifier",
                                                                    "name/uuid/host", true)
                                                        .set_auto_complete(true))
                                    .set_default_permissions(
                                            dpp::permissions::p_kick_members),// 0 is admins only

                            dpp::slashcommand("avlworldtime", "Get or set world time", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_number, "time", "world time"))
                                    .set_default_permissions(0),// 0 is admins only

                                                                // Together
    #if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
                            dpp::slashcommand("avlscript", "Run a Lua string", m_bot->me.id)
                                    .add_option(dpp::command_option(dpp::co_string, "script", "code", true))
                                    .add_option(dpp::command_option(dpp::co_string, "chunk",
                                                                    "lua descriptive name", false))
                                    .set_default_permissions(0),// 0 is admins only

                                                                // TODO complicated...
                //dpp::slashcommand("avlscriptf", "Run a Lua script from file", m_bot->me.id)
                //        .add_option(
                //                dpp::command_option(dpp::co_attachment, "file", "lua file", true))
                //        .add_option(dpp::command_option(dpp::co_string, "chunk",
                //                                        "lua descriptive name", false))
                //        .set_default_permissions(0)// 0 is admins only
    #endif
                    },
                    AVL_SETTINGS.discordGuild);
        }
    });

    m_bot->start(dpp::st_return);
}

void IDiscordManager::period_update()
{
    // If integration is off
    if (!m_bot)
        return;

    for (auto &&itr = m_temp_linking_keys.begin(); itr != m_temp_linking_keys.end();) {
        auto &&peer  = NetManager()->FindPeerByHost(itr->first);
        auto &&since = Avledet()->Nanos() - itr->second.second;
        if (since > 5min) {
            LOG_INFO(AVL_LOGGER, "Discord linking key expired for {}", itr->first);

            // Kick the user to generate a new key
            if (peer) {
                peer->CenterMessage("<color=#FF5555>Verification timed out!</color>");
                peer->Kick();
            }
            itr = m_temp_linking_keys.erase(itr);
        } else {
            if (peer) {
                //peer->CenterMessage(std::string("Verification required: <color=#FF1111>") + pair.second + "</color>");
                peer->CenterMessage(
                        "Verification required: <color=#FF1111>" + itr->second.first + "</color> ("
                        + std::to_string(duration_cast<std::chrono::seconds>(5min - since).count()) + "s)");
            }
            ++itr;
        }
    }

    /*
	for (auto&& pair : m_tempLinkingKeys) {
		auto&& peer = NetManager()->FindPeerByHost(pair.first);
		if (peer) {
			//peer->CenterMessage(std::string("Verification required: <color=#FF1111>") + pair.second + "</color>");
			peer->CenterMessage("Verification required: <color=#FF1111>" + pair.second.first + "</color>");
		}
	}*/

    /*
	if (AVL_SETTINGS.discordKickOnLeave) {
		// if a peer has left the discord server and linked players are required, then set gated or kick
		for (auto&& peer : NetManager()->GetPeers()) {
			
		}
	}*/
}

Peer *IDiscordManager::find_peer(dpp::snowflake id)
{
    for (auto &&pair : m_linked_accounts) {
        if (pair.second == id) {
            return NetManager()->FindPeerByHost(pair.first);
        }
    }
    return nullptr;
}

Peer *IDiscordManager::unlink_peer(dpp::snowflake id)
{
    for (auto &&itr = m_linked_accounts.begin(); itr != m_linked_accounts.end();) {
        if (itr->second == id) {
            auto &&peer = NetManager()->FindPeerByHost(itr->first);
            m_linked_accounts.erase(itr);
            return peer;
        } else {
            ++itr;
        }
    }
    return nullptr;
}

void IDiscordManager::send_webhook_message(std::string_view msg)
{
    if (!m_bot || AVL_SETTINGS.discordWebhook.empty())
        return;

    auto &&webhook = dpp::webhook(AVL_SETTINGS.discordWebhook);

    m_bot->execute_webhook(webhook, dpp::message(std::string(msg)));
}
#endif// AVL_DISCORD_INTEGRATION