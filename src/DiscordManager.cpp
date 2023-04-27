#include <isteamgameserver.h>

#include "DiscordManager.h"
#include "ValhallaServer.h"

auto DISCORD_MANAGER(std::make_unique<IDiscordManager>());
IDiscordManager* DiscordManager() {
	return DISCORD_MANAGER.get();
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
