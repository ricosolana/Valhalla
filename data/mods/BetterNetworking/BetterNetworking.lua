--[[
    Implementation of the BetterNetworking mod for Valheim (3/23/2023)
    
    https://github.com/CW-Jesse/valheim-betternetworking

    Ported by crzi for use on the C++ Valhalla server
--]]

local COMPRESSION_VERSION = 6

local compressor = Compressor.new(VUtils.Resource.ReadFileBytes('mods/BetterNetworking/small'))

local decompressor = Decompressor.new(VUtils.Resource.ReadFileBytes('mods/BetterNetworking/small'))

local peers = {}

local SIG_CompressionVersion = MethodSig.new("CW_Jesse.BetterNetworking.CompressionVersion", Type.INT32)
local SIG_CompressionEnabled = MethodSig.new("CW_Jesse.BetterNetworking.CompressionEnabled", Type.BOOL)
local SIG_CompressionStarted = MethodSig.new("CW_Jesse.BetterNetworking.CompressedStarted", Type.BOOL)

local SendCompressionStarted = function(peer, started, status)
    if started ~= status.sending then
        peer:Invoke(SIG_CompressionStarted, started)
        status.sending = started
        
        print('Compression to ' .. peer.name .. ': ' .. (started and 'true' or 'false'))
    end
end

local SendCompressionEnabled = function(peer, enabled, status)
    peer:Invoke(SIG_CompressionEnabled, true)
    SendCompressionStarted(peer, status.enabled, status)
end



Valhalla:Subscribe("Join", function(peer)
    local status = { enabled = false, receiving = false, sending = false }
    peers[tostring(peer.uuid)] = status

    -- Handshake for each request is same time, but semi-sequential
    -- Version -> Enabled -> Started
    
    -- Version listener
    peer:Register(SIG_CompressionVersion, function(peer, version)
        if version == COMPRESSION_VERSION then
            
            -- Enable listener
            peer:Register(SIG_CompressionEnabled, function(peer, enabled)
                status.enabled = enabled
                SendCompressionStarted(peer, enabled, status)                
            end)
            
            -- Started listener
            peer:Register(SIG_CompressionStarted, function(peer, started)
                status.started = started
                print('Compression from ' .. peer.name .. ': ' .. (started and 'true' or 'false'))
            end)
            
            SendCompressionEnabled(peer, status.enabled, status)
            SendCompressionStarted(peer, status.sending, status)
        end
    end)
        
    peer:Invoke(SIG_CompressionVersion, COMPRESSION_VERSION)
end)

-- Remove dead references
Valhalla:Subscribe("Quit", function(peer)
    print(peer.name .. ' disconnected')
    peers[tostring(peer.uuid)] = nil
end)

-- delegate to replace call
Valhalla:Subscribe("RpcOut", "ZDOData", function(peer, bytes)
    --local uuid = peer.uuid
    --if peers[uuid] ~= nil then
    --if peers[peer] then
    if peers[tostring(peer.uuid)] then
        event.Cancel() -- To prevent normal packet from being sent
        local compressed = assert(VUtils.Compress(bytes), 'compression error')
        peer:Invoke(SIG_CompressedZDOData, compressed)
    end
end)

Valhalla:Subscribe("Enable", function()
    
end)
