--[[
    This is a port of the compress mod
    for Valheim. It is currently a
    proof of concept and should not
    be used until further testing (11/12/2022)

    by crzi
--]]

-- https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

--local zdoDataHash = VUtils.GetStableHashCode("ZDOData")
--local compressedZdoDataHash = VUtils.GetStableHashCode("CompressedZDOData")
local peers = {}

local RPC_CompressHandshake = function(peer, enabled)
    print("Got CompressHandshake, enabled: " .. enabled)

    --peers[peer] = enabled
    if enabled then
        peers[peer] = peer:GetMethod("ZDOData");
    end

    peer:Invoke("CompressHandshake", enabled);
end

local RPC_CompressedZDOData = function(peer, bytes)
    print("Got CompressedZDOData")

    bytes = VUtils.Decompress(bytes)

    -- now forward to RPC_ZDOData
    method = peers[peer]
    if method then
        method:Invoke(peer, bytes)
    end
end



Valhalla.OnEvent("PeerInfo", function(peer)
    print("Registering CompressHandshake in PeerInfo")

    print("peer: " .. to_string(peer))

    peer:Register("CompressZDOData", 
        DataType.bytes, RPC_CompressedZDOData
    )

    peer:Register("CompressHandshake", 
        DataType.bool, RPC_CompressHandshake
    )
end)

-- remove peer references
Valhalla.OnEvent("PeerQuit", function(peer)
    table.remove(peers, peer)
end)

-- delegate to replace call
Valhalla.OnEvent("RpcOut", "ZDOData", function(peer, pkg)
    if peers[peer] then
        this.event.Cancel()
        peer:Invoke("CompressedZDOData", VUtils.Compress(pkg))
    end
end)



Valhalla.OnEvent("Enable", function()
    print("Compress mod Enable() " .. this.mod.version)
end)
