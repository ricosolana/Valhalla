#pragma once

#include "Peer.h"
#include "Method.h"
#include "ValhallaServer.h"
#include "DataReader.h"
#include "DataWriter.h"

class IRouteManager {
	friend class INetManager;

public:
	struct Data {
		//OWNER_t m_msgID; // TODO this is never utilized
		OWNER_t m_sender;
		OWNER_t m_target;
		ZDOID m_targetZDO;
		HASH_t m_method;
		BYTES_t m_params;

		Data() : m_sender(0), m_target(0), m_method(0) {}

		// Will unpack the package
		explicit Data(DataReader reader) {
			reader.Read<int64_t>(); // msgID
			m_sender = reader.Read<OWNER_t>();
			m_target = reader.Read<OWNER_t>();
			m_targetZDO = reader.Read<ZDOID>();
			m_method = reader.Read<HASH_t>();
			m_params = reader.Read<BYTES_t>();
		}

		void Serialize(DataWriter writer) const {
			writer.Write<int64_t>(0);
			writer.Write(m_sender);
			writer.Write(m_target);
			writer.Write(m_targetZDO);
			writer.Write(m_method);
			writer.Write(m_params);
		}
	};

	static constexpr OWNER_t EVERYBODY = 0;

private:	
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

private:
	// Called from NetManager
	void OnNewPeer(Peer &peer);

	// Internal use only by NetRouteManager
	void InvokeImpl(OWNER_t target, const ZDOID& targetNetSync, HASH_t hash, BYTES_t params);
	void HandleRoutedRPC(Peer* peer, Data data);

	void RouteRPC(const Data& data);

public:

	/**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<typename F>
	void Register(HASH_t hash, F func) {
		//m_methods[hash] = std::unique_ptr<IMethod<Peer*>>(new MethodImpl(func, EVENT_HASH_RouteIn, hash));
		m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, EVENT_HASH_RouteIn, hash);
	}

	template<typename F>
	auto Register(const std::string& name, F func) {
		return Register(VUtils::String::GetStableHashCode(name), func);
	}



	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const ZDOID& targetNetSync, HASH_t hash, const Args&... params) {
		InvokeImpl(target, targetNetSync, hash, DataWriter::Serialize(params...));
	}

	template <typename... Args>
	void InvokeView(Peer& peer, const ZDOID& targetZDO, HASH_t hash, const Args&... params) {
		Data data;
		data.m_sender = SERVER_ID;
		data.m_target = peer.m_uuid;
		data.m_targetZDO = targetZDO;
		data.m_method = hash;
		data.m_params = DataWriter::Serialize(params...);

		static BYTES_t bytes; bytes.clear();
		data.Serialize(DataWriter(bytes));

		peer.Invoke(Hashes::Rpc::RoutedRPC, bytes);
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const ZDOID& targetNetSync, const std::string& name, const Args&... params) {
		InvokeImpl(target, targetNetSync, VUtils::String::GetStableHashCode(name), DataWriter::Serialize(params...));
	}

	template <typename... Args>
	void InvokeView(Peer& peer, const ZDOID& targetZDO, const std::string& name, const Args&... params) {
		InvokeView(peer, targetZDO, VUtils::String::GetStableHashCode(name), params...);
	}



	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, const Args&... params) {
		InvokeView(target, ZDOID::NONE, hash, params...);
	}

	template <typename... Args>
	void Invoke(Peer& peer, HASH_t hash, const Args&... params) {
		InvokeView(peer, ZDOID::NONE, hash, params...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, const std::string& name, const Args&... params) {
		InvokeView(target, ZDOID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}

	template <typename... Args>
	void Invoke(Peer& peer, const std::string& name, const Args&... params) {
		InvokeView(peer, ZDOID::NONE, VUtils::String::GetStableHashCode(name), params);
	}




	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(HASH_t hash, const Args&... params) {
		Invoke(EVERYBODY, hash, params...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(const std::string& name, const Args&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), params...);
	}

};

// Manager class for everything related to high-level networking for simulated p2p communication
IRouteManager* RouteManager();
