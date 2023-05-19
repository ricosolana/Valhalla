#include "NetAcceptor.h"

NetAcceptor::NetAcceptor()
	: m_ctx(), 
	m_acceptor(m_ctx, 
		asio::ip::tcp::endpoint(asio::ip::tcp::v4(), VH_SETTINGS.serverPort)) {
	
}

NetAcceptor::~NetAcceptor() {
	this->m_ctx.stop();
}

void NetAcceptor::Listen() {
	// 24 bytes
	//static constexpr auto y = sizeof(std::jthread)

	//static constexpr auto y1 = sizeof(std::thread)

	m_thread = std::thread([this]() {
		this->m_ctx.run();
	});
}

NetSocket::Ptr NetAcceptor::Accept() {
	std::scoped_lock scoped(m_mux);

	auto&& begin = m_acceptedQueue.begin();
	if (begin != m_acceptedQueue.end()) {
		auto&& ptr = std::move(*begin);
		m_acceptedQueue.erase(begin);
		return ptr;
	}

	return nullptr;
}

void NetAcceptor::DoAccept() {
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
		});
}
