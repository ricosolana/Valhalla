#include "VUtils.h"
#include "DiscordManager.h"
#include <dpp/dispatcher.h>
#include <quill/Quill.h>

auto DISCORD_MANAGER(std::make_unique<IDiscordManager>());
IDiscordManager* DiscordManager() {
	return DISCORD_MANAGER.get();
}

//std::unique_ptr<dpp::cluster> m_bot;

/*
class DiscordClientInterceptor : public dpp::discord_client {
public:

};*/

void IDiscordManager::Init() {
	// https://dpp.dev/slashcommands.html

	//m_bot = std::make_unique<dpp::cluster>("");
	

	//logger->log()
	m_bot->on_log([](const dpp::log_t& log) {
		//LOG_DEBUG(logger, log.message);

		switch (log.severity) {
		case dpp::loglevel::ll_trace: //LOG_BACKTRACETRACE(log.message); break;
		case dpp::loglevel::ll_debug: LOG_DEBUG(logger, "{}", log.message.c_str()); break;
		case dpp::loglevel::ll_info: LOG_INFO(logger, "{}", log.message); break;
		case dpp::loglevel::ll_warning: LOG_WARNING(logger, "{}", log.message); break;
		case dpp::loglevel::ll_error: // fallthrough
		case dpp::loglevel::ll_critical: LOG_ERROR(logger, "{}", log.message); break;
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
			LOG_INFO(logger, "Running slash command!");
			event.reply("Command success!");
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
					.set_default_permissions(0), // 0 is admins only
				dpp::slashcommand("vhpardon", "Unban a player", m_bot->me.id)
					.add_option(dpp::command_option(dpp::co_string, "host", "host", true))
					.set_default_permissions(dpp::permissions::p_ban_members), // 0 is admins only
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
			}, 0);
		}
	});

	m_bot->start(dpp::st_return);
}
