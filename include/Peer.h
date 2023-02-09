#pragma once

#include "VUtils.h"
#include "NetSocket.h"
#include "Method.h"
#include "ZDO.h"
#include "ValhallaServer.h"
#include "ModManager.h"

class IZDOManager;
class INetManager;

// TODO merge player message types all in one
// MessageType: 

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

private:
    std::chrono::steady_clock::time_point m_lastPing;
    robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

public:
    robin_hood::unordered_map<NetID, ZDO::Rev> m_zdos;
    robin_hood::unordered_set<NetID> m_forceSend;
    robin_hood::unordered_set<NetID> m_invalidSector;
    //int m_sendIndex = 0; // used incrementally for which next zdos to send from index

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
        m_methods[hash] = std::unique_ptr<IMethod<Peer*>>(new MethodImpl(func, EVENT_HASH_RpcIn, hash)); // TODO use make_unique
        //m_methods[hash] = std::make_unique<MethodImpl<Peer*>>(func, EVENT_HASH_RpcIn, hash);
    }

    template<typename F>
    void Register(const std::string& name, F func) {
        Register(VUtils::String::GetStableHashCode(name), func);
    }

    void Register(HASH_t hash, sol::function func, std::vector<DataType> types) {
        //std::make_unique<MethodImplLua<Peer*>>(std::move(func), std::move(types));

        m_methods[hash] = std::make_unique<MethodImplLua<Peer*>>(std::move(func), std::move(types));
            //std::unique_ptr<IMethod<Peer*>>(new MethodImplLua(std::move(func), std::move(types)));
    }

    template <typename... Types>
    void Invoke(HASH_t hash, Types&&... params) {
        // Can still fail during mid-frame Close() calls
        //assert(m_socket && m_socket->Connected());

        if (!m_socket->Connected())
            return;

        if (ModManager()->CallEvent(EVENT_HASH_RpcOut ^ hash, this, params...) == EventStatus::CANCEL)
            return;

        static BYTES_t bytes; bytes.clear();
        DataWriter writer(bytes);

        writer.Write(hash);
        DataWriter::_Serialize(writer, std::forward<Types>(params)...); // serialize

        m_socket->Send(std::move(bytes));
    }

    template <typename... Types>
    void Invoke(const char* name, Types&&... params) {
        Invoke(VUtils::String::GetStableHashCode(name), std::forward<Types>(params)...);
    }

    template <typename... Types>
    void Invoke(std::string& name, Types&&... params) {
        Invoke(name.c_str(), std::forward<Types>(params)...);
    }



    void InvokeSelf(HASH_t hash, DataReader reader) {
        if (auto method = GetMethod(hash))
            method->Invoke(this, reader);
    }

    void InvokeSelf(const std::string& name, DataReader reader) {
        if (auto method = GetMethod(name))
            method->Invoke(this, reader);
    }

    IMethod<Peer*>* GetMethod(const std::string& name);
    IMethod<Peer*>* GetMethod(HASH_t hash);

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
