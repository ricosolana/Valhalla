--[[
    Implementation of the compress mod originally used on the Comfy Valheim server (2/9/2023)
    https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

    Ported by crzi for use on the C++ Valhalla server
--]]

local peers = {}
local SIG_CompressedZDOData = MethodSig.new("CompressedZDOData", Type.BYTES)
local SIG_CompressHandshake = MethodSig.new("CompressHandshake", Type.BOOL)

local compressor = GzCompressor.new()
local decompressor = GzDecompressor.new()

local RPC_CompressedZDOData = function(peer, compressed)
    -- Decompress raw data sent from client (intended for direct forwarding to RPC_ZDOData)
    local decompressed = assert(decompressor:Decompress(compressed), 'decompression error');

    -- This is necessary because method invoke treats the package as a sub package...
    compressed:clear() -- sol container clear
    local writer = DataWriter.new(compressed);
    writer:write(decompressed)

    -- Finally invoke it
    peer:InvokeSelf("ZDOData", DataReader.new(compressed));
end

local RPC_CompressHandshake = function(peer, enabled)
    print("Got CompressHandshake")

    if enabled then
        print("Registering CompressedZDOData")

        peers[tostring(peer.socket.host)] = true

        peer:Register(SIG_CompressedZDOData, RPC_CompressedZDOData)

        peer:invoke(SIG_CompressHandshake, enabled);
    end
    
    return false
end

Valhalla:Subscribe("Join", function(peer)
    print("Registering CompressHandshake")
    
    peer:Register(SIG_CompressHandshake, RPC_CompressHandshake)
end)

-- Remove dead references
Valhalla:Subscribe("Quit", function(peer)
    print("Disconnecting peer")
    peers[tostring(peer.socket.host)] = nil
end)

-- delegate to replace call
Valhalla:Subscribe("RpcOut", "ZDOData", function(peer, bytes)
    if peers[tostring(peer.socket.host)] then
        event.Cancel() -- To prevent normal packet from being sent
        local compressed = assert(compressor:Compress(bytes), 'compression error')
        peer:invoke(SIG_CompressedZDOData, compressed)
    end
end)

Valhalla:Subscribe("Enable", function()
    print(this.name .. " " .. this.version .. " enabled")
end)
