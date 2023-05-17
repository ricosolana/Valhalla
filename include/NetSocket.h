#pragma once

#include <string>
#include <memory>
#include <optional>
#include <queue>

#include "VUtils.h"

// All ISocket functions are expected to:
// - return instantly without blocking
// - be thread safe
// - be fully implemented
class ISocket {
public:
    ~ISocket();



    // Terminates the connection
    // If flush is set, socket wont close until a few seconds
    void Close(bool flush);



    // Call every tick to reengage writers
    void Update();

    // Send a packet to the remote host
    // Packet will be copied unless moved
    void Send(BYTES_t bytes);

    // Receive a packet from the remote host
    // Packet will undergo basic structure validation
    // This function shall not block
    std::optional<BYTES_t> Recv();



    // Get the name of this connection
    // This represents the identity of the remote
    std::string GetHostName() const;

    // Get the address of this socket
    std::string GetAddress() const;

    // Returns whether the socket is connected
    //  The return value is updated every frame, 
    //  so calls might not reflect the actual 
    //  state if called from mid-frame
    bool Connected() const;



    // Returns the size in bytes of packets queued for sending
    unsigned int GetSendQueueSize() const;

    unsigned int GetPing() const;
};
