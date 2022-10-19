#pragma once

#include <string>
#include <memory>

#include "Utils.h"
#include "NetPackage.h"
#include "AsyncDeque.h"

#include <asio.hpp> // Where should this go?

#ifdef ENABLE_STEAM
#include <steam_api.h>
#include <isteamgameserver.h>
#include <steam_gameserver.h>
#include <isteamnetworkingutils.h>
#endif

class ISocket : public std::enable_shared_from_this<ISocket> {
public:
	using Ptr = std::shared_ptr<ISocket>;

	virtual ~ISocket() = default;

	virtual void Start() = 0;
	virtual void Close() = 0;

	virtual void Update() = 0;
	virtual void Send(NetPackage::Ptr packet) = 0;
	virtual NetPackage::Ptr Recv() = 0;

	virtual const std::string& GetHostName() const = 0;

	virtual bool Connected() const = 0;

	virtual int GetSendQueueSize() const = 0;
};

#ifdef ENABLE_STEAM
class SteamSocket : public ISocket {
private:
	std::deque<BYTES_t> m_sendQueue;
	std::deque<NetPackage::Ptr> m_recvQueue;

public:
	HSteamNetConnection m_con;
	SteamNetworkingIdentity m_peerID;

public:
	SteamSocket(HSteamNetConnection con);
	~SteamSocket();

	// Virtual
	void Start() override;
	void Close() override;
	
	void Update() override;
	void Send(NetPackage::Ptr packet) override;
	NetPackage::Ptr Recv() override;

	const std::string& GetHostName() const override {
		return "";
	}
	bool Connected() const override {
		return m_con != k_HSteamNetConnection_Invalid;
	}
	int GetSendQueueSize() const override {
		return 0;
	}

	// Declared
	// Flush is used only once the socket is closed
	//void Flush();

	void SendQueued();

};
#endif // ENABLE_STEAM

/**
	* @brief Custom socket implementation which makes use of Asio tcp socket
	*
*/
// Shared ptr not really necessary with a simple 
// client where socket state is tracked step by step
class ZSocket2 : public ISocket {
	asio::ip::tcp::socket m_socket;

	// Reusable vec pool
	AsyncDeque<BYTES_t> m_pool;

	AsyncDeque<BYTES_t> m_sendQueue;
	AsyncDeque<NetPackage::Ptr> m_recvQueue;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;
	std::atomic_int m_sendQueueSize = 0;

	std::string m_hostname;
	uint16_t m_port;

	std::atomic_bool m_connected = true;

	//std::atomic<Connectivity> m_connectivity;

public:
	ZSocket2(asio::ip::tcp::socket sock);
	~ZSocket2();

	// Virtual
	void Start() override;
	void Close() override;

	void Update() override;
	void Send(NetPackage::Ptr packet) override;
	NetPackage::Ptr Recv() override;

	const std::string& GetHostName() const override {
		return m_hostname;
	}
	bool Connected() const override {
		return m_connected;
	}
	int GetSendQueueSize() const override {
		return m_sendQueueSize;
	}

	// Declared
	auto&& GetSocket() {
		return m_socket;
	}

private:
	void ReadPkgSize();
	void ReadPkg();
	void WritePkgSize();
	void WritePkg(const std::vector<byte_t>& buf);
};
