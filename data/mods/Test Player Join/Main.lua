--[[
    This program catches when a fully 
    authorized player is about to be
    admitted into the server (11/12/2022)

    by crzi
--]]

Valhalla.OnEvent("PeerConnect", function(rpc, uuid, name, version)
    print("Hello " .. name .. " (" .. rpc.socket:GetAddress() .. ")")
end)
