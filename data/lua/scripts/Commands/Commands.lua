--[[
    Command mod
--]]

--require("mobdebug").listen()
require("mobdebug").start()

-- will be essentials minecraft color coded

--local temp = { ['tomlTypeConverters'] = tomlTypeConverters }
local color_codes = {
    -- todo
    --  2 converters
    --  one for peer, another for console
    -- peer:console_message("<color=#FF5555>" .. s .. "</color>")
    --#define COLOR_RESET  "\033[0m"
    --#define COLOR_BLACK  "\033[30m"
    --#define COLOR_RED    "\033[31m"
    --#define COLOR_GREEN  "\033[32m"
    --#define COLOR_GOLD   "\033[33m"
    --#define COLOR_BLUE   "\033[34m"
    --#define COLOR_PURPLE "\033[35m"
    --#define COLOR_CYAN   "\033[36m"
    --#define COLOR_WHITE  "\033[37m"
    --#define COLOR_GRAY   "\033[90m"
    ['&4'] = { hex24 = 'be0000', rtf = { tag = 'color', value = 'be0000' }, ansi16 = { value = '\033[31m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&c'] = { hex24 = 'fe3f3f', rtf = { tag = 'color', value = 'fe3f3f' }, ansi16 = { value = '\033[91m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&6'] = { hex24 = '09a334', rtf = { tag = 'color', value = '09a334' }, ansi16 = { value = '\033[33m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&e'] = { hex24 = 'fefe3f', rtf = { tag = 'color', value = 'fefe3f' }, ansi16 = { value = '\033[93m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&2'] = { hex24 = '00be00', rtf = { tag = 'color', value = '00be00' }, ansi16 = { value = '\033[32m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&a'] = { hex24 = '3ff33f', rtf = { tag = 'color', value = '3ff33f' }, ansi16 = { value = '\033[102m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&b'] = { hex24 = '3ffefe', rtf = { tag = 'color', value = '3ffefe' }, ansi16 = { value = '\033[36m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&1'] = { hex24 = '00bebe', rtf = { tag = 'color', value = '00bebe' }, ansi16 = { value = '\033[34m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&9'] = { hex24 = '3f3ffe', rtf = { tag = 'color', value = '3f3ffe' }, ansi16 = { value = '' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&d'] = { hex24 = 'fe3ffe', rtf = { tag = 'color', value = 'fe3ffe' }, ansi16 = { value = '' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&5'] = { hex24 = 'be00be', rtf = { tag = 'color', value = 'be00be' }, ansi16 = { value = '' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&f'] = { hex24 = 'ffffff', rtf = { tag = 'color', value = 'ffffff' }, ansi16 = { value = '\033[37m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&7'] = { hex24 = 'bebebe', rtf = { tag = 'color', value = 'bebebe' }, ansi16 = { value = '' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&8'] = { hex24 = '3f3f3f', rtf = { tag = 'color', value = '3f3f3f' }, ansi16 = { value = '' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    ['&0'] = { hex24 = '000000', rtf = { tag = 'color', value = '000000' }, ansi16 = { value = '\033[90m' }, ansi256 = { value = '' }, ansi888 = { value = '' } },
    -- format
    ['&l'] = { 'be0000' },
    ['&n'] = { 'be0000' },
    ['&o'] = { 'be0000' },
    ['&k'] = { 'be0000' },
    ['&m'] = { 'be0000' }
    --['&r'] = { 'be0000' },
}

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
                peer:console_message("World time: " .. Avledet.world_time)
            else
                Avledet.world_time = assert(tonumber(args[1]), "not a number")
                peer:console_message("Set world time to " .. args[1])
            end
        end,
        usage = "[time]",
        desc = "gets or sets the absolute world time"
    },
    tod = {
        func = function(peer, cmd, args)
            if #args == 0 then
                peer:console_message("Time of day: " .. Avledet.time_of_day)
            else
                Avledet.time_of_day = assert(tonumber(args[1]) or TimeOfDay[args[1]:upper()], "not a number")
                peer:console_message("Set time of day to " .. args[1] .. "(worldTime: " .. Avledet.world_time .. ")")
            end
        end,
        usage = "[time]",
        desc = "gets or sets the relative time of day"
    },
    destroy = {
        func = function(peer, cmd, args)
            assert(#args >= 3, "not enough args")

            local mode =
                assert((args[1] == "nearby" and args[1]) or (args[1] == "closest" and args[1]), "invalid selector mode")
            local prefab = args[2] ~= "all" and VUtils.String.get_stable_hash(args[2]) or 0
            local radius = assert(tonumber(args[3]), "radius expects number")
            --local withFlags = #args >= 4 and tonumber(args[4]) or 0
            --local withoutFlags = #args >= 5 and tonumber(args[5]) or 0

            local withFlags = #args >= 4 and assert(Flag[string.upper(args[4])], "invalid flag") or 0
            local withoutFlags = #args >= 5 and assert(Flag[string.upper(args[5])], "invalid !flag") or 0

            --if radius > 32 and args[#args] ~= '-y' then
            --    return peer:console_message('add "-y" to confirm')
            --end

            -- ignore SESSIONED flags
            withFlags = withFlags & ~Flag.SESSIONED

            -- exclude SESSIONED flags
            withoutFlags = withoutFlags | Flag.SESSIONED

            -- target zdos in radius zdo
            if mode == "nearby" then
                -- vs destroy nearest all 10
                local zdos = ZDOManager:get_zdos(peer.zdo.pos, radius, prefab, withFlags, withoutFlags)

                for i = 1, #zdos do
                    local zdo = zdos[i]
                    ZDOManager:destroy_zdo(zdo)
                end

                peer:console_message("destroyed " .. #zdos .. " zdos")
            elseif mode == "closest" then
                local zdo = ZDOManager:nearest_zdo(peer.zdo.pos, radius, prefab, withFlags, withoutFlags)

                if zdo then
                    peer:console_message("destroying " .. zdo.prefab.name .. " " .. tostring(zdo.pos))
                    ZDOManager:destroy_zdo(zdo)
                else
                    peer:console_message("no matches found")
                end
            end
        end,
        usage = '<"nearby"/"closest"> <prefab/"all"> <radius> [flags] [!flags]',
        desc = "destroy zdos in world according to filters"
    },
    findfeature = {
        func = function(peer, cmd, args)
            local instance = ZoneManager:find_nearest_feature(args[1], peer.zdo.pos)
            if instance then
                peer:chat_message(args[1], ChatMsgType.SHOUT, instance.pos, "", "")
                peer:console_message("feature located at " .. tostring(instance.pos))
            else
                peer:console_message("feature not found")
            end
        end,
        usage = "<feature>",
        desc = "find the specified ZoneLocation"
    },
    gendungeon = {
        func = function(peer, cmd, args)
            local dungeon
            if #args >= 1 then
                dungeon = DungeonManager:find_dungeon(args[1])
            end

            if not dungeon then
                peer:console_message('valid dungeons:')
                --for _, dg in pairs(DungeonManager.dungeons) do
                local dungeons = DungeonManager.dungeons
                --for i in 1, #dungeons do
                for i = 1, #dungeons do
                    local dungeon = dungeons[i]
                    peer:console_message(' - ' .. dungeon.name)
                end
            else
                local pos =
                    #args >= 4 and Vector3f.new(tonumber(args[2]), tonumber(args[3]), tonumber(args[4])) or peer.zdo.pos

                DungeonManager:generate(dungeon, pos, Quaternion.IDENTITY)

                peer:console_message("generated dungeon at " .. tostring(pos))
            end
        end,
        usage = "<dungeon> [x] [y] [z]",
        desc = "generates a dungeon by name"
    },
    regenzone = {
        func = function(peer, cmd, args)
            --local radius = assert(tonumber(args[1]), 'radius expects number')
            --
            --if radius > 2 and args[#args] ~= '-y' then
            --    return peer:console_message('add "-y" to confirm')
            --end

            local zdos = ZDOManager:get_zdos(peer.zdo.zone, 0, Flag.NONE, Flag.PLAYER)
            for i = 1, #zdos do
                ZDOManager:destroy_zdo(zdos[i])
            end

            ZoneManager:populate_zone(peer.zdo.zone)

            --ZoneManager:RegenerateZone(peer.zdo.zone)

            peer:console_message("regenerated zone at " .. tostring(peer.zdo.pos))
        end,
        desc = "completely regenerates a zone in world (all objects in zone will be deleted!)"
    },
    claimwards = {
        func = function(peer, cmd, args)
            local radius = (#args == 1 and tonumber(args[1])) or 32

            if radius > 64 and not (#args >= 1 and args[#args] == "-y") then
                return peer:console_message('add "-y" to confirm')
            end

            local peerZdo = peer.zdo

            local zdos = ZDOManager:get_zdos(peerZdo.pos, radius, "guard_stone")

            local count = 0

            for i = 1, #zdos do
                local old = zdos[i]

                if old:get_string("creatorName") ~= peer.name then
                    -- clone zdo
                    local new = ZDOManager:instantiate(old)
                    new:set("creatorName", peer.name)
                    new:set("creator", peerZdo:get_long("playerID"))

                    ZDOManager:destroy_zdo(old)

                    count = count + 1
                end

                --Views.Ward.new(zdos[i]).creator = peer
            end

            peer:console_message("claimed " .. count .. " wards within " .. radius .. "m")
        end,
        usage = "[radius]",
        desc = "make yourself the owner of all nearby wards"
    },
    op = {
        func = function(peer, cmd, args)
            local p = NetManager:find_peer(args[1])
            if p then
                p.admin = not p.admin

                if p.admin then
                    peer:console_message("opped " .. p.name)
                else
                    peer:console_message("deopped " .. p.name)
                end
            else
                peer:console_message("peer not found")
            end
        end,
        usage = "<peer>",
        desc = "gives or revokes admin privileges to the specified player"
    },
    tpa = {
        func = function(peer, cmd, args)
            local p1 = assert(NetManager:find_peer(args[1]), 'peer not found')

            if #args == 1 then
                peer:teleport(p1.zdo.pos)
            else
                local p2 = NetManager:find_peer(args[2])

                p1:teleport(p2.zdo.pos)
            end
        end,
        usage = "<peer> [peer]",
        desc = "teleport yourself to a player, or teleports a player to another player"
    },
    tp = {
        func = function(peer, cmd, args)
            local x = tonumber(args[1])
            local y = tonumber(args[2])
            local z = tonumber(args[3])

            peer:teleport(Vector3f.new(x, y, z))
        end,
        usage = "[x] [y] [z]",
        desc = "teleport yourself to coordinates in world"
    },
    --[[
    moveto = {
        function(peer, cmd, args)
            local p1 = NetManager:find_peer(args[1])
        
            if #args == 1 then
                peer:MoveTo(p1.zdo.pos)
            else
                local p2 = NetManager:find_peer(args[2])
                
                p1:MoveTo(p2.zdo.pos)
            end
        end,
        '<peer> [peer]'
    },--]]
    abandonz = {
        func = function(peer, cmd, args)
            -- abandon the player-arg
            local p = assert(NetManager:find_peer(args[1]), 'peer not found')
            p.zdo:disown()
        end,
        usage = "<peer>",
        desc = "abandons a players zdo"
    },
    reclaimz = {
        func = function(peer, cmd, args)
            -- abandon to the player-arg
            local p = assert(NetManager:find_peer(args[1]), 'peer not found')
            p.zdo.owner = p.uuid
        end,
        usage = "[peer]",
        desc = "reclaims a players zdo"
    },
    destroyz = {
        func = function(peer, cmd, args)
            -- abandon to the player-arg
            local p = assert(NetManager:find_peer(args[1]), 'peer not found')
            ZDOManager:destroy_zdo(p.zdo)
        end,
        usage = "[peer]",
        desc = "destroy a players zdo; WARNING: this will brick the players session"
    },
    gc = {
        func = function(peer, cmd, args)
            collectgarbage("collect")
        end,
        desc = "runs lua garbage collector"
    }
}

local invoke = function(peer, rargs)
    local label = rargs[2]
    if not label then 
        peer:console_message("Avledet " .. Avledet.version .. ", time: " .. tostring(Avledet.world_time))
        return
    end
    
    local skip_first = function(v, x)
        local result = {}
        for i = x + 1, #v do
            result[#result + 1] = v[i]
        end
        return result
    end

    --local args = table.remove(rargs, 1)
    local args = skip_first(rargs, 2)

    local command = commands[string.lower(label)]

    if command then
        -- the function must be handled because this is an Rpc call,
        --  and exceptions occurring during commands should not kick the player

        local success, err = pcall(command.func, peer, label, args)
        if not success then
            for s in err:gmatch("[^\n]+") do
                peer:console_message("<color=#FF5555>" .. s .. "</color>")
            end

            if command.usage then
                peer:console_message(
                    "<color=#FFAA00>Usage: " .. label .. " " .. (command.usage or "") .. "</color>"
                )
            end
        end
    else
        peer:console_message("<color=#FF5555>unknown .vs command</color>")
        for k, command in pairs(commands) do
            peer:console_message(
                " <color=#555555>-</color> " ..
                    "<color=#AAAAAA>" ..
                        k ..
                            " " ..
                                (command.usage or label) ..
                                    " </color><color=#FFAA00>" ..
                                        (command.desc or "a command") .. "</color>"
            )
        end
    end
end

Avledet:subscribe(
    "Join",
    function(peer)
        print("Registering command avl")

        peer:register(
            MethodSig.new("_AvlCommand", Type.INT, Type.STRINGS),
            function(peer, _, rargs)
                -- rargs is a userdata container
                -- "sol.std::vector<std::__cxx11::basic_string<char> >: 0x555557429b78"

                if not peer.admin then
                    peer:console_message("must be an admin")
                else
                    invoke(peer, rargs)
                end
            end
        )

        print("Registered command avl")
    end
)

local do_test = function()
    local dummy_proxy_peer = {
        console_message = function(self, msg)
            -- TODO
            print(msg)
        end,
        chat_message = function(self)
            --peer:chat_message(args[1], ChatMsgType.SHOUT, instance.pos, "", "")
        end,
        zdo = {
            zone = Vec2s.new(0, 0),
            pos = Vec3f.new(0, 0, 0)
        }
    }

    local test_suite = {
        --{ 'avl', 'worldtime' },
        --{ 'avl', 'worldtime', '10000' },
        --{ 'avl', 'tod' },
        --{ 'avl', 'tod', 'night' },
        --{ 'avl', 'gendungeon' },
        --{ 'avl', 'gendungeon', 'a' },
        { 'avl', 'gendungeon', 'DG_ForestCrypt'}
    }

    for i = 1, #test_suite do
        local rargs = test_suite[i]
        invoke(dummy_proxy_peer, rargs)
    end
end

-- we perform test after all managers are loaded
Avledet:subscribe("Enable", do_test)