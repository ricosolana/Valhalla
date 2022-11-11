#include "NetAcceptor.h"
#include "VServer.h"

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

std::optional<ISocket::Ptr> RCONAcceptor::Accept() {
    OPTICK_EVENT();
    std::scoped_lock<std::mutex> scope(m_mut);
    auto &&pair = m_connected.begin();
    if (pair == m_connected.end())
        return std::nullopt;
    auto socket = *pair;
    auto itr = m_connected.erase(pair);
    return socket;
}



void RCONAcceptor::DoAccept() {
    m_acceptor.async_accept(
            [this](const asio::error_code& ec, tcp::socket socket) {

        if (!ec) {
            std::scoped_lock<std::mutex> scope(m_mut);
            m_connected.insert(std::make_shared<RCONSocket>(std::move(socket)));
        }
        else if (ec.value() == asio::error::operation_aborted) {
            return;
        }

        DoAccept();
    });
}
