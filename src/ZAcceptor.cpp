#include "NetAcceptor.h"
#include "ValhallaServer.h"

AcceptorZSocket2::AcceptorZSocket2(asio::io_context& ctx, asio::ip::port_type port)
	: m_ctx(ctx), m_acceptor(ctx, tcp::endpoint(tcp::v4(), port)) {
	m_accepting = false;
}

AcceptorZSocket2::~AcceptorZSocket2() {
	Close();
}

void AcceptorZSocket2::Start() {
	assert(!m_accepting, "Tried starting ZAccepter twice!");

	LOG(INFO) << "Starting server on port " << m_acceptor.local_endpoint().port();

	DoAccept();

	m_ctxThread = std::thread([this]() {
		el::Helpers::setThreadName("io");
		m_ctx.run();
	});

	m_accepting = true;
}

void AcceptorZSocket2::Close() {
	assert(m_accepting && "Tried closing ZAccepter while inactive");

	m_accepting = false;
	m_acceptor.close();

	assert(std::this_thread::get_id() !=
		m_ctxThread.get_id() && "Tried closing ZAccepter from same thread (gridlock)");

	if (m_ctxThread.joinable())
		m_ctxThread.join();
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
				if (ec.value() == asio::error::operation_aborted) {
					LOG(INFO) << "ZAccepter aborted";
					return;
				}
				else {
					LOG(ERROR) << "Failed to accept: " << ec.message() << " : " << ec.value();
				}
			}

			DoAccept();
		});
}
