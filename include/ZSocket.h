#pragma once

#include <string>
#include "ZPackage.h"

#include "Utils.h"
#include <memory>

#include <isteamnetworkingutils.h>
#include <steam_gameserver.h>
#include <isteamutils.h>

#include <asio.hpp>

using namespace asio::ip;

class ISocket : public std::enable_shared_from_this<ISocket> {
public:
	using Ptr = std::shared_ptr<ISocket>;

	virtual ~ISocket() = default;

	virtual void Start() = 0;
	virtual void Close() = 0;

	virtual void Send(ZPackage packet) = 0;
	virtual ZPackage Recv() = 0;
	virtual bool HasNewData() = 0;

	virtual std::string& GetHostName() = 0;
	virtual uint16_t GetHostPort() = 0;
	virtual bool IsConnected() = 0;
};





//class SteamSocket : public ISocket {
//public:
//	//void onStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);
//
//	// https://forums.unrealengine.com/t/implementing-steam-callback/55272/6
//	STEAM_GAMESERVER_CALLBACK(SteamSocket, onStatusChanged, SteamNetConnectionStatusChangedCallback_t);
//};


/**
	* @brief Custom socket implementation which makes use of Asio tcp socket
	*
*/
// Shared ptr not really necessary with a simple 
// client where socket state is tracked step by step
class ZSocket2 : public ISocket {
	tcp::socket m_socket;

	AsyncDeque<ZPackage> m_sendQueue;
	AsyncDeque<ZPackage> m_recvQueue;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;

	ZPackage m_recv;

	std::string m_hostname;
	uint16_t m_port;

	std::atomic_bool m_connected = true;

public:
	ZSocket2(tcp::socket sock);
	~ZSocket2();

	// Virtual
	void Start() override;
	void Close() override;

	void Send(ZPackage packet) override;
	ZPackage Recv() override;
	bool HasNewData() override;

	std::string& GetHostName() override;
	uint16_t GetHostPort() override;
	bool IsConnected() override;

	// Declared
	tcp::socket& GetSocket();

private:
	void ReadPkgSize();
	void ReadPkg();
	void WritePkgSize();
	void WritePkg(ZPackage &pkg);
};
