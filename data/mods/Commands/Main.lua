--[[
    Vanilla portal mod to connect portals together throughout gameplay

    Created by crzi for use on the C++ Valhalla server

    I never knew this, but tags are not fully unique
        Once a portal is paired with another with a certain tag,
        another pair of portals can be paired with the same tag as the first pair

        my code below also exhibits this, (un)fortunately? so.
--]]

local RPC_vha = function(peer, cmd, args)
    print("Got command " .. cmd)
end

Valhalla.OnEvent("PeerInfo", function(peer)

    print("Registering vha")

    peer.Register("vha",
        DataType.string, DataType.strings,
        RPC_vha
    )

    print("Registered vha")

end)
