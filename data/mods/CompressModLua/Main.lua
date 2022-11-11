-- https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs
Valhalla.OnEvent("Rpc", "PeerInfo", "POST", function(rpc, pkg)
    print("Registering CompressHandshake")
    rpc:Register("CompressHandshake", PkgType.BOOL, function(enabled)
        print("Got CompressHandshake: " .. enabled)
    end)
end)
