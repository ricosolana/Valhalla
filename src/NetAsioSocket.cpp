#include "NetSocket.hpp"
#include <string>
#include <iostream>

namespace Alchyme {
	namespace Net {
		AsioSocket::AsioSocket(asio::io_context& ctx,
			asio::ip::tcp::socket socket)
			: m_socket(std::move(socket)) {}

		AsioSocket::AsioSocket(asio::io_context& ctx)
			: m_socket(ctx) {}

		AsioSocket::~AsioSocket() {
			LOG(DEBUG) << "~AsioSocket()";
			Close();
		}



		void AsioSocket::Accept() {
			LOG(DEBUG) << "AsioSocket::Accept()";

			m_hostname = m_socket.remote_endpoint().address().to_string();
			m_port = m_socket.remote_endpoint().port();
			ReadHeader();
		}



		bool AsioSocket::QueuePacket(NOTNULL Packet* packet) {
			if (m_online) {
				m_sendQueue.push_back(packet);
				return true;
			}
			return false;
		}

		bool AsioSocket::FlushPacket(NOTNULL Packet* packet) {
			if (m_online) {
				const bool was_empty = m_sendQueue.empty();
				m_sendQueue.push_back(packet);
				if (was_empty) {
					LOG(DEBUG) << "Reinitiating Writer";
					WriteHeader();
					return true;
				}
			}
			return false;
		}

		bool AsioSocket::Flush() {
			if (m_online && m_sendQueue.empty()) {
				LOG(DEBUG) << "Reinitiating Writer";
				WriteHeader();
				return true;
			}
			return false;
		}

		NULLABLE Packet* AsioSocket::NextPacket() {
			if (m_recvQueue.empty())
				return nullptr;
			return m_recvQueue.pop_front();
		}



		bool AsioSocket::Close() {
			if (m_online) {
				LOG(DEBUG) << "AsioSocket::Close()";

				m_online = false;

				m_socket.close();

				return true;
			}

			return false;
		}



		std::string& AsioSocket::GetHostName() {
			return m_hostname;
		}

		uint_least16_t AsioSocket::GetHostPort() {
			return m_port;
		}

		bool AsioSocket::IsOnline() {
			return m_online;
		}

		tcp::socket& AsioSocket::GetSocket() {
			return m_socket;
		}



		void AsioSocket::ReadHeader() {
			LOG(DEBUG) << "ReadHeader()";

			Packet* temp = new Packet();

			auto self(shared_from_this());
			asio::async_read(m_socket,
				asio::buffer(&temp->offset, sizeof(Packet::offset)),
				[this, self, temp](const std::error_code& e, size_t) {
				if (!e) {
					ReadBody(temp);
				}
				else {
					LOG(DEBUG) << "read header error: " << e.message() << " (" << e.value() << ")";
					Close();
				}
			}
			);
		}

		void AsioSocket::ReadBody(Packet* packet) {
			LOG(DEBUG) << "ReadBody()";

			if (packet->offset == 0) {
				Close();
			}
			else {
				packet->m_buf.resize(packet->offset);
				auto self(shared_from_this());
				asio::async_read(m_socket,
					asio::buffer(packet->m_buf),
					[this, self, packet](const std::error_code& e, size_t) {
					if (!e) {
						packet->offset = 0;
						m_recvQueue.push_back(packet);
						ReadHeader();
					}
					else {
						LOG(DEBUG) << "read body error: " << e.message() << " (" << e.value() << ")";
						Close();
					}
				});
			}
		}

		void AsioSocket::WriteHeader() {
			LOG(DEBUG) << "WriteHeader()";

			assert(m_sendQueue.front()->offset > 0 && "Offset must be greater than 0");

			Packet* packet = m_sendQueue.front();

			auto self(shared_from_this());
			asio::async_write(m_socket,
				asio::buffer(&packet->offset, sizeof(Packet::offset)),
				[this, self, packet](const std::error_code& e, size_t) {
				if (!e) {
					WriteBody(packet);
				}
				else {
					LOG(DEBUG) << "write header error: " << e.message() << " (" << e.value() << ")";
					Close();
				}
			});
		}

		void AsioSocket::WriteBody(Packet* packet) {
			LOG(DEBUG) << "WriteBody()";

			auto self(shared_from_this());
			asio::async_write(m_socket,
				asio::buffer(packet->m_buf.data(), packet->offset),
				[this, self, packet](const std::error_code& e, size_t) {
				delete packet;
				if (!e) {
					m_sendQueue.pop_front();
					if (!m_sendQueue.empty()) {
						WriteHeader();
					}
				}
				else {
					LOG(DEBUG) << "write body error: " << e.message() << " (" << e.value() << ")";
					Close();
				}
			});
		}
	}
}
