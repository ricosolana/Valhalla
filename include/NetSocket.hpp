#pragma once

#include <string>
#include "NetPacket.hpp"

#include "Utils.hpp"
#include <memory>

using namespace asio::ip;

namespace Alchyme {
	namespace Net {
		/**
		 * @brief Custom socket implementation which makes use of Asio tcp socket
		 *
		*/
		class AsioSocket : public std::enable_shared_from_this<AsioSocket> {
			tcp::socket m_socket;
			AsyncDeque<Packet*> m_sendQueue;
			AsyncDeque<Packet*> m_recvQueue;

			std::string m_hostname;
			uint_least16_t m_port;

			std::atomic_bool m_online = true;

		public:
			using Ptr = std::shared_ptr<AsioSocket>;

			AsioSocket(asio::io_context& ctx, tcp::socket socket);
			AsioSocket(asio::io_context& ctx);
			~AsioSocket();

			void Accept();

			/**
			 * @brief Add packet to queue
			 * @param packet the new Packet()
			 * @return result
			*/
			bool QueuePacket(NOTNULL Packet* packet);

			/**
			 * @brief Add packet to queue and flush
			 * @param packet the new Packet()
			 * @return result
			*/
			bool FlushPacket(NOTNULL Packet* packet);
			bool Flush();

			NULLABLE Packet* NextPacket();
			bool Close();

			std::string& GetHostName();
			uint_least16_t GetHostPort();
			bool IsOnline();
			tcp::socket& GetSocket();

		private:
			void ReadHeader();
			void ReadBody(Packet* packet);
			void WriteHeader();
			void WriteBody(Packet* packet);
		};
	}
}
