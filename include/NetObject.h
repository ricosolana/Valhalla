#pragma once

#include "Vector.h"
#include "VUtils.h"
#include "ZDO.h"

#include "NetRouteManager.h"
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
    static constexpr HASH_t SYNC_HASH = VUtils::String::GetStableHashCode("Sync");

	NetSync *m_sync; // zdo

	//Rigidbody m_body;

	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<OWNER_t>>> m_functions;

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
	//void Destroy();

	NetSync* GetNetSync() const {
		return m_sync;
	}

	void ResetNetSync();

    void Register(HASH_t hash, IMethod<OWNER_t>* method);

    template<class ...Args>
    auto Register(HASH_t hash, void(*f)(OWNER_t, Args...)) {
        return Register(hash, new MethodImpl(f, SYNC_HASH ^ hash));
    }

    template<class C, class ...Args>
    auto Register(HASH_t hash, C* object, void(C::* f)(OWNER_t, Args...)) {
        return Register(hash, new MethodImpl(object, f, SYNC_HASH ^ hash));
    }

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(const char* name, void(*f)(OWNER_t, Args...)) {
		return Register(VUtils::String::GetStableHashCode(name), f);
	}

	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(const char* name, C* object, void(C::* f)(OWNER_t, Args...)) {
		return Register(VUtils::String::GetStableHashCode(name), object, f);
	}

	//void Unregister(string name); // seems unused

	void HandleRoutedRPC(NetRouteManager::Data rpcData); // directly calls

	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Types>
	void Invoke(OWNER_t target, const char* method, Types... params) {
		NetRouteManager::Invoke(target, m_sync->ID(), method, std::move(params)...);
	}

	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Types>
	void Invoke(const char* method, Types... params) {
		NetRouteManager::Invoke(m_sync->Owner(), m_sync->ID(), method, std::move(params)...);
	}

	//static void StartGhostInit();

	//static void FinishGhostInit();

};

typedef NetObject ZNetView;
