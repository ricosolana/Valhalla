#pragma once

#include <asio.hpp>
#include <string>
#include <memory>
#include <optional>
#include <queue>

#include "VUtils.h"
#include "NetPackage.h"
#include "AsyncDeque.h"

#include <steam_api.h>
#include <isteamgameserver.h>
#include <steam_gameserver.h>
#include <isteamnetworkingutils.h>

// All ISocket functions are expected to:
// - return instantly without blocking
// - be thread safe
// - be fully implemented
class ISocket : public std::enable_shared_from_this<ISocket> {
public:
    using Ptr = std::shared_ptr<ISocket>;

public:
	virtual ~ISocket() = default;



	// Terminates the connection
	virtual void Close(bool flush) = 0;



	// Call every tick to reengage writers
	virtual void Update() = 0;

	// Send a packet to the remote host
    // Packet will be copied unless moved
	virtual void Send(NetPackage pkg) = 0;

	// Receive a packet from the remote host
    // Packet will undergo basic structure validation
    // This function shall not block
	virtual std::optional<NetPackage> Recv() = 0;



	// Get the name of this connection
    // This represents the identity of the remote
	[[nodiscard]] virtual std::string GetHostName() const = 0;



    // Get the address of this socket
    [[nodiscard]] virtual std::string GetAddress() const = 0;


	// Returns true if the socket is alive
    // Expected to return false if the connection has been closed
	[[nodiscard]] virtual bool Connected() const = 0;



	// Returns the size in bytes of packets queued for sending
	// Returns -1 on failure
	[[nodiscard]] virtual int GetSendQueueSize() const = 0;
};

class SteamSocket : public ISocket {
private:
	std::deque<BYTES_t> m_sendQueue;
    bool m_connected;
    std::string m_address;

public:
	HSteamNetConnection m_hConn;
	SteamNetworkingIdentity m_steamNetId;

public:
	explicit SteamSocket(HSteamNetConnection hConn);
    ~SteamSocket() override;

	// Virtual
	void Close(bool flush) override;
	
	void Update() override;
	void Send(NetPackage pkg) override;
	std::optional<NetPackage> Recv() override;

	[[nodiscard]] std::string GetHostName() const override {
		return std::to_string(m_steamNetId.GetSteamID64());
	}

    [[nodiscard]] std::string GetAddress() const override {
        return m_address;
    }

	[[nodiscard]] bool Connected() const override {
		//return m_hConn != k_HSteamNetConnection_Invalid;
        return m_connected;
	}

	[[nodiscard]] int GetSendQueueSize() const override;

private:
	void SendQueued();

};



enum class RCONMsgType : int32_t {
    RCON_S2C_RESPONSE = 0,
    RCON_COMMAND = 2,
    RCON_C2S_LOGIN = 3
};

struct RCONMsg {
    int32_t client;
    RCONMsgType type;
    std::string msg;
};

class RCONAcceptor;
class RCONSocket : public ISocket {
    friend RCONAcceptor; // might require forward declaring

private:
    asio::ip::tcp::socket m_socket;

    AsyncDeque<BYTES_t> m_sendQueue;
    AsyncDeque<NetPackage> m_recvQueue;
    std::atomic_int m_sendQueueSize;
    std::atomic_bool m_connected;
    std::string m_address;

    NetPackage m_tempReadPkg;
    uint32_t m_tempReadSize;
    uint32_t m_tempWriteSize;

    int32_t m_id = -1;

public:
    explicit RCONSocket(asio::ip::tcp::socket socket);
    ~RCONSocket() override;

    // Virtual
    void Close(bool flush) override;

    void Update() override;
    void Send(NetPackage pkg) override;
    std::optional<NetPackage> Recv() override;

    [[nodiscard]] std::string GetHostName() const override;
    [[nodiscard]] std::string GetAddress() const override;

    bool Connected() const override {
        return m_connected;
    }

    int GetSendQueueSize() const override {
        return m_sendQueueSize;
    }


    // Declared
    void SendMsg(const std::string& msg);
    std::optional<RCONMsg> RecvMsg();

private:
    void ReadPacketSize();
    void ReadPacket();

    void WritePacketSize();
    void WritePacket(BYTES_t& bytes);
};
