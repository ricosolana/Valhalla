--[[
    Command mod to illustrate RPC handler
--]]

--local WARD_PREFAB = VUtils.String.GetStableHashCode('guard_stone')

local WARD_PREFAB = PrefabManager.GetPrefab('guard_stone')

local RPC_vs = function(peer, cmd, args)

    if not peer.admin then 
        peer:ConsoleMessage("must be an admin")
        return 
    end
    
    print('Got command ' .. cmd)
    
    print('args: ' .. #args)
    for i=1, #args do
        print(' - ' .. args[i])
    end
    
    if cmd == 'feature' then
        if #args == 1 then
            local instance = ZoneManager.GetNearestFeature(args[1], peer.pos)
            if instance then
                peer:ChatMessage(args[1], ChatMsgType.shout, instance.pos, '', '')
                peer:ConsoleMessage('feature located at ' .. tostring(instance.pos))
            else
                peer:ConsoleMessage('feature not found')
            end
        else
            peer:ConsoleMessage("feature arg missing")
        end
    elseif cmd == 'dungeon' then
        if #args == 1 then
            local dungeon = DungeonManager.GetDungeon(args[1])
            if dungeon then
                local pos = peer.pos
                local rot = Quaternion.identity
                
                dungeon:Generate(pos, rot)
                
                peer:ConsoleMessage('generated dungeon at ' .. tostring(pos))
            else
                peer:ConsoleMessage('dungeon not found')
            end
        else
            peer:ConsoleMessage("dungeon arg missing")
        end
    elseif cmd == 'claim' then
        local radius = (#args == 1 and tonumber(args[1])) or 32
        --local radius = #args ~= 1 and 32 or (true and tonumber(args[1]))
        
        local zdos = ZDOManager.GetZDOs(peer.pos, radius, WARD_PREFAB)
        
        for i=1, #zdos do
            Views.Ward.new(zdos[i]).creator = peer
        end
        
        peer:ConsoleMessage('claimed ' .. #zdos .. ' wards within radius ' .. radius)
    elseif cmd == 'op' then
        if #args == 1 then
            local p = NetManager.GetPeer(args[1])
            if p then 
                p.admin = not p.admin 
                
                if p.admin then
                    peer:ConsoleMessage('opped ' .. p.name)
                else
                    peer:ConsoleMessage('deopped ' .. p.name)
                end                
            else
                peer:ConsoleMessage('player not found')
            end
        else
            peer:ConsoleMessage('player arg missing')
        end
    elseif cmd == 'tp' then
        if #args == 1 then
            -- tp to the player-arg
            local p = NetManager.GetPeer(args[1])
            if p then 
                peer:Teleport(p.pos)
            else
                peer:ConsoleMessage('player not found ')
            end
        elseif #args == 2 then
            local p1 = NetManager.GetPeer(args[1])
            local p2 = NetManager.GetPeer(args[2])
            if p1 and p2 then 
                p1:Teleport(p2.pos)
            else
                peer:ConsoleMessage('players not found ')
            end
        else
            peer:ConsoleMessage('expected 1 or 2 args')
        end
    elseif cmd == 'moveto' then
        if #args == 1 then
            -- tp to the player-arg
            local p = NetManager.GetPeer(args[1])
            if p then 
                peer:MoveTo(p.pos)
            else
                peer:ConsoleMessage('player not found ')
            end
        elseif #args == 2 then
            local p1 = NetManager.GetPeer(args[1])
            local p2 = NetManager.GetPeer(args[2])
            if p1 and p2 then 
                p1:MoveTo(p2.pos)
            else
                peer:ConsoleMessage('players not found ')
            end
        else
            peer:ConsoleMessage('expected 1 or 2 args')
        end
    elseif cmd == 'abandon' then
        if #args == 1 then
            -- abandon to the player-arg
            local p = NetManager.GetPeer(args[1])
            if p then 
                local zdo = peer.zdo
                if zdo then
                    zdo:Abandon()
                end
            else
                peer:ConsoleMessage('player not found ')
            end
        else
            peer:ConsoleMessage('expected 1 arg')
        end
    elseif cmd == 'reclaim' then
        if #args == 1 then
            -- abandon to the player-arg
            local p = NetManager.GetPeer(args[1])
            if p then 
                local zdo = peer.zdo
                if zdo then
                    zdo.owner = p.uuid
                end
            else
                peer:ConsoleMessage('player not found ')
            end
        else
            peer:ConsoleMessage('expected 1 arg')
        end        
    end
end

Valhalla.OnEvent('Join', function(peer)
    print('Registering command vs')

    peer.value:Register(
        MethodSig.new('vs', DataType.string, DataType.strings),
        RPC_vs
    )

    print('Registered command vs')
end)

Valhalla.OnEvent('Enable', function()
    print(this.name .. ' enabled')
end)
