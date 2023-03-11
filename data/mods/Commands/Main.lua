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
                peer:ConsoleMessage('player not found')
            end
        end,
        '<player>'
    },
    tpa = {
        function(peer, cmd, args)
            if #args == 1 then
                -- tp to the player-arg
                local p = NetManager:GetPeer(args[1])
                if p then 
                    peer:Teleport(p.zdo.pos)
                else
                    peer:ConsoleMessage('player not found ')
                end
            elseif #args == 2 then
                local p1 = NetManager:GetPeer(args[1])
                local p2 = NetManager:GetPeer(args[2])
                if p1 and p2 then 
                    p1:Teleport(p2.zdo.pos)
                else
                    peer:ConsoleMessage('players not found ')
                end
            else
                peer:ConsoleMessage('expected 1 or 2 args')
            end
        end,
        '[player] [player]'
    },
    --tp = {
    --    function(peer, cmd, args)
    --        if #args == 1 then
    --            -- tp to the player-arg
    --            local p = NetManager:GetPeer(args[1])
    --            if p then 
    --                peer:Teleport(p.zdo.pos)
    --            else
    --                peer:ConsoleMessage('player not found ')
    --            end
    --        elseif #args == 2 then
    --            local p1 = NetManager:GetPeer(args[1])
    --            local p2 = NetManager:GetPeer(args[2])
    --            if p1 and p2 then 
    --                p1:Teleport(p2.zdo.pos)
    --            else
    --                peer:ConsoleMessage('players not found ')
    --            end
    --        else
    --            peer:ConsoleMessage('expected 1 or 2 args')
    --        end
    --    end,
    --    '[x] [y] [z]'
    --}
    
    --moveto = {
    --    function(peer, cmd, args)
    --    
    --    end
    --},
    --abandon = {
    --    function(peer, cmd, args)
    --    
    --    end
    --},
    --reclaim = {
    --    function(peer, cmd, args)
    --    
    --    end
    --}
}

local handleVsCommand = function(peer, cmd, args)
    
    local command = commands[cmd]
    
    if command then
        return command[1](peer, cmd, args)
    else
        peer:ConsoleMessage('unknown .vs command')
        for k,v in pairs(commands) do
            peer:ConsoleMessage(' - ' .. k .. ' ' .. v[2])
        end
        return
    end
    
    
    
    if cmd == 'time' then
        if #args == 0 then
            peer:ConsoleMessage('World time: ' .. tostring(Valhalla.worldTime))
            --peer:ConsoleMessage('World time: ' .. Valhalla)
            --peer:ConsoleMessage('This better run')
        else
            local t = tonumber(args[1])
            if t then
                Valhalla.worldTime = t
                peer:ConsoleMessage('Set world time to ' .. args[1])
            else
                peer:ConsoleMessage('Invalid time')
            end
        end
    elseif cmd == 'destroy' then
        -- destroy DG_Cave 32
        -- destroy DG_Cave <32>
        -- destroy -all <32>
        --local radius = tonumber(args[2]) or 32
        
        local radius = nil
        local proximal = nil
        local prefab = nil
        
        if #args == 0 or #args % 2 ~= 0 then
            return peer:ConsoleMessage('usage: destroy --target <prox/rad> --prefab <name/all> --radius <radius>')
        end
        
        for i=1, #args do
            if args[i] == '--target' then
                if args[i + 1] == 'prox' then
                    proximal = true
                elseif args[i + 1] == 'rad' then
                    proximal = false
                end
            elseif args[i] == '--prefab' then
                prefab = args[i + 1]
            elseif args[i] == '--radius' then
                radius = tonumber(args[i + 1]) 
                if not radius or radius > 32 then 
                    radius = nil
                    break
                end
            end
        end

        if not prefab then
            return peer:ConsoleMessage('prefab unspecified')
        elseif prefab == 'all' then
            prefab = nil
        end

        if not radius then 
            return peer:ConsoleMessage('radius invalid or too large')
        end
        
        if proximal == nil then
            return peer:ConsoleMessage('target invalid or unspecified')
        end

        -- target only nearest zdo
        if proximal then
            local zdo = ZDOManager.NearestZDO(peer.pos, radius, prefab or 0, PrefabFlag.NONE, PrefabFlag.SESSIONED)
            
            if zdo then
                peer:ConsoleMessage('destroying ' .. zdo.prefab.name)
                ZDOManager.DestroyZDO(zdo)
            else
                peer:ConsoleMessage('no persistent matches found')
            end
        else
            -- vs destroy --target prox --prefab all --radius 10
            local zdos = nil
            if prefab then
                zdos = ZDOManager.GetZDOs(peer.pos, radius, prefab)
            else
                zdos = ZDOManager.GetZDOs(peer.pos, radius, 0, PrefabFlag.NONE, PrefabFlag.SESSIONED)
            end
            
            for i=1, #zdos do
                local zdo = zdos[i]
                ZDOManager.DestroyZDO(zdo)
            end
            
            peer:ConsoleMessage('destroyed ' .. tostring(#zdos) .. ' persistent zdos')
        end
    elseif cmd == 'feature' then
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
        if #args == 0 then
            peer:ConsoleMessage("usage: vs dungeon [dungeon]")
            return
        end
    
        local dungeon = DungeonManager.GetDungeon(args[1])
        
        if not dungeon then
            peer:ConsoleMessage('dungeon not found')
            return
        end
        
        local pos = peer.pos
        if #args == 4 then
            local x = tonumber(args[2])
            local y = tonumber(args[3])
            local z = tonumber(args[4])
            
            if not x or not y or not z then
                peer:ConsoleMessage('invalid position')
                return
            end
            
            pos = Vector3.new(x, y, z)
        end
        
        DungeonManager.Generate(dungeon, pos, Quaternion.identity)
        
        peer:ConsoleMessage('generated dungeon at ' .. tostring(pos))
    elseif cmd == 'claim' then
        local radius = (#args == 1 and tonumber(args[1])) or 32
        
        local zdos = ZDOManager.GetZDOs(peer.pos, radius, 'guard_stone')
        
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

local RPC_vs = function(peer, label, args)
    if not peer.admin then 
        peer:ConsoleMessage("must be an admin")
    else         
        --print('Got command ' .. cmd)
        --
        --print('args: ' .. #args)
        --for i=1, #args do
        --    print(' - ' .. args[i])
        --end
        
        local command = commands[string.lower(label)]
        
        if command then
            -- the function must be handled because this is an Rpc call, 
            --  and exceptions occurring during commands should not kick the player
            local success, errMsg = pcall(command[1], peer, label, args)
            if not success then
                peer:ConsoleMessage('Error: ' .. errMsg)
                peer:ConsoleMessage('Usage: ' .. label .. ' ' .. command[2])
            end
        else
            peer:ConsoleMessage('unknown .vs command')
            for k,v in pairs(commands) do
                peer:ConsoleMessage(' - ' .. k .. ' ' .. v[2])
            end
        end
    end    
end

Valhalla:OnEvent('Join', function(peer)
    print('Registering command vs')

    peer.value:Register(
        MethodSig.new('vs', DataType.STRING, DataType.STRINGS),
        RPC_vs
    )

    print('Registered command vs')
end)

Valhalla:OnEvent('Enable', function()
    print(this.name .. ' enabled')
end)
