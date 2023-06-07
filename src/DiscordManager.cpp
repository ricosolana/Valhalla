#include "DiscordManager.h"

#if VH_IS_ON(VH_DISCORD_INTEGRATION)

#include <isteamgameserver.h>
#include <dpp/dpp.h>
#include <dpp/dispatcher.h>

#include "ValhallaServer.h"
#include "NetManager.h"
#include "Peer.h"

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
				if (label == "vhkick") {
					// TODO use get_if to get pointers and not newly allocated strings

					auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
					if (auto peer = NetManager()->Kick(identifier))
						event.reply("Kicked " + peer->m_name + " (" + peer->m_socket->GetHostName() + ")");
					else
						event.reply("Player not found");
				}
				else if (label == "vhban") {
					auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
					if (auto peer = NetManager()->Ban(identifier))
						event.reply("Banned " + peer->m_name + " (" + peer->m_socket->GetHostName() + ")");
					else
						event.reply("Player not found");
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
				else if (label == "vhpardon") {
					auto&& host = std::get<std::string>(event.get_parameter("host"));
					if (Valhalla()->m_blacklist.erase(host))
						event.reply("Unbanned " + host);
					else
						event.reply("Player is not banned");
				}
				else if (label == "vhsave") {
					WorldManager()->GetWorld()->WriteFiles();
					event.reply("Saved the world");
				}
				else if (label == "vhstop") {
					Valhalla()->Stop();
					event.reply("Stopping the server!");
				}
				else if (label == "vhbroadcast") {
					auto&& message = std::get<std::string>(event.get_parameter("message"));
					Valhalla()->Broadcast(UIMsgType::Center, message);
					event.reply("Broadcasted message to all players");
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
				else if (label == "vhwhois") {
					auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
					if (auto peer = NetManager()->GetPeer(identifier)) {
						event.reply("Name: " + peer->m_name + "\n"
							+ "Uuid: " + std::to_string(peer->m_characterID.GetOwner()) + "\n"
							+ "Host: " + peer->m_socket->GetHostName() + "\n"
							+ "Address: " + peer->m_socket->GetAddress());
					}
					else
						event.reply("Player not found");
				}
				else if (label == "vhtime") {
					event.reply("Server time is "
						+ std::to_string(duration_cast<seconds>(Valhalla()->Elapsed()).count()) + "s");
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
#ifdef VH_OPTION_ENABLE_MODS
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
				else if (label == "vhlink") {
					auto&& key = std::get_if<std::string>(&event.get_parameter("key"));
					if (key) {
						// Verify the key
						for (auto&& itr = m_tempLinkingKeys.begin(); itr != m_tempLinkingKeys.end(); ) {
							auto&& host = itr->first;
							auto&& vkey = itr->second;
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
				else if (label == "vhadmin") {
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
					if (opt.name == "identifier") {
						auto&& val = std::get<std::string>(opt.value);
						// responses to be alphabetical
						auto&& response = dpp::interaction_response(dpp::ir_autocomplete_reply);

						auto sorted = std::vector<std::pair<Peer*, int>>();
						for (auto&& peer : NetManager()->GetPeers()) {
							sorted.push_back({ peer, VUtils::String::LevenshteinDistance(peer->m_name, val) });
						}

						std::sort(sorted.begin(), sorted.end(), [](const std::pair<Peer*, int>& a, const std::pair<Peer*, int>& b) {
							return a.second < b.second;
						});

						for (auto&& pair : sorted) {
							auto&& peer = pair.first;
							response.add_autocomplete_choice(dpp::command_option_choice(peer->m_name, peer->m_socket->GetHostName()));
						}

						m_bot->interaction_response_create(evt.command.id, evt.command.token, response);
					}

				});
				break;
			}
		}
	});

	m_bot->on_ready([this](const dpp::ready_t& evt) {
		if (dpp::run_once<struct register_bot_commands>()) {
			m_bot->guild_bulk_command_create({
				dpp::slashcommand("vhfreset", "Delete all commands", m_bot->me.id)
					.set_default_permissions(0),
				dpp::slashcommand("vhkick", "Kick a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(dpp::permissions::p_kick_members), // 0 is admins only
				dpp::slashcommand("vhban", "Ban a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(dpp::permissions::p_ban_members), // 0 is admins only
				dpp::slashcommand("vhwhitelist", "Whitelist information", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_boolean, "flag", "enable/disable"))
					.set_default_permissions(dpp::permissions::p_ban_members), // 0 is admins only
				dpp::slashcommand("vhpardon", "Unban a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "host", "host", true))
					.set_default_permissions(dpp::permissions::p_ban_members), // 0 is admins only
				dpp::slashcommand("vhsave", "Save the world", m_bot->me.id)
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhstop", "Shutdown the server", m_bot->me.id)
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhbroadcast", "Broadcast a message to all players", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "message", "the message to broadcast", true))
					.set_default_permissions(dpp::permissions::p_manage_messages), // 0 is admins only
				dpp::slashcommand("vhmessage", "Message a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_string, "message", "the message to send", true))
					.set_default_permissions(dpp::permissions::p_manage_messages), // 0 is admins only
				dpp::slashcommand("vhwhois", "Get player information", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(dpp::permissions::p_kick_members), // 0 is admins only
				dpp::slashcommand("vhtime", "Get the server time", m_bot->me.id),
				dpp::slashcommand("vhworldtime", "Get or set world time", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_number, "time", "world time"))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhtod", "Get or set time of day", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "time", "time of day")
						.add_choice(dpp::command_option_choice("Morning", std::string("Morning")))
						.add_choice(dpp::command_option_choice("Day", std::string("Day")))
						.add_choice(dpp::command_option_choice("Afternoon", std::string("Afternoon")))
						.add_choice(dpp::command_option_choice("Night", std::string("Night")))
					).set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhlua", "Run a Lua script from string", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "script", "script text", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhscript", "Run a Lua script from file", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_attachment, "script", "lua file", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhadmin", "See which players are admin", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host").set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_boolean, "flag", "grant/revoke admin"))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhlist", "List currently online players", m_bot->me.id),
				dpp::slashcommand("vhlink", "Links your Steam-id to Discord", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "key", "Verification key from server"))
			}, VH_SETTINGS.discordGuild);
		}
	});

	m_bot->start(dpp::st_return);
}

void IDiscordManager::PeriodUpdate() {
	for (auto&& pair : m_tempLinkingKeys) {
		auto&& peer = NetManager()->GetPeerByHost(pair.first);
		if (peer) {
			//peer->CenterMessage(std::string("Verification required: <color=#FF1111>") + pair.second + "</color>");
			peer->CenterMessage("Verification required: <color=#FF1111>" + pair.second + "</color>");
		}
	}
}

void IDiscordManager::SendSimpleMessage(std::string_view msg) {
	if (VH_SETTINGS.discordWebhook.empty())
		return;

	auto&& webhook = dpp::webhook(VH_SETTINGS.discordWebhook);

	m_bot->execute_webhook(webhook, dpp::message(std::string(msg)));
}
#endif // VH_DISCORD_INTEGRATION