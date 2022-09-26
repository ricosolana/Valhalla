#include "ZAcceptor.h"
#include "ValhallaServer.h"

AcceptorZSocket2::AcceptorZSocket2(asio::io_context& ctx, asio::ip::port_type port)
	: m_ctx(ctx), m_acceptor(ctx, tcp::endpoint(tcp::v4(), port)) {}

AcceptorZSocket2::~AcceptorZSocket2() {
	Close();
}

void AcceptorZSocket2::Start() {
	LOG(INFO) << "Starting server on port " << m_acceptor.local_endpoint().port();

	DoAccept();

	m_ctxThread = std::thread([this]() {
		el::Helpers::setThreadName("io");
		m_ctx.run();
	});

#if defined(_WIN32)// && !defined(_NDEBUG)
	void* pThr = m_ctxThread.native_handle();
	SetThreadDescription(pThr, L"IO Thread");
#endif
}

void AcceptorZSocket2::Close() {
	if (m_accepting) {
		m_accepting = false;
		m_acceptor.close();
	}
}

std::shared_ptr<ISocket> AcceptorZSocket2::Accept() {
	return m_awaiting.pop_front();
}

bool AcceptorZSocket2::HasNewConnection() {
	return !m_awaiting.empty();
}



void AcceptorZSocket2::DoAccept() {
	m_acceptor.async_accept(
		[this](const asio::error_code& ec, tcp::socket socket) {

			if (!ec) {
				m_awaiting.push_back(std::make_shared<ZSocket2>(std::move(socket)));
			}
			else {
				LOG(ERROR) << "Failed to accept: " << ec.message();
			}

			DoAccept();
		});
}
