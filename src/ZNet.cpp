#include "ZNet.h"
#include "ValhallaServer.h"
#include "ScriptManager.h"
#include <openssl/md5.h>
#include "ValhallaServer.h"

using namespace asio::ip;

ZNet::ZNet(uint16_t port) 
	: m_ctx() {

	m_acceptor = std::make_unique<AcceptorZSocket2>(m_ctx, port);
}

void ZNet::Listen() {
	LOG(INFO) << "Starting server";

	UUID uid(1234567891011);

	m_routedRpc = std::make_unique<ZRoutedRpc>(uid);
	m_zdoMan = std::make_unique<ZDOMan>(uid);
	m_world = std::make_unique<World>();

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

		REGISTER_RPC(rpc, "PeerInfo", ZNet::RPC_PeerInfo);
		REGISTER_RPC(rpc, "Disconnect", ZNet::RPC_Disconnect); // client only
		REGISTER_RPC(rpc, "ServerHandshake", ZNet::RPC_ServerHandshake);

		rpc->m_socket->Start();

		m_joining.push_back(std::move(rpc));
	}
	
	{
		// Remove stale joining
		// Allowed to have empty references
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
		peer->m_rpc->Update();
	}

	// Update joining
	//	this is done after peers rpc update because in the weird case
	//	where a joining becomes a peer then is processed twice in an update tick
	for (auto&& rpc : m_joining) {
		rpc->Update();
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

// The server send peer info once the client is completely validated
void ZNet::RPC_PeerInfo(ZRpc* rpc, ZPackage::Ptr pkg) {
	auto &&hostName = rpc->m_socket->GetHostName();

	auto refUid = pkg->Read<UUID>();
	auto refVer = pkg->Read<std::string>();
	LOG(INFO) << "Client " << hostName << " has version " << refVer;
	if (refVer != std::string(ValhallaServer::VERSION)) {
		rpc->Invoke("Error", ConnectionStatus::ErrorVersion);
		// disconnect client later as a cleanup
		LOG(INFO) << "Client version is incompatible";
		return;
	}
	auto&& refPos = pkg->Read<Vector3>();
	auto&& refName = pkg->Read<std::string>();

	auto&& refPassword = pkg->Read<std::string>();
	if (refPassword != Valhalla()->m_serverPassword) {
		rpc->Invoke("Error", ConnectionStatus::ErrorPassword);
		// disconnect client later as a cleanup
		LOG(INFO) << "Client password is incorrect";
		return;
	}

	// Find the rpc and transfer
	std::unique_ptr<ZRpc> swapped;
	for (auto&& j : m_joining) {
		if (j.get() == rpc) {
			swapped = std::move(j);
			break;
		}
	}
	assert(swapped && "Swapped rpc wsa never assigned!");

	auto peer(std::make_shared<ZNetPeer>(std::move(swapped)));
	m_peers.push_back(peer);

	// check if player uid is already connected
	peer->m_refPos = refPos;
	peer->m_uid = refUid;
	peer->m_playerName = refName;

	REGISTER_RPC(rpc, "RefPos", ZNet::RPC_RefPos);
	//REGISTER_RPC(rpc, "PlayerList", ) // used by client 
	//REGISTER_RPC(rpc, "RemotePrint") // used by client

	//rpc.Register<Vector3, bool>("RefPos", new Action<ZRpc, Vector3, bool>(this.RPC_RefPos));
	//rpc.Register<ZPackage>("PlayerList", new Action<ZRpc, ZPackage>(this.RPC_PlayerList));
	//rpc.Register<string>("RemotePrint", new Action<ZRpc, string>(this.RPC_RemotePrint));

	REGISTER_RPC(rpc, "CharacterID", ZNet::RPC_CharacterID);
	REGISTER_RPC(rpc, "Kick", ZNet::RPC_Kick);
	REGISTER_RPC(rpc, "Ban", ZNet::RPC_Ban);
	REGISTER_RPC(rpc, "Unban", ZNet::RPC_Unban);
	REGISTER_RPC(rpc, "Save", ZNet::RPC_Save);
	REGISTER_RPC(rpc, "PrintBanned", ZNet::RPC_PrintBanned);

	//rpc.Register<ZDOID>("CharacterID", new Action<ZRpc, ZDOID>(this.RPC_CharacterID));
	//rpc.Register<string>("Kick", new Action<ZRpc, string>(this.RPC_Kick));
	//rpc.Register<string>("Ban", new Action<ZRpc, string>(this.RPC_Ban));
	//rpc.Register<string>("Unban", new Action<ZRpc, string>(this.RPC_Unban));
	//rpc.Register("Save", new ZRpc.RpcMethod.Method(this.RPC_Save));
	//rpc.Register("PrintBanned", new ZRpc.RpcMethod.Method(this.RPC_PrintBanned));

	SendPeerInfo(rpc);
	SendPlayerList();



	// check if player is banned

	//rpc->Register("RefPos", new ZMethod(this, &ZNet::RPC_RefPos));
	//rpc->Register("PlayerList", new ZMethod(this, &ZNet::RPC_PlayerList));
	//rpc->Register("RemotePrint", new ZMethod(this, &ZNet::RPC_RemotePrint));

	//rpc->Register("NetTime", new ZMethod(this, &ZNet::RPC_NetTime));

}

//void ZNet::RPC_Disconnect(ZRpc* rpc) {
//	LOG(INFO) << "RPC_Disconnect";
//	//Disconnect();
//}

void ZNet::RPC_ServerHandshake(ZRpc* rpc) {
	//LOG(INFO) << "Client initiated handshake " << peer->m_socket->GetHostName();
	//this.ClearPlayerData(peer);
	bool flag = !Valhalla()->m_serverPassword.empty();
	std::string salt = "Im opposing salt"; // must be 16 bytes
	rpc->Invoke("ClientHandshake", flag, salt);
}

void ZNet::RPC_RefPos(ZRpc* rpc, Vector3 pos, bool publicRefPos) {
	auto&& peer = GetPeer(rpc);

	peer->m_refPos = pos;
	peer->m_publicRefPos = publicRefPos;
}

void ZNet::RPC_CharacterID(ZRpc* rpc, ZDOID characterID) {
	auto &&peer = GetPeer(rpc);
	peer->m_characterID = characterID;

	LOG(INFO) << "Got character ZDOID from " << peer->m_playerName << " : " << characterID.ToString();
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

}





void ZNet::Kick(std::string user) {

}

void ZNet::Kick(ZNetPeer::Ptr peer) {
	LOG(INFO) << "Kicking " << peer->m_playerName;

	SendDisconnect(peer);
	Disconnect(peer);
}

void ZNet::Ban(std::string user) {

}

void ZNet::Ban(ZNetPeer::Ptr peer) {

}

void ZNet::Unban(std::string user) {

}

void ZNet::Unban(ZNetPeer::Ptr peer) {

}






void ZNet::SendPlayerList() {
	if (!m_peers.empty()) {
		auto pkg(PKG());
		pkg->Write((int)m_peers.size());

		for (auto&& peer : m_peers) {
			pkg->Write(peer->m_playerName);
			pkg->Write(peer->m_rpc->m_socket->GetHostName());
			pkg->Write(peer->m_characterID);
			pkg->Write(peer->m_publicRefPos);
			if (peer->m_publicRefPos) {
				pkg->Write(peer->m_refPos);
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

void ZNet::RemotePrint(ZRpc* rpc, std::string& s) {
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





ZNetPeer::Ptr ZNet::GetPeer(ZRpc* rpc) {
	for (auto&& peer : m_peers) {
		if (peer->m_rpc.get() == rpc)
			return peer;
	}
	return nullptr;
}

int64_t ZNet::GetUID()
{
	return m_zdoMan->GetMyID();
}

