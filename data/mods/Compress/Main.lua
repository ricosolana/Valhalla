--[[
    This is a port of the compress mod
    for Valheim. It is currently a
    proof of concept and should not
    be used until further testing (11/12/2022)

    by crzi
--]]

-- https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

--local zdoDataHash = VUtils.GetStableHashCode("ZDOData")
--local compressedZdoDataHash = VUtils.GetStableHashCode("CompressedZDOData")
local peers = {}

local RPC_CompressedZDOData = function(peer, compressed)
    print("Got CompressedZDOData")

    -- now forward to RPC_ZDOData
    --print("type of peer: " .. type(peer))

    --local method = peers[peer]
    --print("type of method: " .. typeof(method))

    --if peers[peer.uuid] ~= nil then
        --print("Decompressing ZDOs from peer...")

        --local reader = DataReader.new(pkg)

        print("Reading compressed buffer...")

        --local compressed = reader:Read(DataType.bytes)

        print("Decompressing buffer...")

        local decompressed = VUtils.Decompress(compressed);
        --if decompressed ~= nil then
            --local reader = DataReader.new(decompressed)
            print("forwarding decompressed ZDOs from peer...")

            compressed:clear() -- sol container clear
            local writer = DataWriter.new(compressed);
            writer:Write(decompressed)

            peer:InvokeSelf("ZDOData", DataReader.new(compressed));
            --method:Invoke(peer, VUtils.Decompress(bytes))
            print("ZDOs internally forwarded!")
        --end
    --else
    --    print("Peer is not using compression")
    --end
end

local RPC_CompressHandshake = function(peer, enabled)
    print("Got CompressHandshake")
    --print("enabled type: " .. type(enabled))
    --print("enabled: " .. to_string(enabled))

    --peers[peer] = enabled

    

    if enabled then
        --peers[peer] = peer:GetMethod("ZDOData");
        
        --print("got peer ZDOData method")

        print("Registering CompressedZDOData")

        peers[peer.uuid] = true

        peer:Register("CompressedZDOData", 
            DataType.bytes, RPC_CompressedZDOData
        )

    end

    peer:Invoke("CompressHandshake", enabled);
end

Valhalla.OnEvent("PeerInfo", function(peer)
    print("Registering CompressHandshake")

    --print("peer: " .. type(peer))



    --print("peer: " .. to_string(peer))

    peer:Register("CompressHandshake", 
        DataType.bool, RPC_CompressHandshake
    )
end)

-- remove peer references
Valhalla.OnEvent("PeerQuit", function(peer)
    table.remove(peers, peer.uuid)
end)

-- delegate to replace call
Valhalla.OnEvent("RpcOut", "ZDOData", function(peer, bytes)
    print("Caught ZDOData before send")

    local uuid = peer.uuid
    if peers[uuid] ~= nil then
        print("Compressing...")

        this.event.Cancel()
        local compressed = VUtils.Compress(bytes)
        --if compressed ~= nil then
            peer:Invoke("CompressedZDOData", compressed)
        --end

        print("invoked CompressedZDOData")
    end
end)



Valhalla.OnEvent("Enable", function()
    print("Compress mod Enable() " .. this.mod.version)
end)
