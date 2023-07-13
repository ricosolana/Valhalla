#include "NetAcceptor.h"

AcceptorTCP::AcceptorTCP()
	: m_ctx(),
	m_acceptor(m_ctx,
		asio::ip::tcp::endpoint(asio::ip::tcp::v4(), VH_SETTINGS.serverPort)) {}

AcceptorTCP::~AcceptorTCP() {
	this->m_ctx.stop();
}

void AcceptorTCP::Listen() {
	DoAccept();

	m_thread = std::thread(
		[this]() {
			this->m_ctx.run();
		}
	);
}

ISocket::Ptr AcceptorTCP::Accept() {
	std::scoped_lock scoped(m_mux);

	if (!m_acceptedQueue.empty()) {
		NetSocket::Ptr socket = std::move(m_acceptedQueue.front());
		m_acceptedQueue.pop_front();
		return socket;
	}

	//auto&& begin = m_acceptedQueue.begin();
	//if (begin != m_acceptedQueue.end()) {
	//	auto&& ptr = std::move(*begin);
	//	m_acceptedQueue.erase(begin);
	//	return ptr;
	//}

	return nullptr;
}

void AcceptorTCP::DoAccept() {
	m_acceptor.async_accept(
		[this](const asio::error_code& ec, asio::ip::tcp::socket socket) {
			if (!ec) {
				auto&& ptr = std::make_shared<NetSocket>(std::move(socket));
				std::scoped_lock scoped(m_mux);
				m_acceptedQueue.push_back(std::move(ptr));
			}
			else {
				if (ec.value() == asio::error::operation_aborted) {
					LOG_INFO(LOGGER, "NetAccepter aborted");
					return;
				}
				else {
					LOG_ERROR(LOGGER, "Failed to accept: "); // << ec.message() << " : " << ec.value();
				}
			}

			DoAccept();
		}
	);
}