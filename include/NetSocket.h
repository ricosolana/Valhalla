#pragma once

#include <string>
#include <memory>
#include <optional>
#include <queue>
#include <steamnetworkingtypes.h>

#include "VUtils.h"
#include "NetPackage.h"

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
    virtual std::string GetHostName() const = 0;

    // Get the address of this socket
    virtual std::string GetAddress() const = 0;

    // Returns true if the socket is alive
    // Expected to return false if the connection has been closed
    virtual bool Connected() const = 0;

    // Returns the size in bytes of packets queued for sending
    // Returns -1 on failure
    virtual int GetSendQueueSize() const = 0;
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

    std::string GetHostName() const override;
    std::string GetAddress() const override;
    bool Connected() const override;
    int GetSendQueueSize() const override;

private:
    void SendQueued();

};
