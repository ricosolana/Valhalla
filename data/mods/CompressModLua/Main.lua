-- https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

local compressHandshake = function(rpc, enabled)
    print("Got CompressHandshake")
    print(enabled)
end

Valhalla.OnEvent("Rpc", "PeerInfo", "POST", function(rpc, pkg)
    print("Registering CompressHandshake")
    rpc:Register("CompressHandshake", 6, PkgType.BOOL, compressHandshake)
end)
