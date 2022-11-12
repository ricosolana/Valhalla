-- https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

local peers = {}

local preSendZDOsInvoke = function(rpc, method, pkg)
    if peers[rpc] then
        This.Event.Cancel()
        -- now compress the data

    end
end

local compressHandshake = function(rpc, enabled)
    print("Got CompressHandshake")
    print(enabled)

    if (enabled) then peers[rpc] = enabled end

    rpc:Invoke("CompressHandshake", enabled);

    -- now register preSendZDOs to catch zdo calls, and only cancel the post
    -- event if the peer is using compress mod
end

Valhalla.OnEvent("Rpc", "PeerInfo", "POST", function(rpc, pkg)
    print("Registering CompressHandshake")
    rpc:Register("CompressHandshake", PkgType.BOOL, compressHandshake)
end)
