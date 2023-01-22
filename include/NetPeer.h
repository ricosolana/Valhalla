#pragma once

#include "VUtils.h"
#include "NetSocket.h"
#include "NetRpc.h"
#include "ZDOPeer.h"

struct NetPeer {
public:
    std::unique_ptr<NetRpc> m_rpc;
    std::unique_ptr<ZDOPeer> m_zdoPeer;

    const OWNER_t m_uuid;
    const std::string m_name;

    // Constantly changing vars
    Vector3 m_pos;
    bool m_visibleOnMap = false;
    NetID m_characterID = NetID::NONE;

public:
    NetPeer(std::unique_ptr<NetRpc> rpc,
            OWNER_t uuid, const std::string &name)
        : m_rpc(std::move(rpc)), m_zdoPeer(std::make_unique<ZDOPeer>()), m_name(name), m_uuid(uuid)
    {}

    void Kick();
    void SendDisconnect();
    void Disconnect();
};
