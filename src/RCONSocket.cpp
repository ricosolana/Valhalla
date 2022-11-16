#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

#include "NetSocket.h"
#include "VServer.h"
#include "VUtilsString.h"

RCONSocket::RCONSocket(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)),
    m_tempReadSize(0),
    m_tempWriteSize(0) {
    m_connected = true;
    m_address = m_socket.remote_endpoint().address().to_string();
}

RCONSocket::~RCONSocket() {
    Close(true);
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

void RCONSocket::SendMsg(const std::string& msg) {
    // If pkg made static, must be single threaded
    //  otherwise everything breaks
    // Package is just a wrapper around a Stream, which wraps a 
    //  vec, marker

    /*static*/ NetPackage pkg;
    pkg.Write(m_id);
    pkg.Write(RCONMsgType::RCON_S2C_RESPONSE);
    pkg.m_stream.Write((BYTE_t*)msg.data(),
        msg.size() + 1); // +1 to go ahead and copy the \0

    Send(std::move(pkg));
}

std::optional<RCONMsg> RCONSocket::RecvMsg() {
    auto&& opt = Recv();
    if (opt) {
        auto&& pkg = opt.value();
        if (pkg.m_stream.m_buf[m_tempReadSize - 1] != '\0') {
            Close(false);
        }
        else {
            RCONMsg msg;
            msg.client = pkg.Read<int32_t>();
            msg.type = pkg.Read<RCONMsgType>();
            msg.msg = std::string((char*)pkg.m_stream.Remaining().data());

            if (VUtils::String::FormatAscii(msg.msg)) {
                // send message: non ascii unsupported
                SendMsg("Only ascii is supported");
                Close(true);
            }
            else {
                return msg;
            }
        }
    }
    return std::nullopt;
}

std::string RCONSocket::GetHostName() const {
    return GetAddress();
}

std::string RCONSocket::GetAddress() const {
    return m_address;
}



void RCONSocket::ReadPacketSize() {
    // read the command from the remote client
    auto self(shared_from_this());
    asio::async_read(m_socket,
                     asio::buffer(&m_tempReadSize, sizeof(m_tempReadSize)),
                     [this, self](const asio::error_code& ec, size_t) {
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
    // Splitting big packets is not yet supported
    if (m_tempReadSize < 10 || m_tempReadSize > 4096) {
        Close(false);
    } else {
        m_tempReadPkg.m_stream.m_buf.resize(m_tempReadSize);

        auto self(shared_from_this());
        asio::async_read(m_socket,
            asio::buffer(m_tempReadPkg.m_stream.m_buf),
            [this, self](const asio::error_code& ec, size_t) {
            if (!ec) {
                m_recvQueue.push_back(std::move(m_tempReadPkg));
                ReadPacketSize();
            } else {
                Close(false);
            }
        });
    }
}

void RCONSocket::WritePacketSize() {
    auto&& bytes = m_sendQueue.front();
    m_tempWriteSize = bytes.size();

    auto self(shared_from_this());
    asio::async_write(m_socket,
                     asio::buffer(&m_tempWriteSize, sizeof(m_tempWriteSize)),
                     [this, self, &bytes](const asio::error_code& ec, size_t) {
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

    auto self(shared_from_this());
    asio::async_write(m_socket,
                      asio::buffer(bytes),
                      [this, self](const asio::error_code& ec, size_t) {
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