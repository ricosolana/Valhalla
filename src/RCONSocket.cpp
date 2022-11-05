#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

#include "NetSocket.h"
#include "ValhallaServer.h"

RCONSocket::RCONSocket(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)),
    m_tempReadSize(0),
    m_tempWriteSize(0) {
    m_connected = true;
}

RCONSocket::~RCONSocket() {
    Close(true);
}



void RCONSocket::Start() {
    ReadPacketSize();
}

void RCONSocket::Close(bool flush) {
    if (!Connected())
        return;

    m_connected = false;

    if (flush) {
        auto self(shared_from_this());
        Valhalla()->RunTaskLater([this, self](Task&) {
            m_socket.close();
        }, 3s);
    } else {
        m_socket.close();
    }
}

void RCONSocket::Update() {
    OPTICK_EVENT();
    if (!m_sendQueue.empty()) {
        WritePacketSize();
    }
}

void RCONSocket::Send(NetPackage pkg) {
    if (pkg.m_stream.Length() == 0)
        return;

    m_sendQueue.push_back(std::move(pkg.m_stream.Bytes()));
}

std::optional<NetPackage> RCONSocket::Recv() {
    OPTICK_EVENT();
    if (Connected() && !m_recvQueue.empty())
        return m_recvQueue.pop_back();
    return std::nullopt;
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
            if (Connected()) // if flushing while closing, do not read
                ReadPacket();
        } else {
            Close(false);
        }
    });
}

void RCONSocket::ReadPacket() {
    if (m_tempReadSize < 10 || m_tempReadSize > 4096) {
        Close(false);
    } else {
        m_tempReadPkg.m_stream.m_buf.resize(m_tempReadSize);
        asio::async_read(m_socket,
                         asio::buffer(m_tempReadPkg.m_stream.m_buf),
                         [this](const asio::error_code& ec, size_t) {
             if (!ec) {
                 // Ensure payload is null terminated
                 if (m_tempReadPkg.m_stream.m_buf[m_tempReadSize - 1] != '\0') {
                     Close(false);
                 } else {
                     m_recvQueue.push_back(std::move(m_tempReadPkg));
                     ReadPacketSize();
                 }
             } else {
                 Close(false);
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
             Close(false);
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
              Close(false);
          }
      });
}








#pragma clang diagnostic pop