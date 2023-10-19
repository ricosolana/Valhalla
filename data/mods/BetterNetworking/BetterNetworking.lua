--[[
    Implementation of the BetterNetworking mod for Valheim (3/23/2023)
    
    https://github.com/CW-Jesse/valheim-betternetworking

    Ported by crzi for use on the C++ Valhalla server
--]]

--require('mobdebug').start()

local COMPRESSION_VERSION = 6

local compressor = ZStdCompressor.new(VUtils.Resource.ReadFileBytes('mods/BetterNetworking/small'))
local decompressor = ZStdDecompressor.new(VUtils.Resource.ReadFileBytes('mods/BetterNetworking/small'))

local peers = {}

local SIG_CompressionVersion = MethodSig.new("CW_Jesse.BetterNetworking.CompressionVersion", Type.INT32)
local SIG_CompressionEnabled = MethodSig.new("CW_Jesse.BetterNetworking.CompressionEnabled", Type.BOOL)
local SIG_CompressionStarted = MethodSig.new("CW_Jesse.BetterNetworking.CompressedStarted", Type.BOOL)

local SendCompressionStarted = function(peer, started, status)
    if started ~= status.o then
        peer:invoke(SIG_CompressionStarted, started)
        status.o = started

        print('Compression to ' .. peer.name .. ': ' .. (started and 'true' or 'false'))
    end
end

local SendCompressionEnabled = function(peer, enabled, status)
    peer:invoke(SIG_CompressionEnabled, true)
    SendCompressionStarted(peer, status.enabled, status)
end

-- DUMMY SPECIAL LISTENER FOR TESTING LUA-INVOLVED RPC IO
--Valhalla:Subscribe('RpcOut', 'CW_Jesse.BetterNetworking.CompressionVersion', 'POST', function(peer, version)
--    print('zdo: ' .. (peer.zdo and 'yes' or 'nil'))
--    print('cool lua post works: ' .. peer.name .. ' ' .. version)
--end)

--Valhalla:Subscribe("Join", function(peer)
--Valhalla:Subscribe('RpcOut', 'ClientHandshake', 'POST', function(peer)
Valhalla:Subscribe('Connect', function(peer)
    --local status = peers[tostring(peer.socket.host)]
    local status = { enabled = false, i = false, o = false }
    
    peers[peer.socket.host] = status
    
    -- Version
    peer:Register(SIG_CompressionVersion, function(peer, version)
        if version < COMPRESSION_VERSION then
            print(peer.name .. ' (v' .. version .. ') is outdated')
        elseif version > COMPRESSION_VERSION then
            print(peer.name .. ' (v' .. version .. ') is newer than server')
        else
            print('Peer ' .. peer.socket.host .. ' compatible (v' .. version .. ')')
        
            SendCompressionEnabled(peer, status.enabled, status)
        end
        
        return false
    end)
    
    -- Enable
    peer:Register(SIG_CompressionEnabled, function(peer, enabled)
        status.enabled = enabled
        SendCompressionStarted(peer, enabled, status)
    end)
    
    -- Started
    peer:Register(SIG_CompressionStarted, function(peer, started)
        -- prepare to start receiving compressed packets
        status.i = started
        print('Compression from ' .. peer.name .. ': ' .. (started and 'true' or 'false'))
    end)
    
    print('Sending version...')
        
    peer:invoke(SIG_CompressionVersion, COMPRESSION_VERSION)
end)

Valhalla:Subscribe('Send', function(peer, bytes)
    -- compress data if peer is started
    local status = peers[peer.socket.host]
    
    if status and status.o then
        local com = compressor:Compress(bytes)
        if com then
            VUtils.Swap(bytes, com)
        else
            -- compression failed for an unknown reason, this is rare
            print('Compression failed; this is rare')
        end
    end
end)

Valhalla:Subscribe('Recv', function(peer, bytes)
    local decom = decompressor:Decompress(bytes)

    local host = peer.socket.host

    local status = peers[host]
    
    if decom then
        --print('got compressed message')
    
        VUtils.Swap(bytes, decom)
        
        --print('Recv swapped messages')
        if not status.i then
            print('Received unexpected compressed message from ' .. host)
            status.i = true
        end
    else
        if status.i then
            print('Received unexpected uncompressed message from ' .. host)
            status.i = false
        end
    end
end)

-- Remove dead references


Valhalla:Subscribe("Disconnect", function(peer)
    print(peer.name .. ' disconnected')
    peers[peer.socket.host] = nil
end)
