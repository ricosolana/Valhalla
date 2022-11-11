Valhalla.OnEvent("Enable", function()
    print(Mod.name .. " enabled!")
end)

Valhalla.OnEvent("Rpc", "PeerInfo", function(rpc, pkg)
    print("Lua caught PeerInfo: peer connected " .. rpc.socket:GetHostName())
end)
