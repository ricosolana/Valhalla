#include "Client.hpp"
#include "ScriptManager.hpp"

namespace Valhalla {
	void Client::RPC_ClientHandshake(Net::Peer* peer, int magic) {
		LOG(INFO) << "ClientHandshake()!";

		auto rpc = peer->m_rpc.get();

		if (magic != MAGIC) {
			peer->Disconnect();
		}
		else {
			rpc->Register("Print", new Net::Method(this, &Client::RPC_Print));
			rpc->Register("PeerInfo", new Net::Method(this, &Client::RPC_PeerInfo));
			rpc->Register("Error", new Net::Method(this, &Client::RPC_Error));

			rpc->Invoke("ServerHandshake", MAGIC, VERSION);
		}
	}

	void Client::RPC_ModeStatus(Net::Peer* peer, std::string svTitle, std::string svDesc, std::chrono::seconds svCreateTime, std::chrono::seconds svStartTime, std::chrono::seconds svPrevUpDur, std::string svVer, size_t svConnections) {
		LOG(INFO) << "Server Status: " << svTitle << ", " << svDesc << ", " << svCreateTime << ", " << svStartTime << ", " << svPrevUpDur << ", " << svVer << ", " << svConnections;
	}

	void Client::RPC_ModeLogin(Net::Peer* peer, std::string serverVersion) {
		LOG(INFO) << "Initiating Server Login...";
		serverAwaitingLogin = true;

		ScriptManager::Event::OnLogin();
	}

	void Client::RPC_PeerInfo(Net::Peer* peer,
		size_t peerUid,
		size_t worldSeed,
		size_t worldTime) {

		LOG(DEBUG) << "my uid: " << peerUid <<
			", worldSeed: " << worldSeed <<
			", worldTime: " << worldTime;
	}

	void Client::RPC_Print(Net::Peer* peer, std::string s) {
		LOG(INFO) << "Remote print: " << s << "\n";
	}

	void Client::RPC_Error(Net::Peer* peer, std::string s) {
		LOG(ERROR) << "Remote error: " << s << "\n";
	}
}
