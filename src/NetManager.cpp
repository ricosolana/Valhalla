#include <optick.h>
#include <openssl/md5.h>
#include <openssl/rand.h>

#include "ModManager.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "World.h"
#include "ZoneSystem.h"
#include "NetSyncManager.h"

using namespace std::chrono;

namespace NetManager {
	//double m_netTime = 2040;
	//static auto m_startTime = steady_clock::now();

	std::unique_ptr<IAcceptor> m_acceptor;

	std::vector<std::unique_ptr<NetRpc>> m_joining; // used to temporarily connecting peers (until PeerInfo)
	std::vector<std::unique_ptr<NetPeer>> m_peers;
	std::unique_ptr<World> m_world;

	bool m_hasPassword = false;
	std::string m_salt;
	std::string m_saltedPassword;

	// used during server start
	void InitPassword() {
		m_hasPassword = !Valhalla()->Settings().serverPassword.empty();

		if (m_hasPassword) {
			// Create random 16 byte salt
			m_salt.resize(16);
			RAND_bytes(reinterpret_cast<uint8_t*>(m_salt.data()), m_salt.size());
			Utils::FormatAscii(m_salt);

			const auto merge = Valhalla()->Settings().serverPassword + m_salt;

			// Hash a salted password
			m_saltedPassword.resize(16);
			MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
				merge.size(), reinterpret_cast<uint8_t*>(m_saltedPassword.data()));

			Utils::FormatAscii(m_saltedPassword);
		}
	}




	void RemotePrint(NetRpc* rpc, const std::string& s) {
		rpc->Invoke(Rpc_Hash::RemotePrint, s);
	}

	void Disconnect(NetPeer *peer) {
		//peer->m_rpc->m_socket->Close();
        peer->Disconnect();
	}




	void Kick(NetPeer *peer) {
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

	void SendDisconnect(NetPeer *peer) {
		LOG(INFO) << "Disconnect sent to " << peer->m_rpc->m_socket->GetHostName();
		peer->m_rpc->Invoke("Disconnect");
	}

	void SendDisconnect() {
		LOG(INFO) << "Sending disconnect msg";

		for (auto&& peer : m_peers) {
			SendDisconnect(peer.get());
		}
	}



	void RPC_ServerHandshake(NetRpc* rpc) {
		//LOG(INFO) << "Client initiated handshake " << peer->m_socket->GetHostName();
		//this.ClearPlayerData(peer);
		//bool flag = !Valhalla()->m_serverPassword.empty();
		//std::string salt = "Im opposing salt"; // must be 16 bytes
		rpc->Invoke("ClientHandshake", m_hasPassword, m_salt);
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
			NetPackage pkg;
			pkg.Write((int)m_peers.size());

			for (auto&& peer : m_peers) {
				pkg.Write(peer->m_name);
				pkg.Write(peer->m_rpc->m_socket->GetHostName());
				pkg.Write(peer->m_characterID);
				pkg.Write(peer->m_visibleOnMap);
				if (peer->m_visibleOnMap) {
					pkg.Write(peer->m_pos);
				}
			}

			for (auto&& peer : m_peers) {
				// this is the problem
				peer->m_rpc->Invoke(Rpc_Hash::PlayerList, pkg);
			}
		}
	}

	void SendNetTime() {
		for (auto&& peer : m_peers) {
			peer->m_rpc->Invoke(Rpc_Hash::NetTime, (double)Valhalla()->Time());
		}
	}







	void SendPeerInfo(NetRpc* rpc) {
		//auto now(steady_clock::now());
		//double netTime =
		//	(double)duration_cast<milliseconds>(now - m_startTime).count() / (double)((1000ms).count());
		NetPackage pkg;
		pkg.Write(Valhalla()->ID());
		pkg.Write(VALHEIM_VERSION);
		pkg.Write(Vector3()); // dummy
		pkg.Write("Stranger"); // valheim uses this, which is dumb

		// why does a server need to send a position and name?
		// clearly someone didnt think of the protocol

		pkg.Write(m_world->m_name);
		pkg.Write(m_world->m_seed);
		pkg.Write(m_world->m_seedName);
		pkg.Write(m_world->m_uid);
		pkg.Write(m_world->m_worldGenVersion);
		pkg.Write((double)Valhalla()->Time());

		rpc->Invoke(Rpc_Hash::PeerInfo, pkg);
	}

	void RPC_PeerInfo(NetRpc* rpc, NetPackage pkg) {
		assert(rpc && rpc->m_socket);

		auto&& hostName = rpc->m_socket->GetHostName();

		auto uuid = pkg.Read<OWNER_t>();
		auto version = pkg.Read<std::string>();
		LOG(INFO) << "Client " << hostName << " has version " << version;
		if (version != VALHEIM_VERSION)
			return rpc->SendError(ConnectionStatus::ErrorVersion);

		auto pos = pkg.Read<Vector3>();
		auto name = pkg.Read<std::string>();
		auto password = pkg.Read<std::string>();
		auto ticket = pkg.Read<BYTES_t>(); // read in the dummy ticket

        auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
		if (steamSocket && SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID()) != k_EBeginAuthSessionResultOK)
			return rpc->SendError(ConnectionStatus::ErrorBanned);

		if (password != m_saltedPassword)
			return rpc->SendError(ConnectionStatus::ErrorPassword);

		// if peer already connected
		if (GetPeer(uuid))
			return rpc->SendError(ConnectionStatus::ErrorAlreadyConnected);

        if (Valhalla()->m_banned.contains(rpc->m_socket->GetHostName()))
            return rpc->SendError(ConnectionStatus::ErrorBanned);

		// pass the data to the lua OnPeerInfo
		if (!ModManager::Event::OnPeerInfo(rpc, uuid, name, version))
			return rpc->SendError(ConnectionStatus::ErrorBanned);

		// Find the rpc and transfer

		std::unique_ptr<NetRpc> swappedRpc;
		for (auto&& j : m_joining) {
			if (j.get() == rpc) {
				swappedRpc = std::move(j);
                assert(!j && "Never moved!");
				break;
			}
		}
		assert(swappedRpc && "RPC was never swapped!");

		auto peer(std::make_unique<NetPeer>(std::move(swappedRpc), uuid, name));

		peer->m_pos = pos;

		rpc->Register(Rpc_Hash::RefPos, &RPC_RefPos);
		rpc->Register(Rpc_Hash::CharacterID, &RPC_CharacterID);
		rpc->Register(Rpc_Hash::Kick, &RPC_Kick);
		rpc->Register(Rpc_Hash::Ban, &RPC_Ban);
		rpc->Register(Rpc_Hash::Unban, &RPC_Unban);
		rpc->Register(Rpc_Hash::Save, &RPC_Save);
		rpc->Register(Rpc_Hash::PrintBanned, &RPC_PrintBanned);

		SendPeerInfo(rpc);

		//NetSyncManager::OnNewPeer(peer);
		//NetSyncManager::OnNewPeer(peer);
		NetRouteManager::OnNewPeer(peer.get());
		ZoneSystem::OnNewPeer(peer.get());

        m_peers.push_back(std::move(peer));
	}

	// Retrieve a peer by its member Rpc
	// Will throw if peer by rpc is not found
	NetPeer *GetPeer(NetRpc* rpc) {
		for (auto&& peer : m_peers) {
			if (peer->m_rpc.get() == rpc)
				return peer.get();
		}
		throw std::runtime_error("Unable to find Peer attributing to Rpc");
	}

	// Return the peer or nullptr
	NetPeer *GetPeer(const std::string& name) {
		for (auto&& peer : m_peers) {
			if (peer->m_name == name)
				return peer.get();
		}
		return nullptr;
	}

	// Return the peer or nullptr
	NetPeer *GetPeer(OWNER_t uuid) {
		for (auto&& peer : m_peers) {
			if (peer->m_uuid == uuid)
				return peer.get();
		}
		return nullptr;
	}



	void Init() {
		m_acceptor = std::make_unique<AcceptorSteam>();
		m_acceptor->Listen();

		InitPassword();

		m_world = std::make_unique<World>();

		ZoneSystem::Init();
	}

	void Update(double delta) {
		OPTICK_EVENT();
		// Accept new connections into joining
		while (auto opt = m_acceptor->Accept()) {
			auto&& rpc = std::make_unique<NetRpc>(opt.value());

			rpc->Register(Rpc_Hash::PeerInfo, &RPC_PeerInfo);
			rpc->Register(Rpc_Hash::Disconnect, &RPC_Disconnect);
			rpc->Register(Rpc_Hash::ServerHandshake, &RPC_ServerHandshake);

			rpc->m_socket->Start();

			m_joining.push_back(std::move(rpc));
		}

		// Cleanup joining
		{
			auto&& itr = m_joining.begin();
			while (itr != m_joining.end()) {
                if (!(*itr && (*itr)->m_socket->Connected())) {
                    itr = m_joining.erase(itr);
                } else {
                    ++itr;
                }
			}
		}

		// Cleanup peers
		{
			// Remove stale peers
			auto&& itr = m_peers.begin();
			while (itr != m_peers.end()) {
				if (!(*itr)->m_rpc->m_socket->Connected()) {
					LOG(INFO) << "Cleaning up peer";
					itr = m_peers.erase(itr);
				}
				else {
					++itr;
				}
			}
		}

		// Send periodic data (2s)
		PERIODIC_NOW(2s, {
			SendNetTime();
			SendPlayerList();
		});		

		// Send periodic pings (1s)
		PERIODIC_NOW(1s, {
			for (auto&& rpc : m_joining) {
				LOG(DEBUG) << "Rpc join pinging ";
				//auto pkg(PKG());
				NetPackage pkg;
				pkg.Write<HASH_t>(0);
				pkg.Write(true);
				rpc->m_socket->Send(pkg);
			}

			for (auto&& peer : m_peers) {
				LOG(DEBUG) << "Rpc pinging " << peer->m_uuid;
				NetPackage pkg;
				pkg.Write<HASH_t>(0);
				pkg.Write(true);
				peer->m_rpc->m_socket->Send(pkg);
			}
		});

		// Update peers
		for (auto&& peer : m_peers) {
			try {
				peer->m_rpc->Update();
			}
			catch (const std::range_error& e) {
				LOG(ERROR) << "Peer provided malformed data: " << e.what();
				peer->m_rpc->m_socket->Close(false);
			}
		}

		// Update joining (after peer update to avoid double updating any moved peer)
		for (auto&& rpc : m_joining) {
			try {
				rpc->Update();
			}
			catch (const std::range_error& e) {
				LOG(ERROR) << "Connecting peer provided malformed payload";
				rpc->m_socket->Close(false);
			}
		}

		//m_netTime += delta;
		SteamGameServer_RunCallbacks();
	}

	void Close() {
		m_acceptor.reset();
	}

	const std::vector<std::unique_ptr<NetPeer>>& GetPeers() {
		return m_peers;
	}

}
