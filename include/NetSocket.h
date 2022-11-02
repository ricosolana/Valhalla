#pragma once

#include <asio.hpp>
#include <string>
#include <memory>
#include <optional>
#include <queue>

#include "Utils.h"
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
class ISocket {
public:
	virtual ~ISocket() noexcept = default;



	// Begin socket readers
	virtual void Start() = 0;



	// Terminates the connection
	virtual void Close() = 0;



	// Call every tick to reengage writers
	virtual void Update() = 0;

	// Send a packet to the remote host
	virtual void Send(const NetPackage& pkg) = 0;

	// Receive a packet from the remote host
    // This function shall not block, and the optional may be empty
	virtual std::optional<NetPackage> Recv() = 0;



	// Get the name of this connection
    // This represents the identity of the remote
	[[nodiscard]] virtual std::string GetHostName() const = 0;



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

public:
	HSteamNetConnection m_hConn;
	SteamNetworkingIdentity m_steamNetId;

public:
	explicit SteamSocket(HSteamNetConnection con);
	~SteamSocket() override;

	// Virtual
	void Start() override;
	void Close() override;
	
	void Update() override;
	void Send(const NetPackage &pkg) override;
	std::optional<NetPackage> Recv() override;

	[[nodiscard]] std::string GetHostName() const override {
		return std::to_string(m_steamNetId.GetSteamID64());
	}

	[[nodiscard]] bool Connected() const override {
		return m_hConn != k_HSteamNetConnection_Invalid;
	}

	[[nodiscard]] int GetSendQueueSize() const override;

private:
	void SendQueued();

};

class RCONSocket : public ISocket {
private:
    asio::ip::tcp::socket  m_socket;

    AsyncDeque<BYTES_t> m_sendQueue;
    AsyncDeque<NetPackage> m_recvQueue;
    std::atomic_int m_sendQueueSize;
    std::atomic_bool m_connected;

    NetPackage m_tempReadPkg;
    uint32_t m_tempReadSize;
    uint32_t m_tempWriteSize;

public:
    explicit RCONSocket(asio::ip::tcp::socket socket);
    ~RCONSocket() noexcept override;

    // Virtual
    void Start() override;
    void Close() override;

    void Update() override;
    void Send(const NetPackage &pkg) override;
    std::optional<NetPackage> Recv() override;

    std::string GetHostName() const override;

    bool Connected() const override {
        return m_connected;
    }

    int GetSendQueueSize() const override {
        return m_sendQueueSize;
    }

private:
    void ReadPacketSize();
    void ReadPacket();

    void WritePacketSize();
    void WritePacket(BYTES_t& bytes);
};
