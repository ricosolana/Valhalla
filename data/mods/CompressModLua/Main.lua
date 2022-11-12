-- https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs



local compressHandshake = function(rpc, enabled)
    print("Got CompressHandshake")
    print(enabled)
    rpc:Invoke("CompressHandshake", enabled);
end

Valhalla.OnEvent("Rpc", "PeerInfo", "POST", function(rpc, pkg)
    print("Registering CompressHandshake")
    rpc:Register("CompressHandshake", PkgType.BOOL, compressHandshake)
end)
