#include "NetSocket.h"

NetSocket::NetSocket(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)) {
    m_connected = true;
}

NetSocket::~NetSocket() {
    Close(true);
}

void NetSocket::Close(bool flush) {

}

void NetSocket::Update() {}

void NetSocket::Send(BYTES_t bytes) {
    std::scoped_lock scoped(m_mux);
    bool empty = this->m_send.empty();
    this->m_send.push_back(std::move(bytes));
    if (empty) {
        WritePkgSize();
    }
}

std::optional<BYTES_t> NetSocket::Recv() { 
    std::scoped_lock scoped(m_mux);
    auto&& begin = m_recv.begin();
    if (begin != m_recv.end()) {
        BYTES_t bytes = std::move(*begin);
        m_recv.erase(begin);
        return bytes;
    }
    return std::nullopt; 
}

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



void NetSocket::ReadPkgSize() {
    auto self(shared_from_this());
    asio::async_read(m_socket,
        asio::buffer(&m_tempReadOffset, sizeof(m_tempReadOffset)),
        [this, self](const std::error_code& e, size_t) {
            if (!e) {
                ReadPkg();
            }
            else {
                Close(false);
            }
        }
    );
}

void NetSocket::ReadPkg() {
    // Max of ~10.48 Mb per packet
    if (m_tempReadOffset == 0 || m_tempReadOffset > 0xA00000) {
        Close(false);
    }
    else {
        auto self(shared_from_this());
        asio::async_read(m_socket,
            asio::buffer(m_tempReadBytes, m_tempReadOffset),
            [this, self](const std::error_code& e, size_t) {
                if (!e) {
                    {
                        std::scoped_lock scoped(m_mux);
                        m_recv.push_back(std::move(m_tempReadBytes));
                    }
                    ReadPkgSize();
                }
                else {
                    Close(false);
                }
            }
        );
    }
}



void NetSocket::WritePkgSize() {
    BYTES_t bytes;
    int left{};
    {    
        std::scoped_lock scoped(m_mux);
        left = m_send.size();
        if (left) {
            bytes = std::move(m_send.front());

            m_tempWriteOffset = bytes.size();
            m_sendQueueSize -= m_tempWriteOffset;

            m_send.erase(m_send.begin());
        }
    }

    if (left) {
        --left;
        // https://stackoverflow.com/questions/8640393/move-capture-in-lambda
        auto self(shared_from_this());
        asio::async_write(m_socket,
            asio::buffer(&m_tempWriteOffset, sizeof(m_tempWriteOffset)),
            [this, self, bytes = std::move(bytes), left] (const std::error_code& e, size_t) mutable {
                if (!e) {
                    // Call writepkg
                    WritePkg(std::move(bytes), left);
                }
                else {
                    Close(false);
                }
            }
        );
    }
}

void NetSocket::WritePkg(BYTES_t bytes, bool empty) {
    auto self(shared_from_this());
    asio::async_write(m_socket,
        asio::buffer(std::move(bytes)),
        [this, self, empty](const std::error_code& e, size_t) {
            if (!e) {
                if (!empty) {
                    WritePkgSize();
                }
            }
            else {
                Close(false);
            }
        }
    );
}