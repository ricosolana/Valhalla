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
  String =   { qualifier = 'System.String',   serialize = 'write',        deserialize = 'read_string'  },
  Boolean =  { qualifier = 'System.Boolean',  serialize = 'write',        deserialize = 'read_bool'    },
  SByte =    { qualifier = 'System.SByte',    serialize = 'WriteInt8',    deserialize = 'read_int8'    },
  Byte =     { qualifier = 'System.Byte',     serialize = 'WriteUInt8',   deserialize = 'read_uint8'   },
  Int16 =    { qualifier = 'System.Int16',    serialize = 'WriteInt16',   deserialize = 'read_int16'   },
  UInt16 =   { qualifier = 'System.UInt16',   serialize = 'WriteUInt16',  deserialize = 'read_uint16'  },
  Int32 =    { qualifier = 'System.Int32, mscorlib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089',    serialize = 'WriteInt32',   deserialize = 'read_int32'   },
  UInt32 =   { qualifier = 'System.UInt32',   serialize = 'WriteUInt32',  deserialize = 'read_uint32'  },
  Int64 =    { qualifier = 'System.Int64',    serialize = 'write',        deserialize = 'read_int64'   },
  UInt64 =   { qualifier = 'System.UInt64',   serialize = 'write',        deserialize = 'read_uint64'  },
  Single =   { qualifier = 'System.Single, mscorlib, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089',   serialize = 'WriteFloat',   deserialize = 'read_float'   },
  Double =   { qualifier = 'System.Double',   serialize = 'WriteDouble',  deserialize = 'read_double'  },
  CraftingTable = { 
    qualifier = 'ItemManager.CraftingTable',
    underlying = 'System.Int32',
    serialize = function(writer, value)
      writer:WriteInt32(CraftingTableType[value])
    end,
    deserialize = function(reader)
      return assert(TOML.ORDINAL_TO_ENUM(CraftingTableType, reader:read_int32()), 'unknown CraftingTable')
    end
  },
  Color = {
    qualifier = 'UnityEngine.Color',
    serialize = function(writer, value)
      writer:WriteFloat(value.r)
      writer:WriteFloat(value.g)
      writer:WriteFloat(value.b)
      writer:WriteFloat(value.a)
    end,
    deserialize = function(reader)
      return {
        r = reader:read_float(),
        g = reader:read_float(),
        b = reader:read_float(),
        a = reader:read_float()
      }
    end
  },
  BuildPieceCategory = { 
    qualifier = 'PieceManager.BuildPieceCategory',
    underlying = 'System.Int32',
    serialize = function(writer, value)
      writer:WriteInt32(BuildPieceCategory[value])
    end,
    deserialize = function(reader)
      return assert(TOML.ORDINAL_TO_ENUM(BuildPieceCategory, reader:read_int32()), 'unknown BuildPieceCategory')
    end
  },
  DamageModifier = { 
    --qualifier = 'HitData+DamageModifier',
    qualifier = 'ItemManager.Item+DamageModifier',
    underlying = 'System.Int32',
    serialize = function(writer, value)
      writer:WriteInt32(DamageModifier[value])
    end,
    deserialize = function(reader)
      return assert(TOML.ORDINAL_TO_ENUM(DamageModifier, reader:read_int32()), 'unknown DamageModifier')
    end
  },
  --Decimal = {}
}



--[[
  Variables
--]]
Config.CONFIGS = {}

Config.PEERS = {}



Config.new = function(self)  
  print('registering sync config for ' .. self.name)
    
  self.configSyncSig = MethodSig.new(self.name .. ' ConfigSync', Type.BYTES)
  
  Config.CONFIGS[self.name] = self
  
  --[[
  RouteManager:Register(self.configSyncSig, function(peer, bytes)
    if not self.locked or peer.admin then
      --then update config values sent by peer
      local reader = DataReader.new(bytes)
      
      local flags = reader:read_uint8()
      if flags & CONFIG_COMPRESSED
    end
  end)
  --]]
  
  self.loadFile = function(self, configPath, tomlTypeConverters)
    self.cfg = TOML.read(configPath, tomlTypeConverters)
  end
  
  return self
end



-- most frequent used types:
--  bool, int32, string, float



-- https://github.com/blaxxun-boop/ServerSync/blob/5fc2d710f44c6fe9b35fc707217879f8576414e9/ConfigSync.cs#L949
Config.SerializeConfig = function(config, extra)
  local bytes = Bytes.new()
  local writer = DataWriter.new(bytes)
  
  writer:WriteUInt8(0)
  writer:WriteInt32(table.size(config.cfg) + table.size(extra))
  
  for section, map in pairs(config.cfg) do
    for key, entry in pairs(map) do
      Config.SerializeEntry(writer, config.dataConverters, section, key, entry)
    end
  end
  
  if extra then
    for section, map in pairs(extra) do
      for key, entry in pairs(map) do
        Config.SerializeEntry(writer, config.dataConverters, section, key, entry)
      end
    end
  end
  
  return bytes
end

Config.SerializeEntry = function(writer, dataConverters, section, key, entry)
  local cvt = assert(
    DATA_CONVERTERS[entry.tomlTypeName] or dataConverters[entry.tomlTypeName], 
    'missing serializer for type ' .. entry.tomlTypeName
  )

  writer:write(section)
  writer:write(key)
  writer:write(cvt.underlying or cvt.qualifier)
  
  print(entry.value)
  
  local func = cvt.serialize
  if type(func) == 'string' then
    writer[func](writer, entry.value)
  elseif type(func) == 'function' then
    func(writer, entry.value)
  else
    error('CP: unsupported serializer specifier for type ' .. qualifiedTypeName)
  end
end



-- this is when receiving a config from player
-- L 451
--[[
local BytesToConfig = function(config, bytes)
  local reader = DataReader.new(bytes)
  
  local count = reader:ReadUInt() assert(count < 1024, 'config count seems too large')
  for i=1, #count do
    local section = reader:read_string()
    local key = reader:read_string()
    local qualifiedTypeName = reader:read_string()
    
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
      value = reader:deserialize(cvt.dataType)
      
    else
      print('unknown type being read ' .. qualifiedTypeName)
    end
    
  end
end--]]



Config.sendZPackage = function(peer, config, bytes)
  if #bytes > 10000 then
    local newBytes = Bytes.new()
    local writer = DataWriter.new(newBytes)
    
    writer:WriteUInt8(CONFIG_COMPRESSED)
    writer:write(Deflater.raw:Compress(bytes))
    
    bytes = newBytes
  end
  
  -- screw packet fragmentation
  --  why the fuck are you manually managing networking?
  --  this isnt pre-tcp, packets are reliable anyways, so... ???
  
  peer:Invoke(config.configSyncSig, bytes)
end

Valhalla:Subscribe('Connect', function(peer)  
  -- https://github.com/blaxxun-boop/ServerSync/blob/master/ConfigSync.cs#L1224
  peer:Register(SIG_VersionCheck, function(peer, bytes)
    local reader = DataReader.new(bytes)
    
    local name = reader:read_string()
    
    local config = Config.CONFIGS[name]
    
    print('received mod from peer ' .. name)
    --[[
    if config then
      local minVersion = reader:read_string() -- semver is more to implement...
      local version = reader:read_string()
      
      print('Received', name, version, 'from peer')
      
      -- peer is good 
      if config.version == version then
        map[name] = true
      end
    end--]]
  end)
  
  
  
  for name, config in pairs(Config.CONFIGS) do
    --if config.modRequired then
      local bytes = Bytes.new()
      local writer = DataWriter.new(bytes)
      
      writer:write(name)
      writer:write(config.minVersion) -- min
      writer:write(config.version) -- current
      
      print('Sending ' .. name .. ' ' 
        .. config.version .. ' to ' .. peer.socket.host)
      
      peer:Invoke(SIG_VersionCheck, bytes)
    --end
  end
end)

-- https://github.com/blaxxun-boop/ServerSync/blob/5fc2d710f44c6fe9b35fc707217879f8576414e9/ConfigSync.cs#L838
Valhalla:Subscribe('Join', function(peer)
  -- send configs to peer
  local map = {}
  
  local configsToSend = {}
  for name, config in pairs(Config.CONFIGS) do
    configsToSend[name]= config
  end
  
  map.configsToSend = configsToSend
  Config.PEERS[peer.socket.host] = map
end)

Valhalla:Subscribe('Periodic', function()
  -- try sending an unsent config to every peer
  for _, peer in ipairs(NetManager.peers) do
    local map = Config.PEERS[peer.socket.host]
    local configsToSend = map.configsToSend
    
    local name, config = next(configsToSend, nil)
    if name then
      configsToSend[name] = nil
      
      local extra = {}
      local internal = {}
      
      internal['serverversion'] = { value = config.version, tomlTypeName = 'String' }
      internal['lockexempt'] = { value = peer.admin, tomlTypeName = 'Boolean' }
      
      extra['Internal'] = internal
      
      local bytes = Config.SerializeConfig(config, extra)
      
      -- send a config
      Config.sendZPackage(peer, config, bytes)
    end
  end
end)

Valhalla:Subscribe('Quit', function(peer)
  Config.PEERS[peer.socket.host] = nil
end)

return Config