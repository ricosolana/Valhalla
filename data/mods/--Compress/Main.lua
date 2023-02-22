--[[
    Implementation of the compress mod originally used on the Comfy Valheim server (2/9/2023)
    https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

    Ported by crzi for use on the C++ Valhalla server
--]]

local peers = {}
local SIG_CompressedZDOData = MethodSig.new("CompressedZDOData", DataType.bytes)
local SIG_CompressHandshake = MethodSig.new("CompressHandshake", DataType.bool)

local RPC_CompressedZDOData = function(peer, compressed)
    -- Decompress raw data sent from client (intended for direct forwarding to RPC_ZDOData)
    local decompressed = VUtils.Decompress(compressed);

    -- This is necessary because method invoke treats the package as a sub package...
    compressed:clear() -- sol container clear
    local writer = DataWriter.new(compressed);
    writer:Write(decompressed)

    -- Finally invoke it
    peer:InvokeSelf("ZDOData", DataReader.new(compressed));
end

local RPC_CompressHandshake = function(peer, enabled)
    print("Got CompressHandshake")

    if enabled then
        print("Registering CompressedZDOData")

        peers[peer.uuid] = true

        peer:Register(SIG_CompressedZDOData, RPC_CompressedZDOData)

        peer:Invoke(SIG_CompressHandshake, enabled);
    end
end

Valhalla.OnEvent("PeerInfo", function(peer)
    print("Registering CompressHandshake")
    
    peer:Register(SIG_CompressHandshake, RPC_CompressHandshake)
end)

-- Remove dead references
Valhalla.OnEvent("PeerQuit", function(peer)
    print("Cleaning up peer")
    table.remove(peers, peer.uuid)
end)

-- delegate to replace call
Valhalla.OnEvent("RpcOut", "ZDOData", function(peer, bytes)
    local uuid = peer.uuid
    if peers[uuid] ~= nil then
        event.Cancel() -- To prevent normal packet from being sent
        local compressed = VUtils.Compress(bytes)
        peer:Invoke(SIG_CompressedZDOData, compressed)
    end
end)

Valhalla.OnEvent("Enable", function()
    print(this.name .. " " .. this.version .. " enabled")
end)
