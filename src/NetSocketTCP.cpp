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
    
    m_sendQueueSize += bytes.size();

    const BYTES_t* ref;

    bool was_empty;
    {
        std::scoped_lock scoped(m_mux);

        was_empty = m_send.empty();
        m_send.push_back(std::move(bytes));
        ref = &m_send.front();
    }

    // (Re)initiate writers
    if (was_empty) {
        WritePkgSize(ref);
    }
}

std::optional<BYTES_t> TCPSocket::Recv() {    
    std::scoped_lock scoped(m_mux);

    if (!m_recv.empty()) {
        BYTES_t bytes = std::move(m_recv.front());
        m_recv.pop_front();
        return bytes;
    }

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



void TCPSocket::WritePkgSize(const BYTES_t* bytes) {
    // https://stackoverflow.com/questions/8640393/move-capture-in-lambda
    
    m_tempWriteOffset = bytes->size();

    auto self(shared_from_this());
    asio::async_write(m_socket,
        asio::buffer(&m_tempWriteOffset, sizeof(m_tempWriteOffset)),
        [this, self, bytes](const std::error_code& ec, size_t) {
            if (!ec) {
                // Call writepkg
                WritePkg(bytes);
            }
            else {
                Close(false);
            }
        }
    );
}

void TCPSocket::WritePkg(const BYTES_t* bytes) {
    auto self(shared_from_this());
    asio::async_write(m_socket,
        asio::buffer(bytes->data(), m_tempWriteOffset),
        [this, self](const std::error_code& ec, size_t) {
            if (!ec) {
                m_sendQueueSize -= m_tempWriteOffset;

                const BYTES_t* ref;

                {
                    std::scoped_lock scoped(m_mux);
                    m_send.pop_front();
                    
                    if (m_send.empty()) {
                        // The assert might fail in cases where 
                        //  the scoped_lock cannot protect (because sendQueue is updated on its own)
                        //  in Send()
                        // m_sendQueueSize is its own atomic in order to block the queue mutex less often
                        //assert(!m_sendQueueSize);
                        return;
                    }
                    else {
                        ref = &m_send.front();
                        //assert(m_sendQueueSize);
                    }
                }

                WritePkgSize(ref);
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
