#include "ZNet.h"
#include "ValhallaServer.h"
#include "ModManager.h"
#include <openssl/md5.h>
#include "ValhallaServer.h"

using namespace asio::ip;

ZNet::ZNet(uint16_t port) 
	: m_ctx() {

	m_acceptor = std::make_unique<AcceptorZSocket2>(m_ctx, port);
}

void ZNet::Listen() {
	LOG(INFO) << "Starting server";

	//uuid_t uid(1234567891011);

	m_routedRpc = std::make_unique<ZRoutedRpc>();
	m_zdoMan = std::make_unique<ZDOMan>();
	m_world = std::make_unique<World>();

	Valhalla()->RunTaskLaterRepeat([this](Task* self) {
		SendNetTime();
		SendPlayerList();		
	}, 1s, 2s);

	m_acceptor->Start();
}

void ZNet::Update() {

	// UPDATE()
	//   update banlist
	//     disconnect msg dispatch
	//     removed from peers
	//     socket terminated
	//   Checks for new queued connections
	//	   Checks if still connected?
	//     adds to peers
	//     register rpcs
	//	 UpdatePeers()
	//	   removes any stale m_peers
	//     updates rpcs
	//   periodic sends
	//   zdo updates
	//   

	// Accept connections
	while (m_acceptor->HasNewConnection()) {
		auto&& rpc = std::make_unique<ZRpc>(m_acceptor->Accept());

		rpc->Register("PeerInfo", this, &ZNet::RPC_PeerInfo);
		rpc->Register("Disconnect", this, &ZNet::RPC_Disconnect);
		rpc->Register("ServerHandshake", this, &ZNet::RPC_ServerHandshake);

		//rpc->Register("ServerHandshake", [](ZRpc*) {
		//	LOG(INFO) << "Lambda handshake!";
		//});

		rpc->m_socket->Start();

		m_joining.push_back(std::move(rpc));
	}
	
	{
		// Remove invalid joining
		// Removes any
		auto&& itr = m_joining.begin();
		while (itr != m_joining.end()) {
			if (!(*itr) || (*itr)->m_socket->GetConnectivity() == Connectivity::CLOSED) {
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
			if ((*itr)->m_rpc->m_socket->GetConnectivity() == Connectivity::CLOSED) {
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
			LOG(ERROR) << "Peer provided malformed data";
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
			LOG(ERROR) << "Connecting peer provided malformed data";
			rpc->m_socket->Close();
		}
	}
}

void ZNet::SendPeerInfo(ZRpc* rpc) {
	auto pkg(PKG());
	pkg->Write(GetUID());
	pkg->Write(ValhallaServer::VERSION);
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

static void my_static(ZRpc*) {}

// The server send peer info once the client is completely validated
void ZNet::RPC_PeerInfo(ZRpc* rpc, ZPackage::Ptr pkg) {
	auto &&hostName = rpc->m_socket->GetHostName();

	auto uuid = pkg->Read<uuid_t>();
	auto version = pkg->Read<std::string>();
	LOG(INFO) << "Client " << hostName << " has version " << version;
	if (version != std::string(ValhallaServer::VERSION)) {
		return rpc->SendError(ConnectionStatus::ErrorVersion);
	}

	auto pos = pkg->Read<Vector3>();
	auto name = pkg->Read<std::string>();
	auto password = pkg->Read<std::string>();
	//auto ticket = read array... // normally for steam
	
	if (password != Valhalla()->m_serverPassword) {
		return rpc->SendError(ConnectionStatus::ErrorPassword);
	}

	// if peer already connected
	if (GetPeer(uuid)) {
		return rpc->SendError(ConnectionStatus::ErrorAlreadyConnected);
	}

	// check if banned
	//if ()

	// pass the data to the lua OnPeerInfo
	if (!ModManager::Event::OnPeerInfo(rpc->m_socket, uuid, name, version))
		return;
	
	// Find the rpc and transfer
	std::unique_ptr<ZRpc> swappedRpc;
	for (auto&& j : m_joining) {
		if (j.get() == rpc) {
			swappedRpc = std::move(j);
			break;
		}
	}
	assert(swappedRpc && "Swapped rpc wsa never assigned!");

	auto peer(std::make_shared<ZNetPeer>(std::move(swappedRpc), uuid, name));
	m_peers.push_back(peer);

	peer->m_pos = pos;

	rpc->Register("RefPos", this, &ZNet::RPC_RefPos);
	rpc->Register("CharacterID", this, &ZNet::RPC_CharacterID);
	rpc->Register("Kick", this, &ZNet::RPC_Kick);
	rpc->Register("Ban", this, &ZNet::RPC_Ban);
	rpc->Register("Unban", this, &ZNet::RPC_Unban);
	rpc->Register("Save", this, &ZNet::RPC_Save);
	rpc->Register("PrintBanned", this, &ZNet::RPC_PrintBanned);

	SendPeerInfo(rpc);

	//m_zdoMan->AddPeer(peer);
	//m_routedRpc->AddPeer(peer);
}

void ZNet::RPC_Disconnect(ZRpc* rpc) {
	LOG(INFO) << "RPC_Disconnect";
	auto&& peer = GetPeer(rpc);
	Disconnect(peer);
}

void ZNet::RPC_ServerHandshake(ZRpc* rpc) {
	//LOG(INFO) << "Client initiated handshake " << peer->m_socket->GetHostName();
	//this.ClearPlayerData(peer);
	bool flag = !Valhalla()->m_serverPassword.empty();
	std::string salt = "Im opposing salt"; // must be 16 bytes
	rpc->Invoke("ClientHandshake", flag, salt);
}

void ZNet::RPC_RefPos(ZRpc* rpc, Vector3 pos, bool publicRefPos) {
	auto&& peer = GetPeer(rpc);

	peer->m_pos = pos;
	peer->m_visibleOnMap = publicRefPos; // stupid name
}

void ZNet::RPC_CharacterID(ZRpc* rpc, ZDOID characterID) {
	auto &&peer = GetPeer(rpc);
	peer->m_characterID = characterID;

	LOG(INFO) << "Got character ZDOID from " << peer->m_name << " : " << characterID.ToString();
}



void ZNet::RPC_Kick(ZRpc* rpc, std::string user) {
	// check if rpc is admin first
	// if (!rpc.perm_admin...) return

	std::string msg = "Kicking user " + user;
	RemotePrint(rpc, msg);
	Kick(user);
}

void ZNet::RPC_Ban(ZRpc* rpc, std::string user) {
	std::string msg = "Banning user " + user;
	RemotePrint(rpc, msg);
	Ban(user);
}

void ZNet::RPC_Unban(ZRpc* rpc, std::string user) {
	std::string msg = "Unbanning user " + user;
	RemotePrint(rpc, msg);
	Unban(user);
}

void ZNet::RPC_Save(ZRpc* rpc) {

}

void ZNet::RPC_PrintBanned(ZRpc* rpc) {
	std::string s = "Banned users";
	//std::vector<std:

	RemotePrint(rpc, s);
}





void ZNet::Kick(const std::string &user) {
	Kick(GetPeer(user));
}

void ZNet::Kick(ZNetPeer::Ptr peer) {
	if (!peer)
		return;

	LOG(INFO) << "Kicking " << peer->m_name;

	SendDisconnect(peer);
	Disconnect(peer);
}

void ZNet::Ban(const std::string &user, std::chrono::minutes dur, const std::string &reason) {
	LOG(INFO) << "Banning " << user;
	
	auto now(std::chrono::steady_clock::now());
	auto expiry(now + dur);

	Valhalla()->m_banList.insert({ user,
		ban_info { now, expiry, reason} });

	//Ban(GetPeer(user));
}

/*
void ZNet::Ban(ZNetPeer::Ptr peer, std::chrono::minutes dur, std::string reason) {

	if (!peer)
		return;

	auto now(std::chrono::steady_clock::now());
	auto expiry(now + dur);

	m_banList.insert({peer->m_playerName, 
		ban_info { now, expiry, reason}});
}*/

void ZNet::Unban(const std::string &user) {
	LOG(INFO) << "Unbanning " << user;

	Valhalla()->m_banList.erase(user);
}

//void ZNet::Unban(ZNetPeer::Ptr peer) {
//
//}






void ZNet::SendPlayerList() {
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
			// where a packet has to be sent to multiple players
			// it ends up getting invalidated when its moved
			// just use shared ptr
			peer->m_rpc->Invoke("PlayerList", pkg);
		}
	}
}

void ZNet::SendNetTime() {
	for (auto&& peer : m_peers) {
		peer->m_rpc->Invoke("NetTime", m_netTime);
	}
}

void ZNet::RemotePrint(ZRpc* rpc, const std::string& s) {
	rpc->Invoke("RemotePrint", s);
}

void ZNet::SendDisconnect() {
	LOG(INFO) << "Sending disconnect msg";

	for (auto&& peer : m_peers) {
		SendDisconnect(peer);
	}
}

void ZNet::SendDisconnect(ZNetPeer::Ptr peer) {
	LOG(INFO) << "Disconnect sent to " << peer->m_rpc->m_socket->GetHostName();
	peer->m_rpc->Invoke("Disconnect");
}

void ZNet::Disconnect(ZNetPeer::Ptr peer) {
	peer->m_rpc->m_socket->Close();
}




ZNetPeer::Ptr ZNet::GetPeer(ZRpc* rpc) {
	for (auto&& peer : m_peers) {
		if (peer->m_rpc.get() == rpc)
			return peer;
	}
	return nullptr;
}

ZNetPeer::Ptr ZNet::GetPeer(const std::string &name) {
	for (auto&& peer : m_peers) {
		if (peer->m_name == name)
			return peer;
	}
	return nullptr;
}

ZNetPeer::Ptr ZNet::GetPeer(uuid_t uuid) {
	for (auto&& peer : m_peers) {
		if (peer->m_uuid == uuid)
			return peer;
	}
	return nullptr;
}



uuid_t ZNet::GetUID()
{
	return m_zdoMan->GetMyID();
}

