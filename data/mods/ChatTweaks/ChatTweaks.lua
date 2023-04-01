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
    
    local writer = DataWriter.new(params.provider)

    local pos = params:ReadVector3f()
    local msgtype = params:ReadInt32()
    local profile = params:ReadProfile()
    local msg = params:ReadString()
    local nid = params:ReadString()
    
    --print('got msg: ' .. msg)
    
    if msgtype == ChatMsgType.SHOUT then
        -- then set the pos to 0,0,0 then display as a simple chat message
        writer:Write(Vector3f.ZERO)
        writer:WriteInt32(ChatMsgType.NORMAL)
        writer:Write(profile)
        writer:Write(msg)
        writer:Write(nid)
    end
end)

Valhalla:Subscribe('Quit', function(peer)
    peers[peer.socket.host] = nil
end)
