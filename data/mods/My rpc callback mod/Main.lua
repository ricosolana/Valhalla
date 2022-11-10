Valhalla.OnEvent("Enable", function()
    print("Join mod enabled!")
end)

Valhalla.OnEvent("InRemoteCall", "PeerInfo", function(rpc, pkg)
    print("Lua caught PeerInfo: peer connected " .. rpc.socket:GetHostName())
end)
