--[[
    Vanilla portal mod to connect portals together throughout gameplay
    
        This is the raw View-less version (utilizes ZDOs get/set only)

    Created by crzi for use on the C++ Valhalla server

    I never knew this, but tags are not fully unique
        Once a portal is paired with another with a certain tag,
        another pair of portals can be paired with the same tag as the first pair

        my code below also exhibits this, (un)fortunately? so.
--]]
local const
SIG_RPC_SetConnection = MethodSig.new("RPC_SetConnection", Type.ZDOID, Type.ZDOID)

-- todo rename...
local ConnectionType = ConnectorType

-- stored ZDOs in connected pairs
local connecting_portals = {}

local is_connecting = function(zdo)
    for _, paired in ipairs(connecting_portals) do
        if zdo == paired[1] or zdo == paired[2] then
            return true
        end
    end
    return false
end

local set_connection = function(portal, connection, force)
    local flag = NetManager:get_peer(portal.owner)
    if not portal.owned or not flag or force then
        portal.owner = Avledet.id
        portal:set_connection(ConnectionType.PORTAL, connection)
        ZDOManager:force_send_zdo(portal.id)
    else
        --ZRoutedRpc.instance.InvokeRoutedRPC(owner, "RPC_SetConnection", new object[] { portal.m_uid, connection })
        RouteManager:invoke(portal.owner, SIG_RPC_SetConnection, portal.id, connection)
    end
end

local force_set_connection = function(portal, connection)
    if portal:get_connection(ConnectionType.PORTAL) ~= connection then
        set_connection(portal, connection, true)
    end
end

local clear_connecting = function()
    for _, paired in ipairs(connecting_portals) do
        force_set_connection(paired[1], paired[2].id)
        force_set_connection(paired[2], paired[1].id)
    end
    connecting_portals = {} -- effective clear
end

local add_connecting = function(portalA, portalB)
    table.insert(connecting_portals, {portalA, portalB})
end

local find_any_portal = function(portals, ignore_zdo, tag)
    local list = {}
    for _, zdo in ipairs(portals) do
        if
            zdo ~= ignore_zdo and zdo:get_string("tag") == tag and
                zdo:get_connection(ConnectionType.PORTAL) == ZDOID.NONE and
                not is_connecting(zdo)
         then
            table.insert(list, zdo)
        end
    end

    if #list == 0 then
        return nil
    end

    --ensure index within range
    return assert(list[Random.new():irange(0, #list) + 1])
end

local RPC_SetConnection = function(sender, portalID, connectionID)
    local zdo = ZDOManager:get_zdo(portalID)
    if zdo then
        zdo.is_local = true
        assert(zdo.is_owner(Avledet.id))
        zdo:set_connection(ConnectionType.PORTAL, connectionID)
        ZDOManager:force_send_zdo(portalID)
    end
end

Avledet:subscribe(
    "Periodic",
    function()
        clear_connecting()

        local portals = ZDOManager:get_zdos("portal_wood")

        for _, zdo in ipairs(portals) do
            local connectionZDOID = zdo:get_connection(ConnectionType.PORTAL)

            if connectionZDOID ~= ZDOID.NONE then
                local tag = zdo:get_string("tag")
                local zdo2 = ZDOManager:get_zdo(connectionZDOID)
                if not zdo2 or zdo2:get_string("tag") ~= tag then
                    set_connection(zdo, ZDOID.NONE --[[, false--]])
                end
            end
        end

        local num = 0
        for _, zdo3 in ipairs(portals) do
            if not is_connecting(zdo3) and zdo3:get_connection(ConnectionType.PORTAL) == ZDOID.NONE then
                local zdo4 = find_any_portal(portals, zdo3, zdo3:get_string("tag"))
                if zdo4 then
                    add_connecting(zdo3, zdo4)
                    set_connection(zdo3, zdo4.id --[[, false--]])
                    set_connection(zdo4, zdo3.id --[[, false--]])

                    print("Connected portals", zdo3, "<->", zdo4)

                    num = num + 1
                end
            end
        end

        if num > 0 then
            print("[", "Connected", num, "portals", "]")
        end
    end
)
