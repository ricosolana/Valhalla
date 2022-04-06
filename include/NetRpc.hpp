#pragma once

#include <robin_hood.h>
#include "NetMethod.hpp"
#include "NetSocket.hpp"

#include "Utils.hpp"
#include "NetPackage.hpp"

class Peer;

/**
* The client and Rpc should be merged somehow
	* @brief
	*
*/
class Rpc {
	void Append_impl(Package* p) {}

	template <typename T, typename... Types>
	void Append_impl(Package* p, T var1, Types... var2) {
		p->Write(var1);

		Append_impl(p, var2...);
	}

	robin_hood::unordered_map<int, std::unique_ptr<IMethod>> m_methods;
	Socket2::Ptr m_socket;

public:
	int not_garbage;

	Rpc(Socket2::Ptr socket);
	~Rpc();

	bool IsConnected();

	/**
		* @brief Register a method to be remotely invoked
		* @param name the function identifier
		* @param method the function
	*/
	void Register(const char* name, IMethod* method);

	/**
		* @brief Invoke a function remotely
		* @param name function name
		* @param ...types function params
	*/
	template <typename... Types>
	void Invoke(const char* method, Types... params) {
		if (!IsConnected())
			return;

		auto pkg = new Package();
		int stable = Utils::GetStableHashCode(method);
		Append_impl(pkg, params...); // serialize
		m_socket->Send(pkg);
	}

	void Update();
};
