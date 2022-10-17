#pragma once

#include "Vector.h"
#include "Utils.h"
#include "NetSync.h"

#include "NetRpcManager.h"
#include "Method.h"

/*
 * ZNetView equivalent
 * NetObject is a wrapper around a ZDO and certain exceptional ZRoutedRPC methods hosted here only
 */

// change name to something more obvious
// netobject
// netview is just a wrapper to represent an object with synchronized fields and method calls
class NetObject {
private:
	NetSync *m_sync; // zdo

	//Rigidbody m_body;

	robin_hood::unordered_map<hash_t, std::unique_ptr<IMethod<uuid_t>>> m_functions;

	bool m_ghost;

	//static bool m_ghostInit; // unused

public:

	bool m_persistent;

	bool m_distant;

	//NetSync.ObjectType m_type; // for send priorities

	bool m_syncInitialScale;

	static bool m_useInitNetSync;

	static NetSync m_initNetSync;

	static bool m_forceDisableInit;

public:
	NetObject();
	void SetLocalScale(Vector3 scale);

	//public void SetPersistent(bool persistent); // seems unused

	std::string GetPrefabName() const;
	void Destroy();

	NetSync* GetNetSync() const {
		return m_sync;
	}

	void ResetNetSync();

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(const char* name, void(*f)(uuid_t, Args...)) {
		return Register(name, new MethodImpl(f));
	}

	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(const char* name, C* object, void(C::* f)(uuid_t, Args...)) {
		return Register(name, new MethodImpl(object, f));
	}

	//void Unregister(string name); // seems unused

	void HandleRoutedRPC(NetRpcManager::Data rpcData); // directly calls

	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Types>
	void Invoke(uuid_t target, const char* method, Types... params) {
		NetRpcManager::InvokeRoute(target, m_sync->m_id, method, std::move(params)...);
	}

	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Types>
	void Invoke(const char* method, Types... params) {
		NetRpcManager::InvokeRoute(m_sync->m_owner, m_sync->m_id, method, std::move(params)...);
	}

	//static void StartGhostInit();

	//static void FinishGhostInit();

};