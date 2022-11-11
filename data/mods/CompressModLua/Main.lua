-- https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs
Valhalla.OnEvent("Rpc", "PeerInfo", "POST", function(rpc, pkg)
    print("Registering Compress Handshake")
    print("Got new socket!")
    print(rpc.socket:GetAddress())
    --rpc.Register("CompressHandshake", PkgType.BOOL, function(enabled)
    --    print("Got CompressHandshake: " .. enabled)
    --end)
end)
