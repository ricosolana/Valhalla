#pragma once

#include <fstream>
#include <chrono>
#include <memory>

#include "Game.hpp"
#include "NetPeer.hpp"
#include <robin_hood.h>

namespace Alchyme {
	class Server : public Game {
		std::future<std::string> consoleFuture;

		// The peers that are attached to one particular socket
		std::list<std::unique_ptr<Net::Peer>> m_peers;

		// server settings.txt
		robin_hood::unordered_map<std::string, std::string> m_settings;

		// Whitelisted names
		robin_hood::unordered_set<std::string> m_whitelist;

		// Banned addresses
		robin_hood::unordered_set<std::string> m_banned;
		bool m_useWhitelist = false;
		uint16_t m_maxPeers = 10;
		size_t m_passwordHash = 0;

		std::unique_ptr<tcp::acceptor> m_acceptor;

		std::string m_serverTitle;
		std::string m_serverDesc;
		std::chrono::seconds m_serverCreateTime;
		std::chrono::seconds m_serverStartTime;
		std::chrono::seconds m_serverPrevUpDur;

		// from the 3 above chrono specifiers, the server age in seconds can be derived by
		// totalUpTime = m_serverPrevUpDuration + (now - m_serverTimeStarted)

	public:
		static Server* Get();

		Server();

		void Start() override;
		void Stop() override;

		void Ban(const std::string&);
		void AddToWhitelist(const std::string&);
		bool UnBan(const std::string&);
		bool RemoveFromWhitelist(const std::string&);
		bool IsBanned(const std::string&);

		void Kick(Net::Peer* peer, std::string reason = "");

	private:
		void Update(float delta) override;

		void ConnectCallback(Net::Peer* peer) override;
		void DisconnectCallback(Net::Peer* peer) override;

		void DoAccept();

		void RPC_ServerHandshake(Net::Peer* peer, int magic, ConnectMode mode);
		void RPC_LoginInfo(Net::Peer* peer, std::string version, std::string name, UID uid, size_t passwordHash);
		void RPC_Print(Net::Peer* peer, std::string s);
		void RPC_BlacklistIp(Net::Peer* peer, std::string host);
		void RPC_BlacklistIpByName(Net::Peer* peer, std::string name);
		void RPC_UnBlacklist(Net::Peer* peer, std::string host);
		void RPC_Whitelist(Net::Peer* peer, std::string key);
		void RPC_UnWhitelist(Net::Peer* peer, std::string key);
		void RPC_ModeWhitelist(Net::Peer* peer, bool useWhitelist);

		NULLABLE Net::Peer* GetPeer(size_t uid);
		NULLABLE Net::Peer* GetPeer(std::string_view name);
		NOTNULL Net::Peer* GetPeer(NOTNULL Net::Rpc* rpc);

		PeerResult PeerAllowedToJoin(Net::Peer* peer, std::string version, size_t passwordHash);
		PeerResult PeerAllowedToStay(Net::Peer* peer);

		void SaveUsers();
		void LoadUsers();
	};
}
