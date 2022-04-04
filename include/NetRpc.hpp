#pragma once

#include <robin_hood.h>
#include "NetMethod.hpp"
#include "NetSocket.hpp"

#include "Utils.hpp"

namespace Alchyme {
	namespace Net {
		class Peer;

		/**
		* The client and Rpc should be merged somehow
		 * @brief
		 *
		*/
		class Rpc {
			using RpcHash = size_t;

			robin_hood::unordered_map<RpcHash, std::unique_ptr<IMethod>> m_methods;

		public:
			AsioSocket::Ptr m_socket;

			int not_garbage;

		public:
			Rpc(AsioSocket::Ptr socket);
			~Rpc();

			/**
			 * @brief Register a method to be remotely invoked
			 * @param name the function identifier
			 * @param method the function
			*/
			void Register(const char* name, IMethod* method);

			void Append_impl(Packet* p) {}

			template <typename T, typename... Types>
			void Append_impl(Packet* p, T var1, Types... var2) {
				p->Write(var1);

				Append_impl(p, var2...);
			}

			/**
			 * @brief Invoke a function remotely
			 * @param name function name
			 * @param ...types function params
			*/
			template <typename... Types>
			void Invoke(const char* name, Types... params) {

				/*
				* Binary Packet Format:
				*  |0...................1...................2....|
				*  |0_1_2_3_4_5_6_7_8_9_0_1_2_3_4_5_6_7_8_9_0....|
				*  |_Fn_Hash_|______________params_______________|
				*/

				Packet* p = new Packet();
				p->Write(Utils::StrHash(name));

				// Recursive variadic template write
				Append_impl(p, params...);

				// Flush packet
				m_socket->FlushPacket(p);
			}

			void Update(Peer* peer);
		};
	}
}
