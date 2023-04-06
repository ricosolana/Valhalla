Config = require 'config'
TOML = require 'toml'

local peers = {}

local mod = ModManager:GetMod('PotionsPlus')

local toggleDataConverter = {
  qualifier = 'PotionsPlus+Toggle',
  serialize = function(writer, value) 
    if value == 'off' then
      writer:WriteInt32(0)
    elseif value == 'on' then
      writer:WriteInt32(1)
    else
      error('bad Toggle ' .. value)
    end
  end,
  deserialize = function(reader) 
    local value = reader:ReadInt32()
    
    if value == 0 then
      return 'off'
    elseif value == 1 then
      return 'on'
    else
      error('bad toggle ' .. value)
    end
  end
}

-- Adds global toml converters
--  Not recommended because some mod types might collide if they have the same name
--TOML.CONVERTERS['Toggle'] = { from = FROM_TOML_DEFAULT }

local typeConverters = {
  ['Toggle'] = { from = TOML.FROM_DEFAULT }
}

local dataConverters = { 
  ['Toggle'] = toggleDataConverter
}

local info = {
  name = mod.name,
  version = mod.version,
  minVersion = mod.version,
  modRequired = false,
  locked = true,
  dataConverters = dataConverters,
}

local config = Config.new(info)
config:loadFile('./mods/PotionsPlus/com.odinplus.potionsplus.cfg', typeConverters)
