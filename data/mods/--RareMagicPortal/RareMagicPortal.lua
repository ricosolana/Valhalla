--[[
    Implementation of the BetterNetworking mod for Valheim (3/23/2023)
    
    https://github.com/CW-Jesse/valheim-betternetworking

    Ported by crzi for use on the C++ Valhalla server
--]]

--TOML = require 'toml'

ConfigSync = require 'config'

local peers = {}

local mod = ModManager:GetMod('RareMagicPortal')

local config = ConfigSync.new(mod.name, 'RareMagicPortal/WackyMole.RareMagicPortal.cfg', mod.version, mod.version, false)

--[[
local open = io.open

local function read_file(path)
    local file = open(path, "rb") -- r read mode and b binary mode
    if not file then return nil end
    local content = file:read "*a" -- *a or *all reads the whole file
    file:close()
    return content
end--]]

-- Unsurprisingly, the BepinEx toml file parser and subsequent RareMagicPortal config file 
--  use non-standard toml syntax (keys contain spaces which breaks parsing)
-- The parser and reader within BepinEx appears to be simple enough to implement,
--  so I will go ahead and do that eventually...
--local config = TOML.parse(read_file('C:/Users/Rico/AppData/Roaming/r2modmanPlus-local/Valheim/profiles/dev/BepInEx/config/WackyMole.RareMagicPortal.cfg'))

--local config = TOML.parse(VUtils.Resource.ReadFileString('mods/RareMagicPortal/WackyMole.RareMagicPortal.cfg'))



--[[
local parseConfig = function()
    local readConfig = TOML.parse(VUtils.Resource.ReadFileString('mods/RareMagicPortal/WackyMole.RareMagicPortal.cfg'))
    
    config._serverConfigLocked = readConfig['1.General']['Force Server Config']
end
--]]


