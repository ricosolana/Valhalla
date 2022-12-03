#include "NetAcceptor.h"
#include "ValhallaServer.h"

using asio::ip::tcp;

RCONAcceptor::RCONAcceptor(uint16_t port)
    : m_ctx(),
    m_acceptor(m_ctx,
               tcp::endpoint(tcp::v4(), port)){}

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

    // The objective is to validate any sockets in here before theyre actually accepted
    // test m_connecting if theyve entered password correctly

    for (auto&& itr = m_connected.begin(); 
        itr != m_connected.end();) {

        auto sock = (*itr);

        if (sock->Connected()) {
            // poll logins
            if (auto&& opt = sock->RecvMsg()) {
                auto&& msg = opt.value();
                if (msg.msg == SERVER_SETTINGS.rconPassword)
                    sock->m_id = msg.client;

                sock->SendMsg(" ");

                itr = m_connected.erase(itr);
                if (sock->m_id != -1)
                    return sock;
                else
                    sock->Close(true);
            } else
                ++itr;
        } else 
            itr = m_connected.erase(itr);
    }

    return std::nullopt;
}



void RCONAcceptor::DoAccept() {
    m_acceptor.async_accept(
            [this](const asio::error_code& ec, tcp::socket socket) {

        if (!ec) {
            std::scoped_lock<std::mutex> scope(m_mut);
            auto ptr(std::make_shared<RCONSocket>(std::move(socket)));
            ptr->ReadPacketSize(); // start reading
            m_connected.push_back(std::move(ptr)); // dont copy any shared ptrs
        }
        else if (ec.value() == asio::error::operation_aborted) {
            return;
        }

        DoAccept();
    });
}
