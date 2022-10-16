#pragma once

#include "NetSocket.h"
#include "NetRpc.h"
#include "NetSync.h"
#include "Utils.h"

struct NetPeer {
public:
	using Ptr = std::shared_ptr<NetPeer>;

	std::unique_ptr<NetRpc> m_rpc;

	const uuid_t m_uuid;
	const std::string m_name;

	Vector3 m_pos;
	bool m_visibleOnMap = false;
	NetSync::ID m_characterID = NetSync::ID::NONE;

public:
	NetPeer(std::unique_ptr<NetRpc> rpc,
		uuid_t uuid, const std::string &name)
		: m_rpc(std::move(rpc)), m_name(name), m_uuid(uuid) 
	{}

	void Kick();
	void SendDisconnect();
	void Disconnect();
};
