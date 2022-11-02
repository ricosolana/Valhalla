#include "NetAcceptor.h"
#include "ValhallaServer.h"

using asio::ip::tcp;

RCONAcceptor::RCONAcceptor()
    : m_ctx(),
    m_acceptor(m_ctx,
               tcp::endpoint(tcp::v4(),
                                    Valhalla()->Settings().rconPort)){}

RCONAcceptor::~RCONAcceptor() noexcept {
    m_ctx.stop();
    if (m_thread.joinable())
        m_thread.join();
}

void RCONAcceptor::Listen() {
    DoAccept();

    m_thread = std::thread([this]() {
        el::Helpers::setThreadName("rcon");
        m_ctx.run();
    });
}

ISocket* RCONAcceptor::Accept() {
    std::scoped_lock<std::mutex> scope(m_mut);
    auto pair = m_connected.begin();
    if (pair == m_connected.end())
        return nullptr;
    auto socket = *pair;
    auto itr = m_connected.erase(pair);
    return socket;
}

void RCONAcceptor::Cleanup(ISocket *socket) {
    auto* rconSocket = dynamic_cast<RCONSocket*>(socket);
    assert(rconSocket && "Received a non rcon socket in RCONAcceptor!");

    std::scoped_lock<std::mutex> scope(m_mut);
    m_sockets.erase(rconSocket);
}



void RCONAcceptor::DoAccept() {
    m_acceptor.async_accept(
            [this](const asio::error_code& ec, tcp::socket socket) {

        if (!ec) {
            auto rcon = std::make_unique<RCONSocket>(std::move(socket));
            {
                std::scoped_lock<std::mutex> scope(m_mut);
                m_connected.insert(rcon.get());
                m_sockets.insert({ rcon.get(), std::move(rcon)});
            }
        }
        else if (ec.value() == asio::error::operation_aborted) {
            return;
        }

        DoAccept();
    });
}




