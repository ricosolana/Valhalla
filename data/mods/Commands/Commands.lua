--[[
    Command mod
--]]

local commands = {
    --[[
    reload = {
        function(peer, cmd, args)
            if #args == 0 then
                ModManager:ReloadMod(this)
            else
                local mod = ModManager:GetMod(args[1])
                
                ModManager:ReloadMod(mod)
            end
        end,
        '[mod]',
        'reloads a mod by name'
    },--]]
    worldtime = {
        func = function(peer, cmd, args)
            if #args == 0 then
                peer:ConsoleMessage('World time: ' .. Valhalla.worldTime)
            else
                Valhalla.worldTime = assert(tonumber(args[1]), 'not a number')
                peer:ConsoleMessage('Set world time to ' .. args[1])
            end
        end,
        usage = '[time]',
        desc = 'gets or sets the absolute world time'
    },
    timeofday = {
        func = function(peer, cmd, args)
            if #args == 0 then
                peer:ConsoleMessage('Time of day: ' .. Valhalla.timeOfDay)
            else
                Valhalla.timeOfDay = assert(tonumber(args[1]) or TimeOfDay[args[1]:upper()], 'not a number')
                peer:ConsoleMessage('Set time of day to ' .. args[1] .. '(worldTime: ' .. Valhalla.worldTime .. ')')
            end
        end,
        usage = '[time]',
        desc = 'gets or sets the relative time of day'
    },
    destroy = {
        func = function(peer, cmd, args)
            assert(#args >= 3, 'not enough args')
        
            local mode = assert((args[1] == 'nearby' and args[1]) or (args[1] == 'closest' and args[1]), 'invalid selector mode')
            local prefab = args[2] ~= 'all' and VUtils.String.GetStableHashCode(args[2]) or 0
            local radius = assert(tonumber(args[3]), 'radius expects number')
            --local withFlags = #args >= 4 and tonumber(args[4]) or 0
            --local withoutFlags = #args >= 5 and tonumber(args[5]) or 0
            
            local withFlags = #args >= 4 and assert(Flag[string.upper(args[4])], 'invalid flag') or 0
            local withoutFlags = #args >= 5 and assert(Flag[string.upper(args[5])], 'invalid !flag') or 0
            
            --if radius > 32 and args[#args] ~= '-y' then
            --    return peer:ConsoleMessage('add "-y" to confirm')
            --end
            
            -- ignore SESSIONED flags
            withFlags = withFlags & ~Flag.SESSIONED
            
            -- exclude SESSIONED flags
            withoutFlags = withoutFlags | Flag.SESSIONED
            
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
        usage = '<"nearby"/"closest"> <prefab/"all"> <radius> [flags] [!flags]',
        desc = 'destroy zdos in world according to filters'
    },
    findfeature = {
        func = function(peer, cmd, args)
            local instance = ZoneManager:GetNearestFeature(args[1], peer.zdo.pos)
            if instance then
                peer:ChatMessage(args[1], ChatMsgType.SHOUT, instance.pos, '', '')
                peer:ConsoleMessage('feature located at ' .. tostring(instance.pos))
            else
                peer:ConsoleMessage('feature not found')
            end
        end,
        usage = '<feature>',
        desc = 'find the specified ZoneLocation'
    },
    gendungeon = {
        func = function(peer, cmd, args)
            local dungeon = assert(DungeonManager:GetDungeon(args[1]))
            
            local pos = #args >= 4 and Vector3f.new(tonumber(args[2]), tonumber(args[3]), tonumber(args[4])) or peer.zdo.pos
            
            DungeonManager:Generate(dungeon, pos, Quaternion.IDENTITY)
            
            peer:ConsoleMessage('generated dungeon at ' .. tostring(pos))
        end,
        usage = '<dungeon>',
        desc = 'generates a dungeon by name'
    },
    regenzone = {
        func = function(peer, cmd, args)
            --local radius = assert(tonumber(args[1]), 'radius expects number')
            --            
            --if radius > 2 and args[#args] ~= '-y' then
            --    return peer:ConsoleMessage('add "-y" to confirm')
            --end
            
            local zdos = ZDOManager:GetZDOs(peer.zdo.zone, 0, Flag.NONE, Flag.PLAYER)
            for i=1, #zdos do
                ZDOManager:DestroyZDO(zdos[i])
            end
            
            ZoneManager:PopulateZone(peer.zdo.zone)
            
            --ZoneManager:RegenerateZone(peer.zdo.zone)
            
            peer:ConsoleMessage('regenerated zone at ' .. tostring(peer.zdo.pos))
        end,
        desc = 'completely regenerates a zone in world (all objects in zone will be deleted!)'
    },
    claimwards = {
        func = function(peer, cmd, args)
            local radius = (#args == 1 and tonumber(args[1])) or 32
            
            if radius > 64 and not (#args >= 1 and args[#args] == '-y') then
                return peer:ConsoleMessage('add "-y" to confirm')
            end
            
            local peerZdo = peer.zdo
            
            local zdos = ZDOManager:GetZDOs(peerZdo.pos, radius, 'guard_stone')
            
            local count = 0
            
            for i=1, #zdos do
                local old = zdos[i]
                
                if old:GetString('creatorName') ~= peer.name then
                    -- clone zdo
                    local new = ZDOManager:Instantiate(old)
                    new:Set('creatorName', peer.name)
                    new:Set('creator', peerZdo:GetLong('playerID'))
                    
                    ZDOManager:DestroyZDO(old)
                    
                    count = count + 1
                end
                
                --Views.Ward.new(zdos[i]).creator = peer
            end
            
            peer:ConsoleMessage('claimed ' .. count .. ' wards within ' .. radius .. 'm')
        end,
        usage = '[radius]',
        desc = 'make yourself the owner of all nearby wards'
    },
    op = {
        func = function(peer, cmd, args)
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
        usage = '<peer>',
        desc = 'gives or revokes admin privileges to the specified player'
    },
    tpa = {
        func = function(peer, cmd, args)
            local p1 = NetManager:GetPeer(args[1])
        
            if #args == 1 then
                peer:Teleport(p1.zdo.pos)
            else
                local p2 = NetManager:GetPeer(args[2])
                
                p1:Teleport(p2.zdo.pos)
            end
        end,
        usage = '<peer> [peer]',
        desc = 'teleport yourself to a player, or teleports a player to another player'
    },
    tp = {
        func = function(peer, cmd, args)
            local x = tonumber(args[1])
            local y = tonumber(args[2])
            local z = tonumber(args[3])
            
            peer:Teleport(Vector3f.new(x, y, z))
        end,
        usage = '[x] [y] [z]',
        desc = 'teleport yourself to coordinates in world'
    },
    --[[
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
    },--]]
    abandonz = {
        func = function(peer, cmd, args)
            -- abandon the player-arg
            NetManager:GetPeer(args[1]).zdo:Disown()
        end,
        usage = '<peer>',
        desc = 'abandons a players zdo'
    },
    reclaimz = {
        func = function(peer, cmd, args)
            -- abandon to the player-arg
            local p = NetManager:GetPeer(args[1])
            p.zdo.owner = p.uuid
        end,
        usage = '[peer]',
        desc = 'reclaims a players zdo'
    },
    destroyz = {
        func = function(peer, cmd, args)
            -- abandon to the player-arg
            local p = NetManager:GetPeer(args[1])
            ZDOManager:DestroyZDO(p.zdo)
        end,
        usage = '[peer]',
        desc = 'destroy a players zdo; WARNING: this will brick the players session'
    }
}

Valhalla:Subscribe('Join', function(peer)
    print('Registering command vs')

    peer:Register(
        MethodSig.new('OnCommand', Type.STRING, Type.STRINGS),
        function(peer, label, args)
            if not peer.admin then 
                peer:ConsoleMessage("must be an admin")
            else
                local command = commands[string.lower(label)]
                
                if command then
                    -- the function must be handled because this is an Rpc call, 
                    --  and exceptions occurring during commands should not kick the player
                    
                    local success, err = pcall(command.func, peer, label, args)
                    if not success then
                        for s in err:gmatch('[^\n]+') do
                            peer:ConsoleMessage('<color=#FF5555>' .. s .. '</color>')
                        end
                        
                        if command.usage then
                            peer:ConsoleMessage('<color=#FFAA00>Usage: ' .. label .. ' ' .. (command.usage or '') .. '</color>')
                        end
                    end
                else
                    peer:ConsoleMessage('<color=#FF5555>unknown .vs command</color>')
                    for k,command in pairs(commands) do
                        peer:ConsoleMessage(' <color=#555555>-</color> ' 
                            .. '<color=#AAAAAA>' .. k .. ' ' .. (command.usage or label) .. ' </color><color=#FFAA00>' .. (command.desc or 'a command') .. '</color>')
                    end
                end
            end    
        end
    )

    print('Registered command vs')
end)

