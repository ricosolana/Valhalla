onEnable = function()
    print("My join mod onEnable!", "Multiple paramaters!", "Really cool!")

    --Valhalla.TestLambda(0, nil)
    --Valhalla.RpcCallback("ServerHandshake", function(rpc)
    --    print("Serverhandshake called!!!")
    --end)

    Valhalla.RpcCallback("PeerInfo", function(rpc, pkg)
        --print(type(pkg))
        --print(pkg.type())
        rpc.socket:GetHostName
        print("PeerInfo called!!!")
    end)
end

-- example rpc callback registration
--Valhalla.RpcCallback()