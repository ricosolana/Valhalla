#include "RCONManager.h"
#include "ModManager.h"

void RCONManager::Init(std::string password, uint16_t port) {
    this->m_password = std::move(password);

    if (m_password.empty())
        m_password = "mysecret";

    LOG(INFO) << "Enabling RCON on port " << port;
    LOG(INFO) << "RCON password is '" << m_password << "'";
    m_rcon = std::make_unique<RCONAcceptor>(port);
    m_rcon->Listen();
}

void RCONManager::UnInit() {
    assert(m_rcon);

    m_rconSockets.clear();
    m_rcon.reset();
}

void RCONManager::Update() {
    assert(m_rcon);

    OPTICK_EVENT("Rcon");
    // TODO add cleanup
    //  also consider making socket cleaner easier to look at and understand; too many iterator / loops
    while (auto opt = m_rcon->Accept()) {
        auto&& rconSocket = std::static_pointer_cast<RCONSocket>(opt.value());

        m_rconSockets.push_back(rconSocket);
        LOG(INFO) << "Rcon accepted " << rconSocket->GetAddress();
        CALL_EVENT(EVENT_HASH_RconConnect, rconSocket);
    }

    // Authed sockets
    for (auto&& itr = m_rconSockets.begin(); itr != m_rconSockets.end();) {
        auto&& rconSocket = *itr;
        if (!rconSocket->Connected()) {
            LOG(INFO) << "Rcon disconnected " << rconSocket->GetAddress();
            CALL_EVENT(EVENT_HASH_RconDisconnect, rconSocket);
            itr = m_rconSockets.erase(itr);
        }
        else {
            rconSocket->Update();

            // receive the msg not pkg
            while (auto opt = rconSocket->RecvMsg()) {
                auto&& msg = opt.value();
                auto args = VUtils::String::Split(msg.msg, " ");
                if (args.empty())
                    continue;

                LOG(INFO) << "Got command " << msg.msg;

                HASH_t cmd = __H(args[0]);

                // Lua global handler
                CALL_EVENT(EVENT_HASH_RconIn, rconSocket, args);

                // Lua specific handler
                CALL_EVENT(EVENT_HASH_RconIn ^ cmd, rconSocket, args);

                /*
                if ("ban" == args[0] && args.size() >= 2) {
                    auto&& pair = m_banned.insert(std::string(args[1]));
                    if (pair.second)
                        rconSocket->SendMsg("Banned " + std::string(args[1]));
                    else
                        rconSocket->SendMsg(std::string(args[1]) + " is already banned");
                }
                else if ("kick" == args[0] && args.size() >= 2) {
                    auto id = (OWNER_t)std::stoll(std::string(args[1]));
                    auto peer = NetManager::GetPeer(id);
                    if (peer) {
                        peer->Kick();
                        rconSocket->SendMsg("Kicked " + peer->m_name);
                    }
                    else {
                        rconSocket->SendMsg(peer->m_name + " is not online");
                    }
                }
                else if ("list" == args[0]) {
                    auto&& peers = NetManager::GetPeers();
                    if (!peers.empty()) {
                        for (auto&& peer : peers) {
                            rconSocket->SendMsg(peer->m_name + " " + peer->m_rpc->m_socket->GetAddress()
                                + " " + peer->m_rpc->m_socket->GetHostName());
                        }
                    }
                    else {
                        rconSocket->SendMsg("There are no peers online");
                    }
                }
                else if ("stop" == args[0]) {
                    rconSocket->SendMsg("Stopping server...");
                    RunTaskLater([this](Task&) {
                        Stop();
                        }, 5s);
                }
                else if ("quit" == args[0] || "exit" == args[0]) {
                    rconSocket->SendMsg("Closing Rcon connection...");
                    rconSocket->Close(true);
                }
                else {
                    rconSocket->SendMsg("Unknown command");
                }*/


            }

            ++itr;
        }
    }
}