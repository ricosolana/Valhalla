#include "NetSocket.h"

NetSocket::NetSocket() {

}

NetSocket::~NetSocket() {

}

void NetSocket::Close(bool) {}

void NetSocket::Update() {}

void NetSocket::Send(BYTES_t) {}

std::optional<BYTES_t> NetSocket::Recv() { return std::nullopt; }

std::string NetSocket::GetHostName() const {
    return "";
}

std::string NetSocket::GetAddress() const {
    return "";
}

bool NetSocket::Connected() const {
    return false;
}

unsigned int NetSocket::GetSendQueueSize() const {
    return 0;
}

unsigned int NetSocket::GetPing() const {
    return 0;
}
