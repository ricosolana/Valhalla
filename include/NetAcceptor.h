#pragma once

#include <memory>
#include <thread>

#include "NetSocket.h"

class IAcceptor {
public:
	virtual ~IAcceptor() = default;

	virtual void Start() = 0;
	virtual void Close() = 0;

	virtual ISocket::Ptr Accept() = 0;
};

class AcceptorSteam : public IAcceptor {
private:
	std::atomic_bool m_accepting;
	uint16_t m_port;
	HSteamListenSocket m_listenSocket;

	robin_hood::unordered_map<HSteamNetConnection, std::shared_ptr<SteamSocket>> m_connecting;

	std::deque<std::shared_ptr<SteamSocket>> m_awaiting;

public:
	AcceptorSteam(uint16_t port);
	~AcceptorSteam();

	void Start() override;
	void Close() override;

	ISocket::Ptr Accept() override;

private:
	// https://partner.steamgames.com/doc/sdk/api#callbacks
	STEAM_CALLBACK(AcceptorSteam, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);

};

class AcceptorZSocket2 : public IAcceptor {
private:
	asio::io_context& m_ctx;
	std::thread m_ctxThread;
	asio::ip::tcp::acceptor m_acceptor;

	std::atomic_bool m_accepting;

	AsyncDeque<std::shared_ptr<ZSocket2>> m_awaiting;

public:
	AcceptorZSocket2(asio::io_context &ctx, asio::ip::port_type port);
	~AcceptorZSocket2();

	void Start() override;
	void Close() override;

	std::shared_ptr<ISocket> Accept() override;
private:
	void DoAccept();
};
