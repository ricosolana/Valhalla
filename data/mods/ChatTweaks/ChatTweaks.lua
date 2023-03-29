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

local SIG_CompressionVersion = MethodSig.new("CW_Jesse.BetterNetworking.CompressionVersion", Type.INT32)
local SIG_CompressionEnabled = MethodSig.new("CW_Jesse.BetterNetworking.CompressionEnabled", Type.BOOL)
local SIG_CompressionStarted = MethodSig.new("CW_Jesse.BetterNetworking.CompressedStarted", Type.BOOL)

Valhalla:Subscribe('RouteInAll', 'ChatMessage', function(peer, _, params)
    local pos = params:ReadVector3()
    local msgtype = params:ReadInt32()
    local profile = params:ReadProfile()
    local msg = params:ReadString()
    local nid = params:ReadString()
    
    print('got msg: ' .. msg)
    
    if msgtype == ChatMsgType.SHOUT then
        -- then set the pos to 0,0,0 then display as a simple chat message
        local writer = DataWriter.new(params)
        writer:Write(Vector3.ZERO)
        writer:WriteInt32(ChatMsgType.NORMAL)
        writer:Write(profile)
        writer:Write(msg)
        writer:Write(nid)
    end
end)

Valhalla:Subscribe('PlayerList', function()
    -- can do something here
    
    -- maybe guild-faction based visibility
    -- players will be visible only to others in their 'guild'

    --for i, peer in ipairs(NetManager.peers) do
    --    peer.visibleOnMap = true
    --end
    --
    --
    --
    --return false
end)
