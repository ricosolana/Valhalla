#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "VUtilsString.h"
#include "Method.h"
#include "NetSocket.h"
#include "Task.h"

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
    MAX // 10
};

class NetRpc {
private:
    std::chrono::steady_clock::time_point m_lastPing;

    robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<NetRpc*>>> m_methods;

public:
    ISocket::Ptr m_socket;

    bool m_skipPassword = false;

public:
    explicit NetRpc(ISocket::Ptr socket) 
        : m_socket(std::move(socket)), m_lastPing(steady_clock::now()) {}

    NetRpc(const NetRpc& other) = delete; // copy
    NetRpc(NetRpc&& other) = delete; // move

    ~NetRpc() {
        LOG(INFO) << "~NetRpc()";
    }

    /**
        * @brief Register a static method for remote invocation
        * @param name function name to register
        * @param lambda
    */
    template<typename F>
    void Register(HASH_t hash, F func) {
        m_methods[hash] = std::unique_ptr<IMethod<NetRpc*>>(new MethodImpl(func));
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
        assert(m_socket && m_socket->Connected());

        NetPackage pkg; // TODO make into member to optimize; or make static
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

    void Close(ConnectionStatus status);

    void PollOne();
};
