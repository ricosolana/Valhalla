#include "Server.hpp"

namespace Alchyme {
	void Server::RPC_ServerHandshake(Net::Peer* peer, int magic, ConnectMode mode) {
		LOG(INFO) << "ServerHandshake()!";

		auto rpc = peer->m_rpc.get();

		if (magic != MAGIC)
			peer->Disconnect();
		//rpc->m_socket->Close();
		else {
			rpc->Register("Print", new Net::Method(this, &Server::RPC_Print));

			if (mode == ConnectMode::STATUS) {
				// send information about server

				// RPC_ModeStatus(serverName, serverBirthDate, serverUpTime, serverStartTime, serverConnections, serverHead, serverDesc)
				rpc->Invoke("ModeStatus", m_serverTitle, m_serverDesc, m_serverCreateTime, m_serverStartTime, m_serverPrevUpDur, VERSION, m_peers.size());
			}
			else if (mode == ConnectMode::LOGIN) {
				rpc->Register("LoginInfo", new Net::Method(this, &Server::RPC_LoginInfo));
				// await info?
				rpc->Invoke("ModeLogin", VERSION);
			}
		}
	}

	void Server::RPC_LoginInfo(Net::Peer* peer,
		std::string version,
		std::string name,
		UID uid,
		size_t passwordHash) {

		LOG(INFO) << "Received LoginInfo";

		peer->m_name = name;
		peer->m_uid = uid;

		auto rpc = peer->m_rpc.get();

		auto res = PeerAllowedToJoin(peer, version, passwordHash);
		if (res != PeerResult::OK) {
			rpc->Invoke("PeerResult", res);
			peer->DisconnectLater();
			return;
		}

		peer->m_authorized = true;

		rpc->Invoke("PeerInfo",
			peer->m_uid, Utils::StrHash("my world"), size_t(0));
	}

	void Server::RPC_Print(Net::Peer* peer, std::string s) {
		LOG(INFO) << "Remote print: " << s;
	}

	void Server::RPC_BlacklistIp(Net::Peer* peer, std::string host) {
		Ban(host);
	}

	void Server::RPC_BlacklistIpByName(Net::Peer* peer, std::string name) {
		Net::Peer* otherPeer = GetPeer(name);
		auto rpc = peer->m_rpc.get();
		if (otherPeer && otherPeer->m_rpc->m_socket)
			Ban(rpc->m_socket->GetHostName());
		else
			rpc->Invoke("Error", std::string("Player not found"));
	}

	void Server::RPC_UnBlacklist(Net::Peer* peer, std::string host) {
		UnBan(host);
	}

	void Server::RPC_Whitelist(Net::Peer* peer, std::string key) {
		AddToWhitelist(key);
	}

	void Server::RPC_UnWhitelist(Net::Peer* peer, std::string key) {
		RemoveFromWhitelist(key);
	}

	void Server::RPC_ModeWhitelist(Net::Peer* peer, bool useWhitelist) {
		m_useWhitelist = useWhitelist;
	}
}
