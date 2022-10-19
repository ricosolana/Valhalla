#pragma once

#include <string>
#include <memory>

#include <steam_api.h>
#include <isteamgameserver.h>
#include <steam_gameserver.h>
#include <isteamnetworkingutils.h>
#include "NetPackage.h"

#include "Utils.h"

#include <asio.hpp>

using namespace asio::ip;

enum class Connectivity {
	CONNECTING,
	CONNECTED,
	CLOSED
};

class ISocket : public std::enable_shared_from_this<ISocket> {
public:
	using Ptr = std::shared_ptr<ISocket>;

	virtual ~ISocket() = default;

	virtual void Start() = 0;
	virtual void Close() = 0;
	virtual void Update() = 0;

	void Send(NetPackage::Ptr packet);
	void // update method to reengage the writers
	virtual NetPackage::Ptr Recv() = 0;
	virtual bool HasNewData() = 0;

	virtual std::string& GetHostName() = 0;
	virtual uint16_t GetHostPort() = 0;
	virtual Connectivity GetConnectivity() = 0;

	virtual int GetSendQueueSize() = 0;
};

class SteamSocket : public ISocket {
private:
	HSteamNetConnection m_con;
	SteamNetworkingIdentity m_peerID;

	std::deque<bytes_t> m_sendQueue;
	std::deque<NetPackage::Ptr> m_recvQueue;

public:
	SteamSocket(HSteamNetConnection con);
	~SteamSocket();

	// Virtual
	void Start() override;
	void Close() override;

	void Send(NetPackage::Ptr packet) override;
	NetPackage::Ptr Recv() override;
	bool HasNewData() override;

	std::string& GetHostName() override;
	uint16_t GetHostPort() override;
	Connectivity GetConnectivity() override;
	int GetSendQueueSize() override;

	// Declared
	// Flush is used only once the socket is closed
	//void Flush();


};

/**
	* @brief Custom socket implementation which makes use of Asio tcp socket
	*
*/
// Shared ptr not really necessary with a simple 
// client where socket state is tracked step by step
class ZSocket2 : public ISocket {
	tcp::socket m_socket;

	AsyncDeque<bytes_t> m_sendQueue;
	AsyncDeque<NetPackage::Ptr> m_recvQueue;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;
	std::atomic_int m_sendQueueSize = 0;

	std::string m_hostname;
	uint16_t m_port;

	std::atomic<Connectivity> m_connectivity;

public:
	ZSocket2(tcp::socket sock);
	~ZSocket2();

	// Virtual
	void Start() override;
	void Close() override;

	NetPackage::Ptr Recv() override;
	bool HasNewData() override;

	std::string& GetHostName() override;
	uint16_t GetHostPort() override;
	Connectivity GetConnectivity() override;
	int GetSendQueueSize() override;

	// Declared
	tcp::socket& GetSocket();

private:
	void ReadPkgSize();
	void ReadPkg();
	void WritePkgSize();
	void WritePkg(const std::vector<byte_t>& buf);
};
