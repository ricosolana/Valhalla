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
	virtual ~ISocket() = default;

	virtual void Accept() = 0;
	virtual void Send(ZPackage packet) = 0;

	virtual bool HasNewData() = 0;
	virtual ZPackage Recv() = 0;
	virtual bool Close() = 0;

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

	// If unique ptr is used, the small ptr only is copied
	// but a deallocation will always take place for dead Packages
	// Should use smart ptr anyways to avoid memory leaks
	// ---
	// If ZPackage ref-value is used, the Deque is reallocated to fit the thing
	// A 
	// TODO use unique ptr
	AsyncDeque<ZPackage> m_sendQueue;

	// TODO use unique_ptr
	AsyncDeque<ZPackage> m_recvQueue;

	//std::list<ZPackage> m_list;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;

	ZPackage m_recv;

	std::string m_hostname;
	uint16_t m_port;

	//
	std::atomic_bool m_online = true;

public:
	using Ptr = std::shared_ptr<ZSocket2>;

	ZSocket2(asio::io_context& ctx);
	~ZSocket2();

	// Virtual overrides
	void Accept();
	void Send(ZPackage packet);

	bool HasNewData();
	ZPackage Recv();
	bool Close();

	std::string& GetHostName() override;
	uint16_t GetHostPort() override;
	bool IsConnected() override;

	// ZSocket unique
	tcp::socket& GetSocket();

private:
	void ReadPkgSize();
	void ReadPkg();
	void WritePkgSize();
	void WritePkg(ZPackage &pkg);
};
