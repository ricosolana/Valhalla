-- ConfigSync

local TOML = require 'toml'

--[[
 Constants
--]]
local SIG_VersionCheck = MethodSig.new('ServerSync VersionCheck', Type.BYTES)

-- table<name, config>
local CONFIGS = {}

-- table<peer.socket.host, table<name, true>> of each peers currently verified mods
--local PEERS = {}

local Config = {}

Config.new = function(name, configPath, version, minVersion, modRequired)
  local self = {}
  
  print('registering sync config for ' .. name)
  
  self.cfg = TOML.read('./mods/' .. configPath)
  self.version = version
  self.minVersion = minVersion
  self.modRequired = modRequired
  
  CONFIGS[name] = self
  
  return self
end

-- strange findings against my initial implementation:
--  server sends version, client receives
--  client NEVER sends version
-- 
-- a vanilla client allowed on modded server is allowed
-- a vanilla server accepting a modded player is allowed...
-- then wtf is the point of mod required?

--

Valhalla:Subscribe('Connect', function(peer)
  local map = {}
  PEERS[peer.socket.host] = map
  
  --[[
  peer:Register(SIG_VersionCheck, function(peer, bytes)
    local reader = DataReader.new(bytes)
    
    local name = reader:ReadString()
    
    local config = CONFIGS[name]
    
    print('received mod from peer ' .. name)
    
    if config then
      local minVersion = reader:ReadString() -- semver is more to implement...
      local version = reader:ReadString()
      
      print('Received', name, version, 'from peer')
      
      -- peer is good 
      if config.version == version then
        map[name] = true
      end
    end
  end)--]]

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

--[[
Valhalla:Subscribe('Join', function(peer)
  local map = PEERS[peer.socket.host]
  
  local result = true
  
  for name, config in pairs(CONFIGS) do
    if config.modRequired then
      if not map[name] then
        print('peer missing mod ' .. name)
        result = false
        break
      end
    end
  end
  
  PEERS[peer.socket.host] = nil
  
  return result
end)
--]]

return Config