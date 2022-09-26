#pragma once

#include <memory>
#include <thread>

#include "ZSocket.h"

class IAcceptor {
public:
	virtual ~IAcceptor() = default;

	virtual void Start() = 0;
	virtual void Close() = 0;

	virtual std::shared_ptr<ISocket> Accept() = 0;
	virtual bool HasNewConnection() = 0;
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
	bool HasNewConnection() override;

private:
	void DoAccept();
};
