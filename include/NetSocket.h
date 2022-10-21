#pragma once

#include <string>
#include <memory>

#include "Utils.h"
#include "NetPackage.h"
#include "AsyncDeque.h"

#include <steam_api.h>
#include <isteamgameserver.h>
#include <steam_gameserver.h>
#include <isteamnetworkingutils.h>

class ISocket : public std::enable_shared_from_this<ISocket> {
public:
	using Ptr = std::shared_ptr<ISocket>;

	virtual ~ISocket() = default;

	virtual void Start() = 0;
	virtual void Close() = 0;

	virtual void Update() = 0;
	virtual void Send(NetPackage::Ptr packet) = 0;
	virtual NetPackage::Ptr Recv() = 0;

	virtual std::string GetHostName() const = 0;

	virtual bool Connected() const = 0;

	virtual int GetSendQueueSize() const = 0;
};

class SteamSocket : public ISocket {
private:
	std::deque<BYTES_t> m_sendQueue;
	std::deque<NetPackage::Ptr> m_recvQueue;

public:
	HSteamNetConnection m_steamNetCon;
	SteamNetworkingIdentity m_steamNetId;

public:
	SteamSocket(HSteamNetConnection con);
	~SteamSocket();

	// Virtual
	void Start() override;
	void Close() override;
	
	void Update() override;
	void Send(NetPackage::Ptr packet) override;
	NetPackage::Ptr Recv() override;

	std::string GetHostName() const override {
		//SteamNetworkingUtils()->GetIPv4FakeIPType
		//m_steamNetId.GetSteamID()
		return std::string("type: ") + std::to_string(m_steamNetId.m_eType); // .m_eTypeGetIPv4()GetIPAddr()->st;
	}
	bool Connected() const override {
		return m_steamNetCon != k_HSteamNetConnection_Invalid;
	}
	int GetSendQueueSize() const override {
		return 0;
	}

private:
	void SendQueued();

};
