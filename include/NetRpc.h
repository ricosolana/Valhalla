#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "VUtilsString.h"
#include "Method.h"
#include "NetSocket.h"
#include "Task.h"
#include "DataWriter.h"
#include "openssl/md5.h"

enum class ConnectionStatus : int32_t {
    None,
    Connecting,
    Connected,
    ErrorVersion,
    ErrorDisconnected,
    ErrorConnectFailed,
    ErrorPassword,
    ErrorAlreadyConnected,
    ErrorBanned,
    ErrorFull,
    ErrorPlatformExcluded,
    ErrorCrossplayPrivilege,
    ErrorKicked,
    MAX // 13
};

class NetRpc {
private:
    std::chrono::steady_clock::time_point m_lastPing;

    robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<NetRpc*>>> m_methods;

    std::string m_password;

public:
    ISocket::Ptr m_socket;

    bool m_skipPassword = false;

public:
    NetRpc(ISocket::Ptr socket);

    NetRpc(const NetRpc& other) = delete; // copy

    ~NetRpc() {
        LOG(DEBUG) << "~NetRpc()";
    }

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        //m_methods[hash] = std::unique_ptr<IMethod<NetRpc*>>(new MethodImpl(func, EVENT_HASH_RpcIn, hash));
        //m_methods[hash] = std::make_unique<IMethod<NetRpc*, F>>(func, EVENT_HASH_RpcIn, hash);
        m_methods[hash] = std::make_unique<MethodImpl<NetRpc*, F>>(func, EVENT_HASH_RpcIn, hash);
    }

    template<typename F>
    void Register(const std::string& name, F func) {
        Register(VUtils::String::GetStableHashCode(name), func);
    }

    /**
        * @brief Invoke a remote function
        * @param name function name to invoke
        * @param ...types function parameters
    */
    // Passing parameter pack by reference
    // https://stackoverflow.com/a/6361619
    template <typename... Types>
    void Invoke(HASH_t hash, const Types&... params) {
        if (!m_socket->Connected())
            return;

        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(hash);
        DataWriter::_Serialize(writer, params...); // serialize

        m_socket->Send(std::move(bytes));
    }

    template <typename... Types>
    void Invoke(const char* name, const Types&... params) {
        Invoke(VUtils::String::GetStableHashCode(name), params...);
    }

    template <typename... Types>
    void Invoke(std::string& name, const Types&... params) {
        Invoke(name.c_str(), params...);
    }

    void Close(ConnectionStatus status);

    void PollOne();
};
