#include "NetAcceptor.h"
#include "NetManager.h"

AcceptorTCP::AcceptorTCP()
	: m_ctx(),
		m_acceptor(m_ctx,
		asio::ip::tcp::endpoint(asio::ip::tcp::v4(), VH_SETTINGS.serverPort)) {}

AcceptorTCP::~AcceptorTCP() {
	this->m_ctx.stop();
}

void AcceptorTCP::Listen() {
	DoAccept();

	StartThread();
}

void AcceptorTCP::StartThread() {
	m_thread = std::jthread(
		[this](std::stop_token token) {
		//if (token.stop_requested())
			this->m_ctx.run();
			//NetManager()->m_ctx.run();
		}
	);
}

ISocket::Ptr AcceptorTCP::Accept() {
	std::scoped_lock scoped(m_mux);

	if (!m_acceptedQueue.empty()) {
		TCPSocket::Ptr socket = std::move(m_acceptedQueue.front());
		m_acceptedQueue.pop_front();
		socket->ReadPkgSize();
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
				auto&& ptr = std::make_shared<TCPSocket>(std::move(socket));
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