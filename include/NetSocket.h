#pragma once

#include <string>
#include <memory>
#include <optional>

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

	// Begin socket readers
	virtual void Start() = 0;

	// Terminates the connection
	virtual void Close() = 0;

	// Call every tick to reengage writers
	virtual void Update() = 0;
	// Send a packet to the remote host
	virtual void Send(const NetPackage& pkg) = 0;
	// Receive a packet from the remote host
	virtual std::optional<NetPackage> Recv() = 0;

	// Get the name of this connection
	virtual std::string GetHostName() const = 0;

	// Returns true if the socket is alive
	virtual bool Connected() const = 0;

	// Returns the size in bytes of packets queued for sending
	// Returns -1 on failure
	virtual int GetSendQueueSize() const = 0;
};

class SteamSocket : public ISocket {
private:
	std::deque<BYTES_t> m_sendQueue;
	std::deque<NetPackage> m_recvQueue;

public:
	HSteamNetConnection m_handle;
	SteamNetworkingIdentity m_steamNetId;

public:
	SteamSocket(HSteamNetConnection con);
	~SteamSocket();

	// Virtual
	void Start() override;
	void Close() override;
	
	void Update() override;
	void Send(const NetPackage &pkg) override;
	std::optional<NetPackage> Recv() override;

	std::string GetHostName() const override {
		return std::to_string(m_steamNetId.GetSteamID64());
	}

	bool Connected() const override {
		return m_handle != k_HSteamNetConnection_Invalid;
	}

	int GetSendQueueSize() const override;

private:
	void SendQueued();

};
