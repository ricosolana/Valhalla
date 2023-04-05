ConfigSync = require 'config'

local peers = {}

local mod = ModManager:GetMod('PotionsPlus')

local toggleType = { 
  serialize = function(writer, value) 
    if value == 'off' then
      writer:WriteInt(0)
    elseif value == 'on' then
      writer:WriteInt(1)
    else
      error('bad Toggle')
    end
  end,
  deserialize = function(reader) 
    local value = reader:ReadInt()
    
    if value == 0 then
      return 'off'
    elseif value == 1 then
      return 'on'
    else
      error('bad toggle')
    end
  end
}

local customDataConverters = { Toggle = toggleType }

local config = ConfigSync.new(mod.name, mod.version, mod.version, false)
config:loadFile('PotionsPlus/com.odinplus.potionsplus.cfg', nil, customDataConverters)



