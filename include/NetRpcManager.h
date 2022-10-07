#pragma once

#include <robin_hood.h>
#include "NetPeer.h"
#include "Method.h"

namespace NetRpcManager {

	static constexpr uuid_t EVERYBODY = 0;

	struct Data {
		uuid_t m_msgID;
		uuid_t m_senderPeerID;
		uuid_t m_targetPeerID;
		ZDOID m_targetZDO;
		hash_t m_methodHash;
		NetPackage::Ptr m_parameters;

		Data() {}

		// Will unpack the package
		Data(NetPackage::Ptr pkg)
			: m_msgID(pkg->Read<uuid_t>()),
			m_senderPeerID(pkg->Read<uuid_t>()),
			m_targetPeerID(pkg->Read<uuid_t>()),
			m_targetZDO(pkg->Read<ZDOID>()),
			m_methodHash(pkg->Read<hash_t>()),
			m_parameters(pkg->Read<NetPackage::Ptr>())
		{}

		void Serialize(NetPackage::Ptr pkg) {
			pkg->Write(m_msgID);
			pkg->Write(m_senderPeerID);
			pkg->Write(m_targetPeerID);
			pkg->Write(m_targetZDO);
			pkg->Write(m_methodHash);
			pkg->Write(m_parameters);
		}
	};

	// Called from NetManager
	void OnNewPeer(NetPeer::Ptr peer);
	void OnPeerQuit(NetPeer::Ptr peer);

	// Internal use only by NetRpcManager
	uuid_t _ServerID();
	void _InvokeRoute(uuid_t target, const ZDOID& targetZDO, const std::string& name, NetPackage::Ptr pkg);
	void _HandleRoutedRPC(Data data);
	void _Register(const std::string& name, ZMethodBase<uuid_t>* method);

	/**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(const std::string& name, void(*f)(uuid_t, Args...)) {
		return _Register(name, new ZMethod(f));
	}

	/**
		* @brief Register an instance method for routed remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(const std::string& name, C* object, void(C::* f)(uuid_t, Args...)) {
		return _Register(name, new ZMethod(object, f));
	}

	/**
		* @brief Invoke a routed remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Args>
	void InvokeRoute(uuid_t target, const ZDOID& targetZDO, const std::string& name, Args... params) {
		auto pkg(PKG());
		NetPackage::Serialize(pkg, params...);
		_InvokeRoute(target, targetZDO, name, pkg);
	}

	/**
		* @brief Invoke a routed remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Args>
	void InvokeRoute(uuid_t target, const std::string& name, Args... params) {
		InvokeRoute(target, ZDOID::NONE, name, params...);
	}

	/**
		* @brief Invoke a routed remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Args>
	void InvokeRoute(const std::string& name, Args... params) {
		InvokeRoute(_ServerID(), name, params...);
	}

};
