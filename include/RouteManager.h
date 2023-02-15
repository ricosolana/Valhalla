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
		OWNER_t m_msgID; // TODO this is never utilized
		OWNER_t m_senderPeerID;
		OWNER_t m_targetPeerID;
		NetID m_targetSync;
		HASH_t m_methodHash;
		BYTES_t m_parameters;

		Data() : m_msgID(0), m_senderPeerID(0), m_targetPeerID(0), m_methodHash(0) {}

		// Will unpack the package
		explicit Data(DataReader reader)
			: m_msgID(reader.Read<OWNER_t>()),
			m_senderPeerID(reader.Read<OWNER_t>()),
			m_targetPeerID(reader.Read<OWNER_t>()),
			m_targetSync(reader.Read<NetID>()),
			m_methodHash(reader.Read<HASH_t>()),
			m_parameters(reader.Read<BYTES_t>())
		{}

		void Serialize(DataWriter writer) const {
			writer.Write(m_msgID);
			writer.Write(m_senderPeerID);
			writer.Write(m_targetPeerID);
			writer.Write(m_targetSync);
			writer.Write(m_methodHash);
			writer.Write(m_parameters);
		}
	};

	static constexpr OWNER_t EVERYBODY = 0;

private:	
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

private:
	// Called from NetManager
	void OnNewPeer(Peer *peer);

	// Internal use only by NetRouteManager
	void InvokeImpl(OWNER_t target, const NetID& targetNetSync, HASH_t hash, BYTES_t params);
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
		assert(!m_methods.contains(hash));
		m_methods[hash] = std::unique_ptr<IMethod<Peer*>>(new MethodImpl(func, EVENT_HASH_RouteIn, hash));
	}

	template<typename F>
	auto Register(const std::string& name, F func) {
		return Register(VUtils::String::GetStableHashCode(name), func);
	}



	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const Args&... params) {
		InvokeImpl(target, targetNetSync, hash, DataWriter::Serialize(params...));
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const NetID& targetNetSync, const char* name, const Args&... params) {
		InvokeImpl(target, targetNetSync, VUtils::String::GetStableHashCode(name), DataWriter::Serialize(params...));
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const NetID& targetNetSync, std::string& name, const Args&... params) {
		InvokeImpl(target, targetNetSync, VUtils::String::GetStableHashCode(name), DataWriter::Serialize(params...));
	}



	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, const Args&... params) {
		InvokeView(target, NetID::NONE, hash, params...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, const char* name, const Args&... params) {
		InvokeView(target, NetID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, std::string& name, const Args&... params) {
		InvokeView(target, NetID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}



	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(HASH_t hash, const Args&... params) {
		Invoke(EVERYBODY, hash, params...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(const char* name, const Args&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), params...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(std::string& name, const Args&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), params...);
	}

};

IRouteManager* RouteManager();
