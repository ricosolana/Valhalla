--[[
    Hides the 'I HAVE ARRIVED!' ping and other chat settings
    
    Features:
        - Catch and dispose arrive ping
        - Anonymous global messages (without giving away player locations)
        - Force enable or disable player map locations
--]]

-- zerobrane studio debugging
--  zerobrane keeps bugging out for some reason, asking me to save a file every time
--require('mobdebug').start()

local peers = {}

Valhalla:Subscribe('RouteInAll', 'ChatMessage', function(peer, _, params)
    -- grab a writer beforehand so position is maintained
    if peers[peer.socket.host] then
        return
    end
    
    peers[peer.socket.host] = true

    local pos = params:ReadVector3f()
    local msgtype = params:ReadInt32()
    local profile = params:ReadProfile()
    local msg = params:ReadString()
    local nid = params:ReadString()
    
    if msgtype == ChatMsgType.SHOUT then
        return false
    end
end)

Valhalla:Subscribe('Quit', function(peer)
    peers[peer.socket.host] = nil
end)
