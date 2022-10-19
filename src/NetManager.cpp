#include <openssl/md5.h>
#include "ModManager.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "World.h"
#include "ZoneSystem.h"
#include "NetSyncManager.h"
#include <optick.h>

using namespace asio::ip;
using namespace std::chrono;

namespace NetManager {
	double m_netTime = 2040;
	//static auto m_startTime = steady_clock::now();

	asio::io_context m_ctx;
	std::unique_ptr<IAcceptor> m_acceptor;

	std::vector<std::unique_ptr<NetRpc>> m_joining;
	std::vector<NetPeer::Ptr> m_peers;
	std::unique_ptr<World> m_world;

	void RemotePrint(NetRpc* rpc, const std::string& s) {
		rpc->Invoke("RemotePrint", s);
	}

	void Disconnect(NetPeer::Ptr peer) {
		peer->m_rpc->m_socket->Close();
	}




	void Kick(NetPeer::Ptr peer) {
		if (!peer)
			return;

		peer->Kick();
	}

	void Kick(const std::string& user) {
		Kick(GetPeer(user));
	}

	void Ban(const std::string& user) {
		LOG(INFO) << "Banning " << user;

		Valhalla()->m_banned.insert(user);
	}

	void Unban(const std::string& user) {
		LOG(INFO) << "Unbanning " << user;

		Valhalla()->m_banned.erase(user);
	}

	void SendDisconnect(NetPeer::Ptr peer) {
		LOG(INFO) << "Disconnect sent to " << peer->m_rpc->m_socket->GetHostName();
		peer->m_rpc->Invoke("Disconnect");
	}

	void SendDisconnect() {
		LOG(INFO) << "Sending disconnect msg";

		for (auto&& peer : m_peers) {
			SendDisconnect(peer);
		}
	}



	void RPC_ServerHandshake(NetRpc* rpc) {
		//LOG(INFO) << "Client initiated handshake " << peer->m_socket->GetHostName();
		//this.ClearPlayerData(peer);
		bool flag = !Valhalla()->m_serverPassword.empty();
		std::string salt = "Im opposing salt"; // must be 16 bytes
		rpc->Invoke("ClientHandshake", flag, salt);
	}

	void RPC_Disconnect(NetRpc* rpc) {
		LOG(INFO) << "RPC_Disconnect";
		auto&& peer = GetPeer(rpc);
		Disconnect(peer);
	}


	void RPC_RefPos(NetRpc* rpc, Vector3 pos, bool publicRefPos) {
		auto&& peer = GetPeer(rpc);

		peer->m_pos = pos;
		peer->m_visibleOnMap = publicRefPos; // stupid name
	}

	void RPC_CharacterID(NetRpc* rpc, NetID characterID) {
		auto&& peer = GetPeer(rpc);
		peer->m_characterID = characterID;

		LOG(INFO) << "Got character NetSyncID from " << peer->m_name << " : " << characterID.ToString();
	}



	void RPC_Kick(NetRpc* rpc, std::string user) {
		// check if rpc is admin first
		// if (!rpc.perm_admin...) return

		std::string msg = "Kicking user " + user;
		RemotePrint(rpc, msg);
		Kick(user);
	}

	void RPC_Ban(NetRpc* rpc, std::string user) {
		std::string msg = "Banning user " + user;
		RemotePrint(rpc, msg);
		Ban(user);
	}

	void RPC_Unban(NetRpc* rpc, std::string user) {
		std::string msg = "Unbanning user " + user;
		RemotePrint(rpc, msg);
		Unban(user);
	}

	void RPC_Save(NetRpc* rpc) {

	}

	void RPC_PrintBanned(NetRpc* rpc) {
		std::string s = "Banned users";
		//std::vector<std:

		RemotePrint(rpc, s);
	}


	//void ZNet::Unban(NetPeer::Ptr peer) {
	//
	//}






	void SendPlayerList() {
		if (!m_peers.empty()) {
			auto pkg(PKG());
			pkg->Write((int)m_peers.size());

			for (auto&& peer : m_peers) {
				pkg->Write(peer->m_name);
				pkg->Write(peer->m_rpc->m_socket->GetHostName());
				pkg->Write(peer->m_characterID);
				pkg->Write(peer->m_visibleOnMap);
				if (peer->m_visibleOnMap) {
					pkg->Write(peer->m_pos);
				}
			}

			for (auto&& peer : m_peers) {
				// this is the problem
				peer->m_rpc->Invoke("PlayerList", pkg);
			}
		}
	}

	void SendNetTime() {
		for (auto&& peer : m_peers) {
			peer->m_rpc->Invoke("NetTime", m_netTime);
		}
	}







	void SendPeerInfo(NetRpc* rpc) {
		//auto now(steady_clock::now());
		//double netTime =
		//	(double)duration_cast<milliseconds>(now - m_startTime).count() / (double)((1000ms).count());
		auto pkg(PKG());
		pkg->Write(Valhalla()->m_serverUuid);
		pkg->Write(SERVER_VERSION);
		pkg->Write(Vector3()); // dummy
		pkg->Write("Stranger"); // valheim uses this, which is dumb

		// why does a server need to send a position and name?
		// clearly someone didnt think of the protocol

		pkg->Write(m_world->m_name);
		pkg->Write(m_world->m_seed);
		pkg->Write(m_world->m_seedName);
		pkg->Write(m_world->m_uid);
		pkg->Write(m_world->m_worldGenVersion);
		pkg->Write(m_netTime);

		rpc->Invoke("PeerInfo", pkg);
	}

	void RPC_PeerInfo(NetRpc* rpc, NetPackage::Ptr pkg) {
		auto&& hostName = rpc->m_socket->GetHostName();

		auto uuid = pkg->Read<uuid_t>();
		auto version = pkg->Read<std::string>();
		LOG(INFO) << "Client " << hostName << " has version " << version;
		if (version != SERVER_VERSION) {
			return rpc->SendError(ConnectionStatus::ErrorVersion);
		}

		auto pos = pkg->Read<Vector3>();
		auto name = pkg->Read<std::string>();
		auto password = pkg->Read<std::string>();
		pkg->Read<std::vector<byte_t>>(); // read in the dummy ticket

		if (password != Valhalla()->m_serverPassword) {
			return rpc->SendError(ConnectionStatus::ErrorPassword);
		}

		// if peer already connected
		if (GetPeer(uuid)) {
			return rpc->SendError(ConnectionStatus::ErrorAlreadyConnected);
		}

		// check if banned
		//if ()

		//if (IsBann)

		// pass the data to the lua OnPeerInfo
		if (!ModManager::Event::OnPeerInfo(rpc, uuid, name, version)) {
			rpc->SendError(ConnectionStatus::ErrorBanned);
			return;
		}

		// Find the rpc and transfer
		std::unique_ptr<NetRpc> swappedRpc;
		for (auto&& j : m_joining) {
			if (j.get() == rpc) {
				swappedRpc = std::move(j);
				break;
			}
		}
		assert(swappedRpc && "Swapped rpc wsa never assigned!");

		auto peer(std::make_shared<NetPeer>(std::move(swappedRpc), uuid, name));
		m_peers.push_back(peer);

		peer->m_pos = pos;

		rpc->Register("RefPos", &RPC_RefPos);
		rpc->Register("CharacterID", &RPC_CharacterID);
		rpc->Register("Kick", &RPC_Kick);
		rpc->Register("Ban", &RPC_Ban);
		rpc->Register("Unban", &RPC_Unban);
		rpc->Register("Save", &RPC_Save);
		rpc->Register("PrintBanned", &RPC_PrintBanned);

		SendPeerInfo(rpc);

		//NetSyncManager::OnNewPeer(peer);
		//NetSyncManager::OnNewPeer(peer);
		NetRpcManager::OnNewPeer(peer);
		ZoneSystem::OnNewPeer(peer);
	}

	// Retrieve a peer by its member Rpc
	// Will breakpoint if peer not found
	NetPeer::Ptr GetPeer(NetRpc* rpc) {
		for (auto&& peer : m_peers) {
			if (peer->m_rpc.get() == rpc)
				return peer;
		}sizeof(NetSync);
		//return nullptr;
		assert(false && "Unable to find Peer attributing to Rpc");
		throw std::runtime_error("Unable to find Peer attributing to Rpc");
	}

	// Return the peer or nullptr
	NetPeer::Ptr GetPeer(const std::string& name) {
		for (auto&& peer : m_peers) {
			if (peer->m_name == name)
				return peer;
		}
		return nullptr;
	}

	// Return the peer or nullptr
	NetPeer::Ptr GetPeer(uuid_t uuid) {
		for (auto&& peer : m_peers) {
			if (peer->m_uuid == uuid)
				return peer;
		}
		return nullptr;
	}



	void Listen(uint16_t port) {
		m_acceptor = std::make_unique<AcceptorZSocket2>(m_ctx, port);
		m_acceptor->Start();

		m_world = std::make_unique<World>();

		ZoneSystem::Init();

		// net time and player list sent every 2 seconds to players
		Valhalla()->RunTaskRepeat([](Task*) {
			SendNetTime();
			SendPlayerList();
		}, 2s);

		// Ping rpcs every second
		Valhalla()->RunTaskRepeat([](Task*) {
			for (auto&& rpc : m_joining) {
				LOG(DEBUG) << "Rpc join pinging ";
				auto pkg(PKG());
				pkg->Write<int32_t>(0);
				pkg->Write(true);
				rpc->m_socket->Send(pkg);
			}

			for (auto&& peer : m_peers) {
				LOG(DEBUG) << "Rpc pinging " << peer->m_uuid;
				auto pkg(PKG());
				pkg->Write<int32_t>(0);
				pkg->Write(true);
				peer->m_rpc->m_socket->Send(pkg);
			}
		}, 1s);
	}

	void Update(double delta) {
		OPTICK_EVENT();
		// Accept connections
		while (auto socket = m_acceptor->Accept()) {
			auto&& rpc = std::make_unique<NetRpc>(socket);

			rpc->Register("PeerInfo", &RPC_PeerInfo);
			rpc->Register("Disconnect", &RPC_Disconnect);
			rpc->Register("ServerHandshake", &RPC_ServerHandshake);

			rpc->m_socket->Start();

			m_joining.push_back(std::move(rpc));
		}

		{
			// Remove invalid joining
			// Removes any
			auto&& itr = m_joining.begin();
			while (itr != m_joining.end()) {
				if (!(*itr) || !(*itr)->m_socket->Connected()) {
					itr = m_joining.erase(itr);
				}
				else {
					++itr;
				}
			}
		}

		{
			// Remove stale peers
			auto&& itr = m_peers.begin();
			while (itr != m_peers.end()) {
				if (!(*itr)->m_rpc->m_socket->Connected()) {
					itr = m_peers.erase(itr);
				}
				else {
					++itr;
				}
			}
		}

		// Update peers
		for (auto&& peer : m_peers) {
			try {
				peer->m_rpc->Update();
			}
			catch (const std::range_error& e) {
				LOG(ERROR) << "Peer might be out of sync: " << e.what();
				peer->m_rpc->m_socket->Close();
			}
		}

		// Update joining
		//	this is done after peers rpc update because in the weird case
		//	where a joining becomes a peer then is processed twice in an update tick
		for (auto&& rpc : m_joining) {
			try {
				rpc->Update();
			}
			catch (const std::range_error& e) {
				LOG(ERROR) << "Connecting peer provided malformed payload";
				rpc->m_socket->Close();
			}
		}

		m_netTime += delta;
		SteamGameServer_RunCallbacks();
	}

	void Close() {
		m_acceptor.reset();
	}

	const std::vector<NetPeer::Ptr>& GetPeers() {
		return m_peers;
	}

}
