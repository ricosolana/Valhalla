-- ConfigSync

local TOML = require 'toml'

--[[
  Constants
--]]
local SIG_VersionCheck = MethodSig.new('ServerSync VersionCheck', Type.BYTES)

local CONFIGS = {}

local Config = {}

Config.new = function(name, version, minVersion, modRequired, locked)
  local self = {}
  
  print('registering sync config for ' .. name)
  
  self.version = version
  self.minVersion = minVersion
  self.modRequired = modRequired
  self.locked = locked
  
  CONFIGS[name] = self
  
  RouteManager:Register(MethodSig.new(name .. ' ConfigSync', Type.BYTES), function(peer, bytes)
    if not self.locked or peer.admin then
      --then update config values sent by peer
      
    end
  end)
  
  self.loadFile = function(self, configPath, customTomlConverters, customDataConverters)
    self.cfg = TOML.read('./mods/' .. configPath, customTomlConverters, customDataConverters)
    
    --self.cfg['Internal']['serverversion'] = { tomlTypeName = 'String', value = version }
    --self.cfg['Internal']['lockexempt'] =    { tomlTypeName = 'Boolean', value = version }
  end
  
  return self
end

local TOML_TO_DATA_NAMES = {
  String = 'System.String',
  Boolean = 'System.Boolean',
  SByte = 'System.SByte',
  Byte = 'System.Byte',
  Int16 = 'System.Int16',
  UInt16 = 'System.UInt16',
  Int32 = 'System.Int32',
  UInt32 = 'System.UInt32',
  Int64 = 'System.Int64',
  UInt64 ='System.UInt64',
  Single = 'System.Single',
  Double = 'System.Double',
  CraftingTable = 'ItemManager.CraftingTable'
}

-- no point in using this because c# has the right to toss absurdely
--  large 'assembly qualified type names' with extra garbage
--   data I do not care about
--[[
local DATA_TO_TOML_NAMES = {
  ['System.String'] = 'String',
  ['System.Boolean'] = 'Boolean',
  ['System.SByte'] = 'SByte',
  ['System.Byte'] = 'Byte',
  ['System.Int16'] = 'Int16',
  ['System.UInt16'] = 'UInt16',
  ['System.Int32'] = 'Int32',
  ['System.UInt32'] = 'UInt32',
  ['System.Int64'] = 'Int64',
  ['System.UInt64'] = 'UInt64',
  ['System.Single'] = 'Single',
  ['System.Double'] = 'Double',
  ['ItemManager.CraftingTable'] = 'CraftingTable',
}--]]

local DATA_CONVERTERS = {
  ['System.String'] =   { serialize = 'Write',        deserialize = 'ReadString' },
  ['System.Boolean'] =  { serialize = 'Write',        deserialize = 'ReadBool' },
  ['System.SByte'] =    { serialize = 'WriteInt8',    deserialize = 'ReadInt8' },
  ['System.Byte'] =     { serialize = 'WriteUInt8',   deserialize = 'ReadUInt8' }, 
  ['System.Int16'] =    { serialize = 'WriteInt16',   deserialize = 'ReadInt16' },
  ['System.UInt16'] =   { serialize = 'WriteUInt16',  deserialize = 'ReadUInt16' },
  ['System.Int32'] =    { serialize = 'WriteInt32',   deserialize = 'ReadInt32' },
  ['System.UInt32'] =   { serialize = 'WriteUInt32',  deserialize = 'ReadUInt32' },
  ['System.Int64'] =    { serialize = 'Write',        deserialize = 'ReadInt64' },
  ['System.UInt64'] =   { serialize = 'Write',        deserialize = 'ReadUInt64' },
  ['System.Single'] =   { serialize = 'WriteFloat',   deserialize = 'ReadFloat' },
  ['System.Double'] =   { serialize = 'WriteDouble',  deserialize = 'ReadDouble' },
  ['ItemManager.CraftingTable'] = { 
    serialize = function(writer, value)
      writer:WriteInt(CraftingTable[value])
    end,
    deserialize = function(reader)
      local value = reader:ReadInt()
      for k, v in pairs(CraftingTable) do
        if v == value then
          return k
        end
      end
      error('unknown CraftingTable ' .. value)
    end
  }
  --Decimal = true
}



-- https://github.com/blaxxun-boop/ServerSync/blob/5fc2d710f44c6fe9b35fc707217879f8576414e9/ConfigSync.cs#L949
local ConfigToBytes = function(config)
  local bytes = Bytes.new()
  local writer = DataWriter.new(bytes)
  
  for section, map in pairs(config.cfg) do
    for key, entry in pairs(map) do
      WriteEntryToPackage(writer, section, key, entry)
    end
  end
  
  return bytes
end



-- L 451
local BytesToConfig = function(bytes)
  local reader = DataReader.new(bytes)
  
  local count = reader:ReadUInt() assert(count < 1024, 'config count seems too large')
  for i=1, #count do
    local groupName = reader:ReadString()
    local configName = reader:ReadString()
    local qualifiedTypeName = reader:ReadString()
    
    local found = false
    
    -- manual similarity search
    for k, v in pairs(TOML_TO_DATA_NAMES) do
      if qualifiedTypeName:contains(v) then
        found = true
        break
      end
    end
    
    if found then
      -- read entry completely from package
      --reader:Read()
    else
      print('unknown type being read ' .. qualifiedTypeName)
    end
    
  end
end

local WriteEntryToPackage = function(writer, section, key, entry)
  local qualifiedTypeName = TOML_TO_DATA_NAMES[entry.tomlTypeName]
  
  if qualifiedTypeName then
    writer:Write(section)
    writer:Write(key)
    writer:Write(qualifiedTypeName)
    
    local conv = DATA_CONVERTERS[qualifiedTypeName]
    if conv then
      local func = conv.serialize
      if type(func) == 'string' then
        writer[func](writer, entry.value)
      elseif type(func) == 'function' then
        func(writer, entry.value)
      else
        error('CP: unsupported serializer specifier for type ' .. qualifiedTypeName)
      end
    else
      error('missing serializer for type ' .. qualifiedTypeName)
    end
  else
    error('CP: missing toml->data function name translation ' .. section .. ':' .. key)
  end
end

-- most frequent used types:
--  bool, int32, string, float

-- L 1013
local BytesToType = function(reader, typeName)
  --if typeName
end


local sendZPackage = function(peer, bytes)
  if #bytes > 10000 then
    bytes:Move(Deflater.raw():Compress(bytes))
    --bytes:
  end
end

Valhalla:Subscribe('Connect', function(peer)
  local map = {}
  PEERS[peer.socket.host] = map

  for name, config in pairs(CONFIGS) do
    if config.modRequired then
      local bytes = Bytes.new()
      local writer = DataWriter.new(bytes)
      
      writer:Write(name)
      writer:Write(config.minVersion) -- min
      writer:Write(config.version) -- current 
      
      print('Sending ' .. name .. ' ' 
        .. config.version .. ' to ' .. peer.socket.host)
      
      peer:Invoke(SIG_VersionCheck, bytes)
    end
  end
end)

local RPC_ConfigSync = function(peer, bytes)
  
end

return Config