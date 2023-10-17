#include "DiscordManager.h"

#if VH_IS_ON(VH_DISCORD_INTEGRATION)

#include <isteamgameserver.h>
#include <dpp/dpp.h>
#include <dpp/dispatcher.h>
#include <range/v3/all.hpp>

#include "ValhallaServer.h"
#include "NetManager.h"
#include "Peer.h"
#include "RandomEventManager.h"
#include "ZDOManager.h"

auto DISCORD_MANAGER(std::make_unique<IDiscordManager>());
IDiscordManager* DiscordManager() {
	return DISCORD_MANAGER.get();
}


/*
class DiscordClientInterceptor : public dpp::discord_client {
public:

};*/

void IDiscordManager::Init() {
	if (VH_SETTINGS.discordToken.empty())
		return;

	// https://dpp.dev/slashcommands.html

	m_bot = std::make_unique<dpp::cluster>(VH_SETTINGS.discordToken);
		
	m_bot->on_log([](const dpp::log_t& log) {
		switch (log.severity) {
		case dpp::loglevel::ll_trace: LOG_TRACE_L1(LOGGER, "{}", log.message); break;
		case dpp::loglevel::ll_debug: LOG_DEBUG(LOGGER, "{}", log.message); break;
		case dpp::loglevel::ll_info: LOG_INFO(LOGGER, "{}", log.message); break;
		case dpp::loglevel::ll_warning: LOG_WARNING(LOGGER, "{}", log.message); break;
		case dpp::loglevel::ll_error: LOG_ERROR(LOGGER, "{}", log.message); break;
		case dpp::loglevel::ll_critical: LOG_CRITICAL(LOGGER, "{}", log.message); break;
		}
	});

	m_bot->on_slashcommand([this](const dpp::slashcommand_t& event) {
		//event.thinking(true);
		
		auto&& label = event.command.get_command_name();

		if (label == "vhfreset") {
			event.reply("Deleted all commands");
			auto&& commands = m_bot->global_commands_get_sync();
			for (auto&& command : commands) {
				if (command.second.name == "vhfreset"
					|| command.second.name == "vhfreg")
					continue;
				m_bot->global_command_delete_sync(command.first);
			}
		}
		else {
			Valhalla()->RunTask([=](Task&) {
				if (label == "vhadmin") {
					auto&& admin = Valhalla()->m_admin;

					auto&& identifier = std::get_if<std::string>(&event.get_parameter("identifier"));
					auto&& flag = std::get_if<bool>(&event.get_parameter("flag"));
					if (identifier) {
						if (auto&& peer = NetManager()->GetPeer(*identifier)) {
							if (flag) {
								peer->SetAdmin(*flag);
								if (*flag)
									event.reply("Granted admin to player");
								else
									event.reply("Revoked admin from player");
							}
							else {
								event.reply(std::string("Player is ") + (peer->IsAdmin() ? "" : "not ") + "an admin");
							}
						}
						else {
							event.reply("Player not found");
						}

						/*
						if (flag) {
							if (*flag) {
								admin.insert(identifier)
							}
						}*/

					}
					else {
						// reply with a list of all admins
						if (admin.empty()) {
							event.reply("There are no players with admin privileges");
						}
						else {
							std::string msg = "Players with admin: \n";
							for (auto&& host : admin) {
								msg += " - " + host + "\n";
							}
							event.reply(msg);
						}
					}
				}
				else if (label == "vhban") {
					auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
					if (auto peer = NetManager()->Ban(identifier))
						event.reply("Banned " + peer->m_name + " (" + peer->m_socket->GetHostName() + ")");
					else
						event.reply("Player not found");
				}
				else if (label == "vhbroadcast") {
					auto&& message = std::get<std::string>(event.get_parameter("message"));
					Valhalla()->Broadcast(UIMsgType::Center, message);
					event.reply("Broadcasted message to all players");
				}
				else if (label == "vhevent") {
					if (auto&& e = RandomEventManager()->GetEvent(std::get<std::string>(event.get_parameter("event")))) {
						auto&& peer = NetManager()->GetPeer(std::get<std::string>(event.get_parameter("identifier")));
						//seconds duration = duration_cast<seconds>(e->m_duration);
						auto&& dur = std::get_if<int64_t>(&event.get_parameter("duration"));
						RandomEventManager()->SetCurrentRandomEvent(*e, peer->m_pos,
							dur ? seconds(*dur) : duration_cast<seconds>(e->m_duration));
						event.reply("Started event in world");
					}
					else {
						event.reply("Event does not exist");
					}
				}
				else if (label == "vhkick") {
					// TODO use get_if to get pointers and not newly allocated strings

					auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
					if (auto peer = NetManager()->Kick(identifier))
						event.reply("Kicked " + peer->m_name + " (" + peer->m_socket->GetHostName() + ")");
					else
						event.reply("Player not found");
				}
				else if (label == "vhlink") {
					auto&& key = std::get_if<std::string>(&event.get_parameter("key"));
					if (key) {
						// Verify the key
						for (auto&& itr = m_tempLinkingKeys.begin(); itr != m_tempLinkingKeys.end(); ) {
							auto&& host = itr->first;
							auto&& vkey = itr->second.first;
							if (vkey == *key) {
								event.reply("Accounts successfully linked!");
								//m_bot->interaction_followup_create(event.command.token, dpp::message("Accounts linked! Have fun!"), );
								m_linkedAccounts[host] = event.command.get_issuing_user().id;
								if (auto&& peer = NetManager()->GetPeerByHost(host)) {
									peer->SetGated(false);
									peer->CenterMessage("Account verified");
								}
								itr = m_tempLinkingKeys.erase(itr);
								return;
							}
							else {
								++itr;
							}
						}

						//m_bot->interaction_followup_create(event.command.token, dpp::message("Invalid key"), [](const dpp::confirmation_callback_t&) {});
						event.reply("Invalid key.");
					}
					else {
						event.reply("Join the in-game server and enter the provided key here to link your account");
					}
				}
				else if (label == "vhlist") {
					if (NetManager()->GetPeers().empty()) {
						event.reply("No players are online");
					}
					else {
						std::string msg = std::to_string(NetManager()->GetPeers().size()) + " players are online\n";
						for (auto&& peer : NetManager()->GetPeers()) {
							msg += " - " + peer->m_name + "\n";
						}
						event.reply(msg);
					}
				}
				else if (label == "vhmessage") {
					auto&& message = std::get<std::string>(event.get_parameter("message"));
					auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
					if (auto peer = NetManager()->GetPeer(identifier)) {
						peer->CenterMessage(message);
						event.reply("Sent message to player");
					}
					else
						event.reply("Player not found");
				}
				else if (label == "vhpardon") {
					auto&& host = std::get<std::string>(event.get_parameter("host"));
					if (Valhalla()->m_blacklist.erase(host))
						event.reply("Unbanned " + host);
					else
						event.reply("Player is not banned");
				}
				else if (label == "reload") {
					Valhalla()->LoadFiles(true);
					event.reply("All files were reloaded");
				}
				else if (label == "vhsave") {
					WorldManager()->GetWorld()->WriteFiles();
					event.reply("Saved the world");
				}
				else if (label == "vhstop") {
					Valhalla()->Stop();
					event.reply("Stopping the server!");
				}
				else if (label == "vhsummon") {
					auto&& name = std::get<std::string>(event.get_parameter("prefab"));
					auto&& peer = NetManager()->GetPeer(std::get<std::string>(event.get_parameter("identifier")));
					if (auto&& prefab = PrefabManager()->GetPrefab(name); peer) {
						ZDOManager()->Instantiate(*prefab, peer->m_pos);
						event.reply("Object was summoned");
					}
					else {
						event.reply("Either prefab or peer are invalid");
					}
				}
				else if (label == "vhtime") {
					event.reply("Server time is "
						+ std::to_string(duration_cast<seconds>(Valhalla()->Elapsed()).count()) + "s");
				}
				else if (label == "vhtod") {
					auto&& time = std::get_if<std::string>(&event.get_parameter("time"));
					if (time) {
						char ch = (*time)[0];
						Valhalla()->SetTimeOfDay(ch == 'M' ? TIME_MORNING : ch == 'D' ? TIME_DAY : ch == 'A' ? TIME_AFTERNOON : TIME_NIGHT);
						event.reply("Set world time to " + *time);
					}
					else {
						event.reply(std::string("It is currently ") 
							+ (Valhalla()->IsMorning() ? "morning" : Valhalla()->IsDay() ? "day" : Valhalla()->IsAfternoon() ? "afternoon" : "night"));
					}
				}
				else if (label == "vhwhitelist") {
					auto&& flag = std::get_if<bool>(&event.get_parameter("flag"));
					if (flag) {
						VH_SETTINGS.playerWhitelist = *flag;
						event.reply(std::string("Whitelist is now ") + (VH_SETTINGS.playerWhitelist ? "enabled" : "disabled"));
					}
					else {
						event.reply(std::string("The whitelist is ") + (VH_SETTINGS.playerWhitelist ? "enabled" : "disabled"));
					}
				}
				else if (label == "vhwhois") {
					auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
					if (auto peer = NetManager()->GetPeer(identifier)) {
						event.reply("Name: " + peer->m_name + "\n"
							+ "Uuid: " + std::to_string(peer->GetUserID()) + "\n"
							+ "Host: " + peer->m_socket->GetHostName() + "\n"
							+ "Address: " + peer->m_socket->GetAddress());
					}
					else
						event.reply("Player not found");
				}
				else if (label == "vhworldtime") {
					auto&& time = std::get_if<double>(&event.get_parameter("time"));
					if (time) {
						Valhalla()->SetWorldTime(*time);
						event.reply("Set world time to " + std::to_string(*time));
					}
					else {
						event.reply("World time is " + std::to_string(Valhalla()->GetWorldTime()));
					}
				}
#if VH_IS_ON(VH_USE_MODS)
				else if (label == "vhlua") {
					event.thinking(true);
					auto&& script = std::get<std::string>(event.get_parameter("script"));
					try {
						ModManager()->m_state.safe_script(script);
						m_bot->interaction_followup_create(event.command.token, 
							std::string("Script success"), 
							[](const dpp::confirmation_callback_t&) {});
					}
					catch (const std::exception& e) {
						//event.reply(std::string("Script failed to run: \n") + e.what());
						m_bot->interaction_followup_create(event.command.token, 
							std::string("Script error: \n") + e.what(), 
							[](const dpp::confirmation_callback_t&) {});
					}
				}
				else if (label == "vhscript") {
					event.thinking(true);
					auto&& script = std::get<std::string>(event.get_parameter("script"));
					try {
						ModManager()->m_state.safe_script(script);
						m_bot->interaction_followup_create(event.command.token,
							std::string("Script success"),
							[](const dpp::confirmation_callback_t&) {});
					}
					catch (const std::exception& e) {
						//event.reply(std::string("Script failed to run: \n") + e.what());
						m_bot->interaction_followup_create(event.command.token,
							std::string("Script error: \n") + e.what(),
							[](const dpp::confirmation_callback_t&) {});
					}
				}
#endif
				else {
					event.reply("Sorry, command is not implemented");
				}
			});
		}
	});
	
	m_bot->on_autocomplete([this](const dpp::autocomplete_t& evt) {
		for (auto& opt : evt.options) {
			if (opt.focused) {
				Valhalla()->RunTask([=](Task&) {
					auto&& base = std::get<std::string>(opt.value);
					auto irsp = dpp::interaction_response(dpp::ir_autocomplete_reply);
					auto&& choices = irsp.autocomplete_choices;

					// Custom autocomplete reply lambda
					//	Automatically sorts response based off base string and sends to client
					auto&& reply = [&]() {
						if (choices.size() > AUTOCOMPLETE_MAX_CHOICES)
							choices.resize(AUTOCOMPLETE_MAX_CHOICES);

						m_bot->interaction_response_create(evt.command.id, evt.command.token, irsp);
					};

					auto&& add_choice = [&](std::string_view choice, bool force) -> bool {
						if (choices.size() < AUTOCOMPLETE_MAX_CHOICES && (choice.contains(base) || force)) {
							choices.push_back(dpp::command_option_choice(std::string(choice), std::string(choice)));
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
						const bool has_num = std::any_of(base.begin(), base.end(), ::isdigit);

						// Populate choices
						for (auto&& peer : NetManager()->GetPeers()) {
							auto&& kw = peer->m_name;
							choices.emplace_back(dpp::command_option_choice(
								has_num ? peer->m_socket->GetHostName() : peer->m_name,
								peer->m_socket->GetHostName()
							));
						}
					}
					else if (opt.name == "event") {
						//add_choices(ranges::views::keys(RandomEventManager()->m_events));
						for (auto&& e : ranges::views::keys(RandomEventManager()->m_events)) {
							choices.emplace_back(dpp::command_option_choice(std::string(e), std::string(e)));
							add_choice(e, false);
						}
					}
					else if (opt.name == "prefab") {
						//add_choices(ranges::views ranges::views::values(PrefabManager()->m_prefabs));
						for (auto&& prefab : PrefabManager()->m_prefabs) {
							if (!add_choice(prefab.m_name, false))
								break;
							//choices.emplace_back(dpp::command_option_choice(prefab->m_name, prefab->m_name));
						}
					}
					else {
						LOG_WARNING(LOGGER, "autocomplete not registered");
						return;
					}

					reply();
				});
				break;
			}
		}
	});
	
	m_bot->on_guild_member_remove([this](const dpp::guild_member_remove_t& event) {
		if (VH_SETTINGS.discordSyncLeaves) {
			// Try kicking player off Valheim server

			if (auto&& peer = UnlinkPeerBySnowflake(event.removed->id)) {
				peer->Kick();

				LOG_INFO(LOGGER, "Kicked {} due to guild leave", peer->m_name);
			}
		}
	});

	m_bot->on_ready([this](const dpp::ready_t& evt) {
		if (dpp::run_once<struct register_bot_commands>()) {
			m_bot->guild_bulk_command_create({
				// Async commands
				dpp::slashcommand("vhfreset", "Delete all commands", m_bot->me.id)
					.set_default_permissions(0),

				// Sync commands
				dpp::slashcommand("vhadmin", "See which players are admin", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host").set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_boolean, "flag", "grant/revoke admin"))
					.set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhban", "Ban a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(dpp::permissions::p_ban_members), // 0 is admins only

				dpp::slashcommand("vhbroadcast", "Broadcast a message to all players", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "message", "the message to broadcast", true))
					.set_default_permissions(dpp::permissions::p_manage_messages), // 0 is admins only

				dpp::slashcommand("vhevent", "Set event in world", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "event", "Name of event", true).set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_string, "identifier", "Player to start event nearby", true).set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_integer, "seconds", "Duration in seconds"))
					.set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhkick", "Kick a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(dpp::permissions::p_kick_members), // 0 is admins only
								
				dpp::slashcommand("vhlink", "Links your Steam-id to Discord", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "key", "Verification key from server")),

				dpp::slashcommand("vhlist", "List currently online players", m_bot->me.id),

				dpp::slashcommand("vhmessage", "Message a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_string, "message", "the message to send", true))
					.set_default_permissions(dpp::permissions::p_manage_messages), // 0 is admins only

				dpp::slashcommand("vhpardon", "Unban a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "host", "host", true))
					.set_default_permissions(dpp::permissions::p_ban_members), // 0 is admins only
				
				dpp::slashcommand("vhreload", "Reload config files from disk", m_bot->me.id)
					.set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhsave", "Save the world", m_bot->me.id)
					.set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhstop", "Shutdown the server", m_bot->me.id)
					.set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhsummon", "Spawns an object into the world", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "prefab", "prefab name", true).set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host of player to spawn at", true).set_auto_complete(true))
					.set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhtime", "Get the server time", m_bot->me.id),

				dpp::slashcommand("vhtod", "Get or set time of day", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "time", "time of day")
						.add_choice(dpp::command_option_choice("Morning", std::string("Morning")))
						.add_choice(dpp::command_option_choice("Day", std::string("Day")))
						.add_choice(dpp::command_option_choice("Afternoon", std::string("Afternoon")))
						.add_choice(dpp::command_option_choice("Night", std::string("Night")))
					).set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhwhitelist", "Whitelist information", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_boolean, "flag", "enable/disable"))
					.set_default_permissions(dpp::permissions::p_ban_members), // 0 is admins only

				dpp::slashcommand("vhwhois", "Get player information", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(dpp::permissions::p_kick_members), // 0 is admins only

				dpp::slashcommand("vhworldtime", "Get or set world time", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_number, "time", "world time"))
					.set_default_permissions(0), // 0 is admins only

				// Together
#if VH_IS_ON(VH_USE_MODS)
				dpp::slashcommand("vhlua", "Run a Lua script from string", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "script", "script text", true))
					.set_default_permissions(0), // 0 is admins only

				dpp::slashcommand("vhscript", "Run a Lua script from file", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_attachment, "script", "lua file", true))
					.set_default_permissions(0) // 0 is admins only
#endif
			}, VH_SETTINGS.discordGuild);
		}
	});

	m_bot->start(dpp::st_return);
}

void IDiscordManager::PeriodUpdate() {
	for (auto&& itr = m_tempLinkingKeys.begin(); itr != m_tempLinkingKeys.end();) {
		auto&& peer = NetManager()->GetPeerByHost(itr->first);
		auto&& since = Valhalla()->Nanos() - itr->second.second;
		if (since > 5min) {
			LOG_INFO(LOGGER, "Discord linking key expired for {}", itr->first);

			// Kick the user to generate a new key
			if (peer) {
				peer->CenterMessage("<color=#FF5555>Verification timed out!</color>");
				peer->Kick();
			}
			itr = m_tempLinkingKeys.erase(itr);
		}
		else {
			if (peer) {
				//peer->CenterMessage(std::string("Verification required: <color=#FF1111>") + pair.second + "</color>");
				peer->CenterMessage("Verification required: <color=#FF1111>" + itr->second.first + "</color> (" 
					+ std::to_string(duration_cast<seconds>(5min - since).count()) 
					+ "s)"
				);
			}
			++itr;
		}
	}

	/*
	for (auto&& pair : m_tempLinkingKeys) {
		auto&& peer = NetManager()->GetPeerByHost(pair.first);
		if (peer) {
			//peer->CenterMessage(std::string("Verification required: <color=#FF1111>") + pair.second + "</color>");
			peer->CenterMessage("Verification required: <color=#FF1111>" + pair.second.first + "</color>");
		}
	}*/

	/*
	if (VH_SETTINGS.discordKickOnLeave) {
		// if a peer has left the discord server and linked players are required, then set gated or kick
		for (auto&& peer : NetManager()->GetPeers()) {
			
		}
	}*/
}

Peer* IDiscordManager::GetPeerBySnowflake(dpp::snowflake id) {
	for (auto&& pair : m_linkedAccounts) {
		if (pair.second == id) {
			return NetManager()->GetPeerByHost(pair.first);
		}
	}
	return nullptr;
}

Peer* IDiscordManager::UnlinkPeerBySnowflake(dpp::snowflake id) {
	for (auto&& itr = m_linkedAccounts.begin(); itr != m_linkedAccounts.end();) {
		if (itr->second == id) {
			auto&& peer = NetManager()->GetPeerByHost(itr->first);
			m_linkedAccounts.erase(itr);
			return peer;
		}
		else {
			++itr;
		}
	}
	return nullptr;
}

void IDiscordManager::SendSimpleMessage(std::string_view msg) {
	if (VH_SETTINGS.discordWebhook.empty())
		return;

	auto&& webhook = dpp::webhook(VH_SETTINGS.discordWebhook);

	m_bot->execute_webhook(webhook, dpp::message(std::string(msg)));
}
#endif // VH_DISCORD_INTEGRATION