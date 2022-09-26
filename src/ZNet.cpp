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
		auto&& peer = std::make_unique<ZNetPeer>(m_acceptor->Accept());

		peer->m_rpc->Register("PeerInfo", new ZMethod(this, &ZNet::RPC_PeerInfo));
		peer->m_rpc->Register("Disconnect", new ZMethod(this, &ZNet::RPC_Disconnect));
		peer->m_rpc->Register("ServerHandshake", new ZMethod(this, &ZNet::RPC_ServerHandshake));

		peer->m_socket->Start();

		m_peers.push_back(std::move(peer));
	}

	// Remove stale peers
	auto&& itr = m_peers.begin();
	while (itr != m_peers.end()) {
		if (!(*itr)->m_socket->IsConnected()) {
			itr = m_peers.erase(itr);
		}
		else {
			++itr;
		}
	}

	// Updaate rpcs
	for (auto&& peer : m_peers) {
		peer->m_rpc->Update();
	}
}

void ZNet::RPC_ServerHandshake(ZRpc* rpc) {
	auto&& peer = GetPeer(rpc);
	assert(peer);

	LOG(INFO) << "Client initiated handshake " << peer->m_socket->GetHostName();
	//this.ClearPlayerData(peer);
	bool flag = !Valhalla()->m_serverPassword.empty();
	peer->m_rpc->Invoke("ClientHandshake", flag);
}

void ZNet::SendPeerInfo(ZRpc* rpc) {
	ZPackage pkg;
	pkg.Write(GetUID());
	pkg.Write(ValhallaServer::VERSION);
	pkg.Write(Vector3()); // dummy
	pkg.Write("Stranger"); // valheim uses this, which is dumb

	// why does a server need to send a position and name?
	// clearly someone didnt think of the protocol
	
	pkg.Write(m_world.m_name);
	pkg.Write(m_world.m_seed);
	pkg.Write(m_world.m_seedName);
	pkg.Write(m_world.m_uid);
	pkg.Write(m_world.m_worldGenVersion);
	pkg.Write(m_netTime);

	rpc->Invoke("PeerInfo", std::move(pkg));
}

// The server send peer info once the client is completely validated
void ZNet::RPC_PeerInfo(ZRpc* rpc, ZPackage pkg) {
	auto&& peer = GetPeer(rpc);

	auto hostName = peer->m_socket->GetHostName();

	auto refUid = pkg.Read<UID_t>();
	auto refVer = pkg.Read<std::string>();
	LOG(INFO) << "Connecting client has version " << refVer;
	if (refVer != std::string(ValhallaServer::VERSION)) {
		rpc->Invoke("Error", ConnectionStatus::ErrorVersion);
		// disconnect client later as a cleanup
		LOG(INFO) << "Client " << hostName << " has an incompatible version";
		return;
	}
	auto&& refPos = pkg.Read<Vector3>();
	auto&& refName = pkg.Read<std::string>();

	auto&& refPassword = pkg.Read<std::string>();
	if (refPassword != Valhalla()->m_serverPassword) {
		rpc->Invoke("Error", ConnectionStatus::ErrorPassword);
		// disconnect client later as a cleanup
		LOG(INFO) << "Client " << hostName << " provided the wrong password";
		return;
	}

	// check if player uid is already connected
	peer->m_refPos = refPos;
	peer->m_uid = refUid;
	peer->m_playerName = refName;

	//rpc.Register<Vector3, bool>("RefPos", new Action<ZRpc, Vector3, bool>(this.RPC_RefPos));
	//rpc.Register<ZPackage>("PlayerList", new Action<ZRpc, ZPackage>(this.RPC_PlayerList));
	//rpc.Register<string>("RemotePrint", new Action<ZRpc, string>(this.RPC_RemotePrint));

	//rpc.Register<ZDOID>("CharacterID", new Action<ZRpc, ZDOID>(this.RPC_CharacterID));
	//rpc.Register<string>("Kick", new Action<ZRpc, string>(this.RPC_Kick));
	//rpc.Register<string>("Ban", new Action<ZRpc, string>(this.RPC_Ban));
	//rpc.Register<string>("Unban", new Action<ZRpc, string>(this.RPC_Unban));
	//rpc.Register("Save", new ZRpc.RpcMethod.Method(this.RPC_Save));
	//rpc.Register("PrintBanned", new ZRpc.RpcMethod.Method(this.RPC_PrintBanned));

	SendPeerInfo(rpc);
	//SendPlayerList();



	// check if player is banned

	//rpc->Register("RefPos", new ZMethod(this, &ZNet::RPC_RefPos));
	//rpc->Register("PlayerList", new ZMethod(this, &ZNet::RPC_PlayerList));
	//rpc->Register("RemotePrint", new ZMethod(this, &ZNet::RPC_RemotePrint));
	
	//rpc->Register("NetTime", new ZMethod(this, &ZNet::RPC_NetTime));
	
}

void ZNet::RPC_Disconnect(ZRpc *rpc) {
	LOG(INFO) << "RPC_Disconnect";
	//Disconnect();
}

ZNetPeer *ZNet::GetPeer(ZRpc* rpc) {
	for (auto&& peer : m_peers) {
		if (peer->m_rpc.get() == rpc)
			return peer.get();
	}
	return nullptr;
}

int64_t ZNet::GetUID()
{
	return m_zdoMan->GetMyID();
}

