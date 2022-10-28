Valhalla.OnEnable(function()
    print("Join mod enabled!")
end)

Valhalla.OnRpc("PeerInfo", function(rpc, pkg)
    print("Lua caught PeerInfo: peer connected " .. rpc.socket:GetHostName())
end)
