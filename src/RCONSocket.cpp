#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#include "NetSocket.h"

RCONSocket::RCONSocket(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)),
    m_tempReadSize(0),
    m_tempWriteSize(0) {
    m_connected = true;
}

RCONSocket::~RCONSocket() noexcept {
    Close();
}

void RCONSocket::Start() {
    ReadPacketSize();
}

void RCONSocket::Close() {
    if (Connected()) {
        m_connected = false;
        m_socket.close();
    }
}

void RCONSocket::Update() {
    if (!m_sendQueue.empty()) {
        WritePacketSize();
    }
}

void RCONSocket::Send(const NetPackage &pkg) {
    m_sendQueue.push_back(pkg.m_stream.Bytes());
}

std::optional<NetPackage> RCONSocket::Recv() {
    if (m_recvQueue.empty())
        return std::nullopt;
    return m_recvQueue.pop_back();
}

std::string RCONSocket::GetHostName() const {
    return m_socket.remote_endpoint().address().to_string();
}



void RCONSocket::ReadPacketSize() {
    // read the command from the remote client
    asio::async_read(m_socket,
                     asio::buffer(&m_tempReadSize, sizeof(m_tempReadSize)),
                     [this](const asio::error_code& ec, size_t) {
        if (!ec) {
            // process command
            ReadPacket();
        } else {
            Close();
        }
    });
}

void RCONSocket::ReadPacket() {
    if (m_tempReadSize < 10 || m_tempReadSize > 4096) {
        Close();
    } else {
        m_tempReadPkg.m_stream.Buf().resize(m_tempReadSize);
        asio::async_read(m_socket,
                         asio::buffer(m_tempReadPkg.m_stream.Buf()),
                         [this](const asio::error_code& ec, size_t) {
             if (!ec) {
                 // Ensure payload is null terminated
                 if (m_tempReadPkg.m_stream.Ptr()[m_tempReadSize - 1] != '\0') {
                     Close();
                 } else {
                     m_recvQueue.push_back(std::move(m_tempReadPkg));
                     ReadPacketSize();
                 }
             } else {
                 Close();
             }
         });
    }
}

void RCONSocket::WritePacketSize() {
    auto&& bytes = m_sendQueue.front();
    m_tempWriteSize = bytes.size();

    asio::async_write(m_socket,
                     asio::buffer(&m_tempWriteSize, sizeof(m_tempWriteSize)),
                     [this, &bytes](const asio::error_code& ec, size_t) {
         if (!ec) {
             // process command
             m_sendQueueSize -= sizeof(m_tempWriteSize);
             WritePacket(bytes);
         } else {
             Close();
         }
     });
}

void RCONSocket::WritePacket(BYTES_t& bytes) {
    asio::async_write(m_socket,
                      asio::buffer(bytes),
                      [this](const asio::error_code& ec, size_t) {
          if (!ec) {
              // process command
              m_sendQueueSize -= m_sendQueue.pop_front().size();
              if (!m_sendQueue.empty())
                WritePacketSize();
          } else {
              Close();
          }
      });
}








#pragma clang diagnostic pop