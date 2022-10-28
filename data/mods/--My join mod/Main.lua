onEnable = function()
    print("My join mod onEnable!")
end

onPeerInfo = function(rpc, uuid, name, version)
    print("testing join: " .. uuid .. " " .. name .. " " .. version)
    -- https://stackoverflow.com/a/4911217
    --print("player ip: " .. rpc:socket:GetHostName())
    print("player ip: " .. rpc.socket:GetHostName())
    return true
end