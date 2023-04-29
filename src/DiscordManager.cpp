#include <isteamgameserver.h>
#include <dpp/dpp.h>
#include <dpp/dispatcher.h>

#include "DiscordManager.h"
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
	if (VH_SETTINGS.discordToken.empty() || VH_SETTINGS.discordGuild == 0)
		return;

	// https://dpp.dev/slashcommands.html
	//dpp::cluster bot(VH_SETTINGS.discordToken);

	//m_bot->get_shards().begin()->second->
	m_bot = std::make_unique<dpp::cluster>(VH_SETTINGS.discordToken);
	
	m_bot->on_log([](const dpp::log_t& log) {
		switch (log.severity) {
		case dpp::loglevel::ll_trace: LOG(TRACE) << log.message; break;
		case dpp::loglevel::ll_debug: LOG(DEBUG) << log.message; break;
		case dpp::loglevel::ll_info: LOG(INFO) << log.message; break;
		case dpp::loglevel::ll_warning: LOG(WARNING) << log.message; break;
		case dpp::loglevel::ll_error: // fallthrough
		case dpp::loglevel::ll_critical: LOG(ERROR) << log.message; break;
		}
	});

	m_bot->on_slashcommand([](const dpp::slashcommand_t& event) {
		Valhalla()->RunTask([=](Task&) {
			auto&& label = event.command.get_command_name();

			if (label == "vhkick") {
				auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
				if (auto peer = NetManager()->Kick(identifier))
					event.reply("Kicked " + peer->m_name + "(" + peer->m_socket->GetHostName() + ")");
				else 
					event.reply("Player not found");
			}
			else if (label == "vhban") {
				auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
				if (auto peer = NetManager()->Ban(identifier))
					event.reply("Banned " + peer->m_name + "(" + peer->m_socket->GetHostName() + ")");
				else
					event.reply("Player not found");
			}
			else if (label == "vhwhitelist") {
				VH_SETTINGS.playerWhitelist = std::get<bool>(event.get_parameter("flag"));
				event.reply(std::string("Whitelist ") + (VH_SETTINGS.playerWhitelist ? "enabled" : "disabled"));
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
				auto&& identifier = std::get<std::string>(event.get_parameter("message"));
				if (auto peer = NetManager()->GetPeer(identifier)) {
					peer->CenterMessage(message);
					event.reply("Sent message to player");
				} else 
					event.reply("Player not found");
			}
			else if (label == "vhwhois") {
				auto&& identifier = std::get<std::string>(event.get_parameter("identifier"));
				if (auto peer = NetManager()->GetPeer(identifier)) {
					event.reply("Name: " + peer->m_name + "\n"
							+ "Uuid: " + std::to_string(peer->m_uuid) + "\n"
							+ "Host: " + peer->m_socket->GetHostName() + "\n"
							+ "Address" + peer->m_socket->GetAddress());
				}
				else
					event.reply("Player not found");
			}
			else if (label == "vhtime") {
				//auto&& time = 
			}
		});
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

				//LOG(INFO) << "Option name: " << opt.name << ", sub-option name: " << (opt.options.empty() ? "NONE" : opt.options.front().name);

				//m_bot->log(dpp::ll_debug, "Autocomplete " + opt.name + " with value '" + val + "' in field " + evt.name);
				break;
			}
		}
	});

	//m_bot->on_guild_member_remove([](const dpp::guild_member_remove_t& event) {

	//});

	m_bot->on_ready([this](const dpp::ready_t& evt) {
		if (dpp::run_once<struct register_bot_commands>()) {

			m_bot->guild_bulk_command_create({
				dpp::slashcommand("vhkick", "Kick a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhban", "Ban a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhwhitelist", "Enable/disable whitelist", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_boolean, "flag", "enable/disable", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhpardon", "Unban a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "host", "host", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhsave", "Save the world", m_bot->me.id)
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhstop", "Shutdown the server", m_bot->me.id)
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhbroadcast", "Broadcast a message to all players", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "message", "the message to broadcast", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhmessage", "Message a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.add_option(dpp::command_option(dpp::co_string, "message", "the message to send", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhwhois", "Get player information", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "identifier", "name/uuid/host", true).set_auto_complete(true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhtime", "Get or set server time", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_number, "time", "server time"))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhworldtime", "Get or set world time", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_number, "identifier", "world time"))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhtod", "Get or set time of day", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "time", "time of day")
						.add_choice(dpp::command_option_choice("Morning", std::string("Morning")))
						.add_choice(dpp::command_option_choice("Day", std::string("Day")))
						.add_choice(dpp::command_option_choice("Afternoon", std::string("Afternoon")))
						.add_choice(dpp::command_option_choice("Night", std::string("Night")))
					).set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhlua", "Run a Lua sample", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "script", "script text", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhscript", "Run a Lua script file", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_attachment, "script", "lua file", true))
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhlist", "List currently online players", m_bot->me.id)
			}, VH_SETTINGS.discordGuild);

			//dpp::slashcommand cmd("vh", "The Valhalla server command", m_bot->me.id);

			/*
			newcommand.add_option(
				dpp::command_option(dpp::co_string, "save", "Saves the world")
					//add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
					//add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
					//add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin")))
			);*/

			/*
			cmd.add_option(dpp::command_option(dpp::co_sub_command, "save", "Saves the world"))
				.add_option(dpp::command_option(dpp::co_sub_command, "list", "Lists all online players"))
				.add_option(dpp::command_option(dpp::co_sub_command, "kick", "Kicks a player")
					.add_option(dpp::command_option(dpp::co_string, "user", "The name of player to kick", true).set_auto_complete(true)).set_auto_complete(true)
				);*/

			/*
			cmd.add_option(dpp::command_option(dpp::co_sub_command, "save", "Saves the world"))
				.add_option(dpp::command_option(dpp::co_sub_command, "list", "Lists all online players"))
				.add_option(dpp::command_option(dpp::co_sub_command, "kick", "Kicks a player")
					.add_option(dpp::command_option(dpp::co_string, "user", "The name of player to kick", true).set_auto_complete(true))
				);
				
			dpp::slashcommand drinkcmd("drink", "Test drink command", m_bot->me.id);
			drinkcmd.add_option(dpp::command_option(dpp::co_string, "item", "Choose a drink").set_auto_complete(true));
			*/

			/*
			newcommand.add_option(
					dpp::command_option(dpp::co_string, "animal", "The type of animal", true).
						add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
						add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
						add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin")
					)
				)
			);*/

			/* Register the command */
			//m_bot->global_command_create(cmd);
			//m_bot->global_command_create(drinkcmd);
		}
	});

	m_bot->start(dpp::st_return);
}

void IDiscordManager::SendSimpleMessage(std::string_view msg) {
	if (VH_SETTINGS.discordWebhook.empty())
		return;

	std::string json = "{ \"content\": \"" + std::string(msg) + "\" }";
	auto bytes = BYTES_t(json.data(), json.data() + json.length());
	DispatchRequest(VH_SETTINGS.discordWebhook, std::move(bytes));
}

void IDiscordManager::DispatchRequest(std::string_view webhook, BYTES_t payload) {	
	auto&& client = VH_SETTINGS.serverDedicated ? SteamGameServerHTTP() : SteamHTTP();

	auto&& req = client->CreateHTTPRequest(k_EHTTPMethodPOST, webhook.data());

	if (!client->SetHTTPRequestRawPostBody(req, "application/json", reinterpret_cast<uint8_t*>(payload.data()), payload.size()))
		LOG(WARNING) << "Failed to set http content";

	SteamAPICall_t handle{};
	if (!client->SendHTTPRequest(req, &handle))
		LOG(WARNING) << "Failed to send webhook http request";

	m_httpRequestCompletedCallResult.Set(handle, this, &IDiscordManager::OnHTTPRequestCompleted);
}

void IDiscordManager::OnHTTPRequestCompleted(HTTPRequestCompleted_t* pCallback, bool failure) {
	if (failure) {
		LOG(WARNING) << "http failed to send";
	}
	else {
		if (pCallback->m_eStatusCode != k_EHTTPStatusCode200OK && pCallback->m_eStatusCode != k_EHTTPStatusCode204NoContent) {
			LOG(WARNING) << "http status code error: " << pCallback->m_eStatusCode;
		}
	}
}
