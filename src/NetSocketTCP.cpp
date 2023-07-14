#include "NetSocket.h"
#include "NetManager.h"

TCPSocket::TCPSocket(asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)) {
    m_connected = true;
}

TCPSocket::~TCPSocket() {
    Close(true);
}

void TCPSocket::Close(bool flush) {
    if (m_connected) {
        m_connected = false;
        m_socket.close();
    }
}

void TCPSocket::Update() {}

void TCPSocket::Send(BYTES_t bytes) {
    assert(!bytes.empty() && "Should try to avoid sending empty packets");
    
    {
        m_sendQueueSize += bytes.size();
        std::scoped_lock scoped(m_mux);
        // If not empty the writers are presumably active (and busy)
        if (!this->m_send.empty()) {
            this->m_send.push_back(std::move(bytes));
            return;
        }
    }

    WritePkgSize(std::move(bytes));
}

std::optional<BYTES_t> TCPSocket::Recv() {
    std::scoped_lock scoped(m_mux);

    if (!m_recv.empty()) {
        BYTES_t bytes = std::move(m_recv.front());
        m_recv.pop_front();
        return bytes;
    }

    //auto&& begin = m_recv.begin();
    //if (begin != m_recv.end()) {
    //    BYTES_t bytes = std::move(*begin);
    //    m_recv.erase(begin);
    //    return bytes;
    //}
    return std::nullopt;
}

std::string TCPSocket::GetHostName() const {
    return "host";
}

std::string TCPSocket::GetAddress() const {
    return "addr";
}

bool TCPSocket::Connected() const {
    return m_connected;
}

unsigned int TCPSocket::GetSendQueueSize() const {

    return m_sendQueueSize;
}

unsigned int TCPSocket::GetPing() const {
    return 0;
}



void TCPSocket::ReadPkgSize() {
    auto self(shared_from_this());
    asio::async_read(m_socket,
        asio::buffer(&m_tempReadOffset, sizeof(m_tempReadOffset)),
        [this, self](const std::error_code& ec, size_t) {
            if (!ec) {
                ReadPkg();
            }
            else {
                Close(false);
            }
        }
    );
}

void TCPSocket::ReadPkg() {
    // Max of ~10.48 Mb per packet
    if (m_tempReadOffset == 0 || m_tempReadOffset > 0xA00000) {
        Close(false);
    }
    else {
        m_tempReadBytes.resize(m_tempReadOffset);

        auto self(shared_from_this());
        asio::async_read(m_socket,
            asio::buffer(m_tempReadBytes.data(), m_tempReadOffset),
            [this, self](const std::error_code& ec, size_t read) {
                if (!ec) {
                    {
                        assert(read == m_tempReadOffset);

                        // tempReadBytes seems to be empty sometimes
                        //  when skipping resize (dont know why, asio::buffer auto resize the vector)

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



void TCPSocket::WritePkgSize(BYTES_t bytes) {
    // https://stackoverflow.com/questions/8640393/move-capture-in-lambda
    m_tempWriteOffset = bytes.size();
    m_tempWriteBytes = std::move(bytes);
    //std::swap(bytes, m_tempWriteBytes); // faster than move

    auto self(shared_from_this());
    asio::async_write(m_socket,
        asio::buffer(&m_tempWriteOffset, sizeof(m_tempWriteOffset)),
        [this, self](const std::error_code& ec, size_t) {
            if (!ec) {
                // Call writepkg
                WritePkg();
            }
            else {
                Close(false);
            }
        }
    );
}

void TCPSocket::WritePkg() {
    auto self(shared_from_this());
    asio::async_write(m_socket,
        asio::buffer(m_tempWriteBytes.data(), m_tempWriteOffset),
        [this, self](const std::error_code& ec, size_t) {
            if (!ec) {
                BYTES_t bytes;

                {
                    m_sendQueueSize -= m_tempWriteOffset;
                    std::scoped_lock scoped(m_mux);
                    if (!m_send.empty()) {
                        assert(m_sendQueueSize);
                        bytes = std::move(m_send.front()); // faster than swap (bytes is empty)
                        m_send.pop_front();
                    }
                    else {
                        assert(!m_sendQueueSize);
                        return;
                    }
                }

                WritePkgSize(std::move(bytes));
            }
            else {
                Close(false);
            }
        }
    );
}

void TCPSocket::Connect(asio::ip::tcp::endpoint ep) {
    //m_socket = asio::ip::tcp::socket(NetManager()->m_ctx);

    auto self(shared_from_this());
    m_socket.async_connect(ep, [this, self](const asio::error_code& ec) {
        if (!ec) {
            ReadPkgSize();
        }
        else {
            LOG_ERROR(LOGGER, "Socket TCP Connect failed");
            Close(false);
        }
    });
}
