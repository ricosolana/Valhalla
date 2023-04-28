#include <isteamgameserver.h>
#include <dpp/dpp.h>

#include "DiscordManager.h"
#include "ValhallaServer.h"

auto DISCORD_MANAGER(std::make_unique<IDiscordManager>());
IDiscordManager* DiscordManager() {
	return DISCORD_MANAGER.get();
}



void IDiscordManager::Init() {
	// https://dpp.dev/slashcommands.html
	//dpp::cluster bot(VH_SETTINGS.discordToken);

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

	m_bot->on_slashcommand([](const dpp::slashcommand_t& evt) {
		auto&& name = evt.command.get_command_name();

		if (name == "vh") {
			evt.reply(std::string("The vh command woo!"));
		}
	});

	m_bot->on_ready([this](const dpp::ready_t& evt) {
		if (dpp::run_once<struct register_bot_commands>()) {

			dpp::slashcommand newcommand("vh", "The Valhalla server command", m_bot->me.id);
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
			m_bot->global_command_create(newcommand);
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
