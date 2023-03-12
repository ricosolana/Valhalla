--[[
    Command mod
--]]

local commands = {
    worldtime = {
        function(peer, cmd, args)
            if #args == 0 then
                peer:ConsoleMessage('World time: ' .. Valhalla.worldTime)
            else
                Valhalla.worldTime = assert(tonumber(args[1]), 'not a number')
                peer:ConsoleMessage('Set world time to ' .. args[1])
            end
        end,
        '[time]'
    },
    timeofday = {
        function(peer, cmd, args)
            if #args == 0 then
                peer:ConsoleMessage('Time of day: ' .. Valhalla.timeOfDay)
            else
                Valhalla.timeOfDay = assert(tonumber(args[1]), 'not a number')
                peer:ConsoleMessage('Set time of day to ' .. args[1] .. '(worldTime: ' .. Valhalla.worldTime .. ')')
            end
        end,
        '[time]'
    },
    destroy = {
        function(peer, cmd, args)
            assert(#args >= 3, 'not enough args')
        
            local mode = assert((args[1] == 'nearby' and args[1]) or (args[1] == 'closest' and args[1]), 'invalid selector mode')
            local prefab = args[2] ~= 'all' and VUtils.String.GetStableHashCode(args[2]) or 0
            local radius = assert(tonumber(args[3]), 'radius expects number')
            --local withFlags = #args >= 4 and tonumber(args[4]) or 0
            --local withoutFlags = #args >= 5 and tonumber(args[5]) or 0
            
            local withFlags = #args >= 4 and assert(PrefabFlag[string.upper(args[4])], 'invalid flag') or 0
            local withoutFlags = #args >= 5 and assert(PrefabFlag[string.upper(args[5])], 'invalid !flag') or 0
            
            --if radius > 32 and args[#args] ~= '-y' then
            --    return peer:ConsoleMessage('add "-y" to confirm')
            --end
            
            -- ignore SESSIONED flags
            withFlags = withFlags & ~PrefabFlag.SESSIONED
            
            -- exclude SESSIONED flags
            withoutFlags = withoutFlags | PrefabFlag.SESSIONED
            
            -- target zdos in radius zdo
            if mode == 'nearby' then
                -- vs destroy nearest all 10
                local zdos = ZDOManager:GetZDOs(peer.zdo.pos, radius, prefab, withFlags, withoutFlags)
                
                for i=1, #zdos do
                    local zdo = zdos[i]
                    ZDOManager:DestroyZDO(zdo)
                end
                
                peer:ConsoleMessage('destroyed ' .. #zdos .. ' zdos')
            elseif mode == 'closest' then
                local zdo = ZDOManager:NearestZDO(peer.zdo.pos, radius, prefab, withFlags, withoutFlags)
                
                if zdo then
                    peer:ConsoleMessage('destroying ' .. zdo.prefab.name .. ' ' .. tostring(zdo.pos))
                    ZDOManager:DestroyZDO(zdo)
                else
                    peer:ConsoleMessage('no matches found')
                end
            end
        end,
        '<"nearby"/"closest"> <prefab/"all"> <radius> [flags] [!flags]'
    },
    findfeature = {
        function(peer, cmd, args)
            local instance = ZoneManager:GetNearestFeature(args[1], peer.zdo.pos)
            if instance then
                peer:ChatMessage(args[1], ChatMsgType.SHOUT, instance.pos, '', '')
                peer:ConsoleMessage('feature located at ' .. tostring(instance.pos))
            else
                peer:ConsoleMessage('feature not found')
            end
        end,
        '<feature>'
    },
    gendungeon = {
        function(peer, cmd, args)
            local dungeon = assert(DungeonManager:GetDungeon(args[1]))
            
            local pos = #args >= 4 and Vector3.new(tonumber(args[2]), tonumber(args[3]), tonumber(args[4])) or peer.zdo.pos
            
            DungeonManager.Generate(dungeon, pos, Quaternion.IDENTITY)
            
            peer:ConsoleMessage('generated dungeon at ' .. tostring(pos))
        end,
        '<dungeon>'
    },
    regenzone = {
        function(peer, cmd, args)
            --local radius = assert(tonumber(args[1]), 'radius expects number')
            --            
            --if radius > 2 and args[#args] ~= '-y' then
            --    return peer:ConsoleMessage('add "-y" to confirm')
            --end
            
            ZoneManager:RegenerateZone(peer.zdo.zone)
            
            peer:ConsoleMessage('regenerated zone at ' .. tostring(peer.zdo.pos))
        end,
        '<z-radius>'
    },
    claimwards = {
        function(peer, cmd, args)
            local radius = (#args == 1 and tonumber(args[1])) or 32
            
            if radius > 64 and not (#args >= 1 and args[#args] == '-y') then
                return peer:ConsoleMessage('add "-y" to confirm')
            end
            
            local zdos = ZDOManager:GetZDOs(peer.zdo.pos, radius, 'guard_stone')
            
            for i=1, #zdos do
                Views.Ward.new(zdos[i]).creator = peer
            end
            
            peer:ConsoleMessage('claimed ' .. #zdos .. ' wards within ' .. radius .. 'm')
        end,
        '[radius]'
    },
    op = {
        function(peer, cmd, args)
            local p = NetManager:GetPeer(args[1])
            if p then 
                p.admin = not p.admin 
                
                if p.admin then
                    peer:ConsoleMessage('opped ' .. p.name)
                else
                    peer:ConsoleMessage('deopped ' .. p.name)
                end                
            else
                peer:ConsoleMessage('peer not found')
            end
        end,
        '<peer>'
    },
    tpa = {
        function(peer, cmd, args)
            local p1 = NetManager:GetPeer(args[1])
        
            if #args == 1 then
                peer:Teleport(p1.zdo.pos)
            else
                local p2 = NetManager:GetPeer(args[2])
                
                p1:Teleport(p2.zdo.pos)
            end
        end,
        '<peer> [peer]'
    },
    tp = {
        function(peer, cmd, args)
            local x = tonumber(args[1])
            local y = tonumber(args[2])
            local z = tonumber(args[3])
            
            peer:Teleport(Vector3.new(x, y, z))
        end,
        '[x] [y] [z]'
    },    
    moveto = {
        function(peer, cmd, args)
            local p1 = NetManager:GetPeer(args[1])
        
            if #args == 1 then
                peer:MoveTo(p1.zdo.pos)
            else
                local p2 = NetManager:GetPeer(args[2])
                
                p1:MoveTo(p2.zdo.pos)
            end
        end,
        '<peer> [peer]'
    },
    abandon = {
        function(peer, cmd, args)
            -- abandon the player-arg
            NetManager.GetPeer(args[1]).zdo:Abandon()
        end,
        '<peer>'
    },
    reclaim = {
        function(peer, cmd, args)
            -- abandon to the player-arg
            local p = NetManager.GetPeer(args[1])
            p.zdo.owner = p.uuid
        end,
        '[peer]'
    }
}

Valhalla:Subscribe('Join', function(peer)
    print('Registering command vs')

    peer:Register(
        MethodSig.new('OnCommand', DataType.STRING, DataType.STRINGS),
        function(peer, label, args)
            if not peer.admin then 
                peer:ConsoleMessage("must be an admin")
            else
                local command = commands[string.lower(label)]
                
                if command then
                    -- the function must be handled because this is an Rpc call, 
                    --  and exceptions occurring during commands should not kick the player
                    
                    local success, err = pcall(command[1], peer, label, args)
                    if not success then
                        peer:ConsoleMessage('Error: ' .. err)
                        peer:ConsoleMessage('Usage: ' .. label .. ' ' .. command[2])
                    end
                else
                    peer:ConsoleMessage('unknown .vs command')
                    for k,v in pairs(commands) do
                        if #v == 1 then
                            peer:ConsoleMessage(' - ' .. k)
                        else
                            peer:ConsoleMessage(' - ' .. k .. ' ' .. v[2])
                        end
                    end
                end
            end    
        end
    )

    print('Registered command vs')
end)

Valhalla:Subscribe('Enable', function()
    print(this.name .. ' enabled')
end)
