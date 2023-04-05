-- ConfigSync

local TOML = require 'toml'

--[[
  Constants
--]]
local CONFIG_PARTIAL    = 1
local CONFIG_FRAGMENTED = 2
local CONFIG_COMPRESSED = 4

local SIG_VersionCheck = MethodSig.new('ServerSync VersionCheck', Type.BYTES)

local Config = {}

local DATA_CONVERTERS = {
  ['String'] =   { package = 'System', dataType = Type.STRING,  serialize = 'Write',        deserialize = 'ReadString' },
  ['Boolean'] =  { package = 'System', dataType = Type.BOOL,    serialize = 'Write',        deserialize = 'ReadBool' },
  ['SByte'] =    { package = 'System', dataType = Type.INT8,    serialize = 'WriteInt8',    deserialize = 'ReadInt8' },
  ['Byte'] =     { package = 'System', dataType = Type.UINT8,   serialize = 'WriteUInt8',   deserialize = 'ReadUInt8' }, 
  ['Int16'] =    { package = 'System', dataType = Type.INT16,   serialize = 'WriteInt16',   deserialize = 'ReadInt16' },
  ['UInt16'] =   { package = 'System', dataType = Type.UINT16,  serialize = 'WriteUInt16',  deserialize = 'ReadUInt16' },
  ['Int32'] =    { package = 'System', dataType = Type.INT32,   serialize = 'WriteInt32',   deserialize = 'ReadInt32' },
  ['UInt32'] =   { package = 'System', dataType = Type.UINT32,  serialize = 'WriteUInt32',  deserialize = 'ReadUInt32' },
  ['Int64'] =    { package = 'System', dataType = Type.INT64,   serialize = 'Write',        deserialize = 'ReadInt64' },
  ['UInt64'] =   { package = 'System', dataType = Type.UINT64,  serialize = 'Write',        deserialize = 'ReadUInt64' },
  ['Single'] =   { package = 'System', dataType = Type.FLOAT,   serialize = 'WriteFloat',   deserialize = 'ReadFloat' },
  ['Double'] =   { package = 'System', dataType = Type.DOUBLE,  serialize = 'WriteDouble',  deserialize = 'ReadDouble' },
  ['CraftingTable'] = { 
    package = 'ItemManager',
    dataType = Type.INT32,
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
  --Decimal = {}
}



--[[
  Variables
--]]
local CONFIGS = {}

local PEERS = {}



Config.new = function(name, version, minVersion, modRequired, locked)
  local self = {}
  
  print('registering sync config for ' .. name)
  
  self.version = version
  self.minVersion = minVersion
  self.modRequired = modRequired
  self.locked = locked
  self.SIG_ConfigSync = MethodSig.new(name .. ' ConfigSync', Type.BYTES)
  
  CONFIGS[name] = self
  
  --[[
  RouteManager:Register(self.SIG_ConfigSync, function(peer, bytes)
    if not self.locked or peer.admin then
      --then update config values sent by peer
      local reader = DataReader.new(bytes)
      
      local flags = reader:ReadUInt8()
      if flags & CONFIG_COMPRESSED
    end
  end)
  --]]
  
  self.loadFile = function(self, configPath, customTomlConverters, customDataConverters)
    self.cfg = TOML.read('./mods/' .. configPath, customTomlConverters, customDataConverters)
    
    --self.cfg['Internal']['serverversion'] = { tomlTypeName = 'String', value = version }
    --self.cfg['Internal']['lockexempt'] =    { tomlTypeName = 'Boolean', value = version }
  end
  
  return self
end



-- most frequent used types:
--  bool, int32, string, float



-- https://github.com/blaxxun-boop/ServerSync/blob/5fc2d710f44c6fe9b35fc707217879f8576414e9/ConfigSync.cs#L949
local SerializeConfig = function(config, extra)
  local bytes = Bytes.new()
  local writer = DataWriter.new(bytes)
  
  for section, map in pairs(config.cfg) do
    for key, entry in pairs(map) do
      SerializeEntry(writer, section, key, entry)
    end
  end
  
  if specialCfg then
    for section, map in pairs(extra) do
      for key, entry in pairs(map) do
        SerializeEntry(writer, section, key, entry)
      end
    end
  end
  
  return bytes
end

local SerializeEntry = function(writer, section, key, entry)  
  local cvt = DATA_CONVERTERS[entry.tomlTypeName]
  if cvt then
    writer:Write(section)
    writer:Write(key)
    writer:Write(cvt.package .. '.' .. entry.tomlTypeName)
    
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
end



-- this is when receiving a config from player
-- L 451
--[[
local BytesToConfig = function(config, bytes)
  local reader = DataReader.new(bytes)
  
  local count = reader:ReadUInt() assert(count < 1024, 'config count seems too large')
  for i=1, #count do
    local section = reader:ReadString()
    local key = reader:ReadString()
    local qualifiedTypeName = reader:ReadString()
    
    local cvt = nil
    
    -- manual similarity search
    --for k, v in pairs(TOML_TO_DATA_NAMES) do
    for tomlTypeName, _cvt in pairs(DATA_CONVERTERS) do
      if qualifiedTypeName:contains(tomlTypeName) then
        cvt = _cvt
        break
      end
    end
        
    if cvt then
      -- read entry completely from package
      value = reader:Deserialize(cvt.dataType)
      
    else
      print('unknown type being read ' .. qualifiedTypeName)
    end
    
  end
end--]]



local sendZPackage = function(config, peer, bytes)
  if #bytes > 10000 then
    local newBytes = Bytes.new()
    local writer = DataWriter.new(newBytes)
    
    writer:WriteInt(CONFIG_COMPRESSED)
    writer:Write(Deflater.raw():Compress(bytes))
    
    bytes = newBytes
  end
  
  -- screw packet fragmentation
  --  why the fuck are you manually managing networking?
  --  this isnt pre-tcp, packets are reliable anyways, so... ???
  
  peer:Invoke(config.SIG_ConfigSync, bytes)
end

Valhalla:Subscribe('Connect', function(peer)
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

-- https://github.com/blaxxun-boop/ServerSync/blob/5fc2d710f44c6fe9b35fc707217879f8576414e9/ConfigSync.cs#L838
Valhalla:Subscribe('Join', function(peer)
  -- send configs to peer
  local map = {}
  
  local configsToSend = {}
  for name, config in pairs(CONFIGS) do
    configsToSend[name]= config
  end
  
  map.configsToSend = configsToSend
  PEERS[peer.socket.host] = map
end)

Valhalla:Subscribe('Periodic', function()
  -- try sending an unsent config to every peer
  for peer in NetManager.peers do
    local map = PEERS[peer]
    local configsToSend = map.configsToSend
    
    local name, config = next(configsToSend, nil)
    if name then
      configsToSend[name] = nil
      
      -- send a config
      
    end
  end
end)

Valhalla:Subscribe('Quit', function(peer)
  PEERS[peer.socket.host] = nil
end)

return Config



