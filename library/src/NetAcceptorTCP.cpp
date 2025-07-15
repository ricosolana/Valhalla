#include "Avledet.h"
#include "NetSocket.h"
#include "TcpAccepter.h"
#include <quill/LogMacros.h>

namespace avledet::network {

    std::unique_ptr<IAcceptor> IAcceptor::tcp_user()
    {
        return std::make_unique<TCPAcceptor>();
    }

    std::unique_ptr<IAcceptor> IAcceptor::tcp_dedicated(std::string bind_addr)
    {

        std::size_t pos = bind_addr.find(':');
        if (pos != std::string::npos && pos + 1 < bind_addr.length()) {
            // 0.0.0.0:		(7)
            // 0.0.0.0:1	(
            std::string ipString   = bind_addr.substr(0, pos);
            std::string portString = bind_addr.substr(pos + 1);// throws
            auto port              = std::stoi(portString);
            if (port < 0 || port > 65565) {
                throw std::runtime_error("invalid port");
            }
            return std::make_unique<TCPAcceptor>(asio::ip::tcp::endpoint(
                    asio::ip::address::from_string(ipString.c_str()), (asio::ip::port_type) port));
        }

        throw std::runtime_error("must specify port");
    }

    TCPAcceptor::TCPAcceptor() :
        TCPAcceptor(0)
    {
    }

    TCPAcceptor::TCPAcceptor(asio::ip::tcp::endpoint ep) :
        m_ctx(),
        m_acceptor(m_ctx, ep),
        m_is_listener(ep.port() != 0)
    {
    }

    TCPAcceptor::TCPAcceptor(asio::ip::address address, asio::ip::port_type port) :
        TCPAcceptor(asio::ip::tcp::endpoint(address, port))
    {
    }

    TCPAcceptor::TCPAcceptor(asio::ip::port_type port) :
        TCPAcceptor(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {
    }

    TCPAcceptor::~TCPAcceptor()
    {
        this->stop();
    }

    /*
    std::vector<char> TCPAcceptor::get_auth_session_ticket() {
        return {};
    }

    bool TCPAcceptor::verify_auth_session_ticket(std::span<const char> ticket, Socket::Ptr socket) {
        (void)ticket;
        (void)socket;

        return true;
    }*/

    std::string TCPAcceptor::resolve(std::string hostname, std::string service)
    {
        using asio::ip::tcp;

        // doesnt work

        tcp::resolver resolver(m_ctx);
        tcp::resolver::query query(hostname, service);
        tcp::resolver::iterator endpoints = resolver.resolve(query);

        for (auto itr = endpoints; itr != tcp::resolver::iterator(); ++itr) {
            auto myip = itr->endpoint().address().to_string();

            return myip;
        }

        return "";
    }

    //std::string TCPAcceptor::get_public_ip()
    //{
    //    // TODO implement or omit
    //    using asio::ip::tcp;

    //    //return this->resolve("myip.opendns.com", "208.67.222.222");

    //    //return this->resolve("myip.opendns.com", "resolver1.opendns.com");

    //    //return this->resolve("myip.opendns.com", this->resolve("resolver1.opendns.com"));

    //    return "";
    //}

    void TCPAcceptor::update()
    {
        assert(m_connect_callback && "Must assign an on_connect callback prior to update");

        std::scoped_lock scoped(m_mux);
        for (auto &&socket : m_queued) {
            m_connect_callback(std::move(socket));
        }
        m_queued.clear();
    }

    void TCPAcceptor::start()
    {
        // TODO I might have set this condition wrong
        assert(!m_thread.joinable());

        LOG_INFO(AVL_LOGGER, "Starting TCP listener on port {}", m_acceptor.local_endpoint().port());

        if (m_is_listener)
            this->do_accept();

        m_thread = std::jthread([this](std::stop_token token) {
            while (!token.stop_requested()) {
                m_ctx.run();
                m_ctx.restart();

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }
        });
    }

    void TCPAcceptor::stop()
    {
        m_thread.request_stop();
        m_ctx.stop();
    }

    ISocket::Ptr TCPAcceptor::connect(std::string address)
    {
        auto idx = address.find_last_of(':');
        // 'address:' or ':port'
        if (idx == address.size() - 1 || idx == 0) {
            throw std::runtime_error("invalid address");
        }

        // TODO use resolver for more competent resolving

        auto addr
                = asio::ip::address::from_string(idx != std::string::npos ? address.substr(0, idx) : address);

        int port_num = std::stoi(idx != std::string::npos ? address.substr(idx + 1) : address);
        if (port_num < 0 || port_num > std::numeric_limits<asio::ip::port_type>::max())
            throw std::runtime_error("invalid port");

        auto ep = asio::ip::tcp::endpoint(addr, static_cast<asio::ip::port_type>(port_num));
        auto socket = std::make_shared<TcpSocket>(asio::ip::tcp::socket(m_ctx), ep, true);

        socket->m_socket.async_connect(ep, [this, socket](asio::error_code const &ec) {
            if (!ec) {
                socket->do_read();
                std::scoped_lock scoped(m_mux);
                m_queued.push_back(socket);
            } else {
                //LOG_ERROR(m_logger, "Socket TCP Connect failed");
                socket->close(false);
            }
        });

        return socket;
    }

    void TCPAcceptor::on_connect(std::function<void(ISocket::Ptr)> callback)
    {
        m_connect_callback = callback;
    }

    /*
    std::shared_ptr<Socket> TCPAcceptor::accept() {
        std::scoped_lock scoped(m_mux);

        if (!m_queued.empty()) {
            std::shared_ptr<TCPSocket> sock = std::move(m_queued.front());
            m_queued.pop_front();
            sock->do_read();
            return sock;
        }

        return nullptr;
    }*/

    void TCPAcceptor::do_accept()
    {
        m_acceptor.async_accept([this](asio::error_code const &ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                // Construct first to avoid mutex blocking too long
                auto endpoint = socket.remote_endpoint();
                auto addr_str = endpoint.address().to_string();
                LOG_INFO(AVL_LOGGER, "Accepted socket {}", addr_str);
                auto ptr = std::make_shared<TcpSocket>(std::move(socket), endpoint, false);
                ptr->do_read();
                std::scoped_lock scoped(m_mux);// unique = write-only lock
                m_queued.push_back(std::move(ptr));
            } else {
                if (ec.value() == asio::error::operation_aborted) {
                    //LOG_INFO(LOGGER, "NetAccepter aborted");
                    return;
                } else {
                    //LOG_ERROR(LOGGER, "Failed to accept: "); // << ec.message() << " : " << ec.value();
                }
            }

            this->do_accept();
        });
    }

}// namespace avledet::network