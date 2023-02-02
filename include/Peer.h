#pragma once

#include "VUtils.h"
#include "NetSocket.h"
#include "Method.h"
#include "ZDO.h"
#include "ValhallaServer.h"

class IZDOManager;
class INetManager;

enum class TalkerType : int32_t
{
    Whisper,
    Normal,
    Shout,
    Ping
};

class Peer {
    friend class IZDOManager;
    friend class INetManager;

    friend void ZDO::InvalidateSector();

private:
    std::chrono::steady_clock::time_point m_lastPing;
    robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

    robin_hood::unordered_map<NetID, ZDO::Rev> m_zdos;
    robin_hood::unordered_set<NetID> m_forceSend;
    robin_hood::unordered_set<NetID> m_invalidSector;
    int m_sendIndex = 0; // used incrementally for which next zdos to send from index

public:
    ISocket::Ptr m_socket;

    const OWNER_t m_uuid;
    const std::string m_name;
    bool m_admin = false;
    bool m_magicLogin = false;
    //bool m_nextMagicLogin = false;

    // Constantly changing vars
    Vector3 m_pos;
    bool m_visibleOnMap = false;
    NetID m_characterID = NetID::NONE;

private:
    void Update();

    void ZDOSectorInvalidated(ZDO* zdo);
    void ForceSendZDO(const NetID& id);
    bool IsOutdatedZDO(ZDO* zdo);

public:
    Peer(ISocket::Ptr socket, OWNER_t uuid, const std::string &name, const Vector3 &pos)
        : m_socket(std::move(socket)), m_lastPing(steady_clock::now()), 
        m_name(name), m_uuid(uuid), m_pos(pos)
    {}

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        m_methods[hash] = std::unique_ptr<IMethod<Peer*>>(new MethodImpl(func));
    }

    template<typename F>
    void Register(const std::string& name, F func) {
        Register(VUtils::String::GetStableHashCode(name), func);
    }

    template <typename... Types>
    void Invoke(HASH_t hash, const Types&... params) {
        // Can still fail during mid-frame Close() calls
        //assert(m_socket && m_socket->Connected());

        if (!m_socket->Connected())
            return;

        static NetPackage pkg; // TODO make into member to optimize; or make static
        pkg.m_stream.Clear();

        pkg.Write(hash);
        NetPackage::_Serialize(pkg, params...); // serialize

        m_socket->Send(std::move(pkg));
    }

    template <typename... Types>
    void Invoke(const char* name, Types... params) {
        Invoke(VUtils::String::GetStableHashCode(name), std::move(params)...);
    }

    template <typename... Types>
    void Invoke(std::string& name, const Types&... params) {
        Invoke(name.c_str(), params...);
    }

    void RemotePrint(const std::string& msg);
    void Kick(bool now = false);
    void Kick(std::string reason);
    void SendDisconnect();
    void Disconnect();

    // Send a chat message
    void Message(const std::string& text, TalkerType type = TalkerType::Normal);

    // Send a screen popup message
    void ShowMessage(const std::string& text, MessageType type = MessageType::TopLeft);
};
