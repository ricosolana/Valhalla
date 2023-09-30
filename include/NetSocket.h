#pragma once

#include <string>
#include <memory>
#include <optional>
#include <queue>

#include <steamnetworkingtypes.h>
#include <isteamfriends.h>

#include "VUtils.h"

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
    // If flush is set, socket wont close until a few seconds
    virtual void Close(bool flush) = 0;



    // Call every tick to reengage writers
    virtual void Update() = 0;

    // Send a packet to the remote host
    // Packet will be copied unless moved
    virtual void Send(BYTES_t bytes) = 0;

    // Receive a packet from the remote host
    // Packet will undergo basic structure validation
    // This function shall not block
    virtual std::optional<BYTES_t> Recv() = 0;



    // Get the name of this connection
    // This represents the identity of the remote
    virtual std::string GetHostName() const = 0;

    // Get the address of this socket
    virtual std::string GetAddress() const = 0;

    // Returns whether the socket is connected
    //  The return value is updated every frame, 
    //  so calls might not reflect the actual 
    //  state if called from mid-frame
    virtual bool Connected() const = 0;



    // Returns the size in bytes of packets queued for sending
    virtual unsigned int GetSendQueueSize() const = 0;

    virtual unsigned int GetPing() const = 0;
};



class SteamSocket : public ISocket {
private:
    std::list<BYTES_t> m_sendQueue;
    std::string m_address;
    bool m_connected{};

public:
    const HSteamNetConnection m_hConn;
    SteamNetworkingIdentity m_steamNetId{};

public:
    explicit SteamSocket(HSteamNetConnection hConn);
    ~SteamSocket() override;

    void Close(bool flush) override;
    
    void Update() override;
    void Send(BYTES_t bytes) override;
    std::optional<BYTES_t> Recv() override;

    std::string GetHostName() const override;
    std::string GetAddress() const override;
    bool Connected() const override;

    unsigned int GetSendQueueSize() const override;
    unsigned int GetPing() const override;

private:
    void SendQueued();
};