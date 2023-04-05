-- ConfigSync

local TOML = require 'toml'

--[[
  Constants
--]]
local SIG_VersionCheck = MethodSig.new('ServerSync VersionCheck', Type.BYTES)

local CONFIGS = {}

local Config = {}

Config.new = function(name, configPath, version, minVersion, modRequired, locked)
  local self = {}
  
  print('registering sync config for ' .. name)
  
  self.cfg = TOML.read('./mods/' .. configPath)
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
  
  return self
end



-- https://github.com/blaxxun-boop/ServerSync/blob/5fc2d710f44c6fe9b35fc707217879f8576414e9/ConfigSync.cs#L949
local ConfigToBytes = function(config)
  local bytes = Bytes.new()
  local writer = DataWriter.new(bytes)
  
  for section, map in pairs(config.cfg) do
    writer:Write(section)
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
    local typeName = reader:ReadString()
    
    
  end
end

-- most frequent used types:
--  bool, int32, string, float
--[[
local QUALIFIED_TYPES_CONVERTERS = {
  'System.String' = function(reader) end,
  'System.Boolean' = function(reader) end,
  'System.Byte' = function(reader) end,
  'System.SByte' = function(reader) end,
  'System.Int16' = function(reader) end,
  'System.UInt16' = function(reader) end,
  'System.Int32' = function(reader) end,
  'System.UInt32' = function(reader) end,
  'System.Int64' = function(reader) end,
  'System.UInt64' = function(reader) end,
  'System.Single' = function(reader) end,
  'System.Double' = function(reader) end,
  --'System.Decimal' = function(reader) end,
  'System.Enum' = function(reader) end
}--]]

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