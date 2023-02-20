--[[
    Vanilla portal mod to connect portals together throughout gameplay

    Created by crzi for use on the C++ Valhalla server

    I never knew this, but tags are not fully unique
        Once a portal is paired with another with a certain tag,
        another pair of portals can be paired with the same tag as the first pair

        my code below also exhibits this, (un)fortunately? so.
--]]

local WARD_PREFAB = VUtils.String.GetStableHashCode("guard_stone")

local RPC_vs = function(peer, cmd, args)

    if not peer.admin then return end
    
    print("Got command " .. cmd)
    
    print("args: " .. #args)
    for i=1, #args do
        print(" - " .. args[i])
    end
    
    if cmd == "claim" then
        if #args == 1 then
            local radius = tonumber(args[1]) or 32
            
            local zdos = ZDOManager.GetZDOs(peer.pos, radius, WARD_PREFAB)
            
            for i=1, #zdos do
                Views.Ward.new(zdos[i]).creator = peer
            end
            
            peer:Message("claimed " .. #zdos .. " wards", MsgType.console)
            
        else
            peer:Message("radius arg missing", MsgType.console)
        end
    elseif cmd == "op" then
        if #args == 1 then
            local p = NetManager.GetPeer(args[1])
            if p then 
                p.admin = not p.admin 
                
                if p.admin then
                    peer:Message("opped " .. p.name)
                else
                    peer:Message("deopped " .. p.name)
                end                
            else
                peer:Message("player not found", MsgType.console)
            end
        else
            peer:Message("player arg missing", MsgType.console)
        end
    elseif cmd == "tp" then
        if #args == 1 then
            -- tp to the player-arg
            local p = NetManager.GetPeer(args[1])
            if p then 
                peer:Teleport(p.pos)
            else
                peer:Message("player not found ", MsgType.console)
            end
        elseif #args == 2 then
            local p1 = NetManager.GetPeer(args[1])
            local p2 = NetManager.GetPeer(args[2])
            if p1 and p2 then 
                p1:Teleport(p2.pos)
            else
                peer:Message("players not found ", MsgType.console)
            end
        else
            peer:Message("expected 1 or 2 args", MsgType.console)
        end
    end
end

Valhalla.OnEvent("Join", function(peer)
    print("Registering command vs")

    peer.value:Register(
        MethodSig.new("vs", DataType.string, DataType.strings),
        RPC_vs
    )

    print("Registered command vs")
end)

Valhalla.OnEvent("Enable", function()
    print(this.name .. " enabled")
end)
