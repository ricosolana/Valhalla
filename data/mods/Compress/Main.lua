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

local compressHandshake = function(rpc, enabled)
    print("Got CompressHandshake, enabled: " .. enabled)

    if (enabled) then peers[rpc] = enabled end

    rpc:Invoke("CompressHandshake", enabled);
end

Valhalla.OnEvent("RpcIn", "PeerInfo", "POST", function(rpc, pkg)
    print("Registering CompressHandshake")
    rpc:Register("CompressHandshake", PkgType.BOOL, compressHandshake)
end)

-- remove rpc references
Valhalla.OnEvent("PeerQuit", function(peer)
    table.remove(peers, peer.rpc)
end)

Valhalla.OnEvent("RpcOut", "ZDOData", function(rpc, method, pkg)
    if peers[rpc] then
        This.Event.Cancel()
        rpc:Invoke("CompressedZDOData", VUtils.Compress(pkg))
    end
end)

Valhalla.OnEvent("RpcIn", "CompressedZDOData", function(rpc, pkg)
    -- uncompress and forward the data

    pkg.buf = VUtils.Decompress(pkg.buf)
end)
