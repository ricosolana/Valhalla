-- https://stackoverflow.com/a/22843701
string.startswith = function(self, str)
  --return self:find('^' .. str) ~= nil
  return self:sub(1, #str) == str
end

string.endswith = function(self, str)
  return self:sub(-#str) == str
end

-- https://stackoverflow.com/a/27455195
string.trim = function(self)
  return self:match( '^%s*(.-)%s*$' )
end

string.lines = function(self)
  if self:sub(-1) ~= '\n' then self = self .. '\n' end
  return self:gmatch('(.-)\n')
end

string.indexof = function(self, str)
  return self:find(str, 1, true)
end

string.contains = function(self, str)
  return self:find(str, 1, true) ~= nil
end



local TOML = {}

TOML.read = function(file, customTomlConverters)
  local map = {}
  
  local section = ''
  local tomlTypeName = nil
  
  local lineNumber = 1
  for line in io.lines(file) do 
    section, tomlTypeName = TOML.parseline(lineNumber, section, tomlTypeName, customTomlConverters, map, line)
    lineNumber = lineNumber + 1
  end
  
  return map
end

TOML.parse = function(str, customTomlConverters)
  return TOML.parselines(str:lines(), customTomlConverters)
end

TOML.parselines = function(lines, customTomlConverters)
  local map = {}
    
  local section = ''
  local tomlTypeName = nil
    
  local lineNumber = 1
  for line in lines do 
    section, tomlTypeName = TOML.parseline(lineNumber, section, tomlTypeName, customTomlConverters, map, line)
    lineNumber = lineNumber + 1
  end
  
  return map
end
--[[
local NUMBER_TYPES = { 
  Byte = true, 
  SByte = true,
  Int16 = true,
  UInt16 = true,
  Int32 = true,
  UInt32 = true,
  Int64 = true,
  UInt64 = true,
  Single = true,
  Double = true,
  --Decimal = true
}--]]

local CraftingTable = {
  Disabled = 0,
  Inventory = 1,
  Workbench = 2,
  Cauldron = 3,
  Forge = 4,
  ArtisanTable = 5,
  StoneCutter = 6,
  MageTable = 7,
  BlackForge = 8,
  Custom = 9,
}

local FROM_TOML_NUMBER = function(raw)
  return tonumber(raw)
end

local FROM_TOML_DEFAULT = function(raw)
  return raw
end

local TO_TOML_NUMBER = function(number)
  return tostring(number)
end

local TOML_CONVERTERS = {
  String = { from = FROM_TOML_DEFAULT },
  Boolean = { 
    from = function(raw)
      if raw == 'true' then
        return true
      elseif raw == 'false' then
        return false
      else
        return nil
      end
    end,
    to = function(value)
      if value then
        return 'true'
      else
        return 'false'
      end
    end
  },
  SByte = { from = FROM_TOML_NUMBER },
  Byte = { from = FROM_TOML_NUMBER },
  Int16 = { from = FROM_TOML_NUMBER },
  UInt16 = { from = FROM_TOML_NUMBER },
  Int32 = { from = FROM_TOML_NUMBER },
  UInt32 = { from = FROM_TOML_NUMBER },
  Int64 = { from = FROM_TOML_NUMBER },
  UInt64 = { from = FROM_TOML_NUMBER },
  Single = { from = FROM_TOML_NUMBER },
  Double = { from = FROM_TOML_NUMBER },
  --CraftingTable = {}
}



TOML.parseline = function(lineNumber, section, tomlTypeName, customTomlConverters, map, line)
  line = line:trim()
  
  -- skip comments and blanks
  if #line > 0 then
    -- try to extract type information from comment
    if line:startswith('#') then
      local S = '# Setting type:'
      if line:startswith(S) then
        tomlTypeName = line:sub(#S + 1):trim()
      end
    else
      if line:startswith('[') and line:endswith(']') then
        section = line:sub(2, #line - 1)
        map[section] = {}
      else
        local delim = line:find('=', 1, true)
        if delim then
          local key = line:sub(1, delim - 1):trim()
          local rawValue = line:sub(delim + 1, #line):trim()
          
          local entry = {}
          entry.tomlTypeName = tomlTypeName
          
          if tomlTypeName == nil then
            error('no type defined L' .. lineNumber)
          else
            local conv = TOML_CONVERTERS[tomlTypeName] or (customTomlConverters and customTomlConverters[tomlTypeName])
            if conv then
              entry.value = assert(conv.from(rawValue), 'failed to parse type L' .. lineNumber)
            else              
              print('no toml converter for ' .. tomlTypeName .. ' L' .. lineNumber .. ' (using value as-is)')
              entry.value = FROM_TOML_DEFAULT(rawValue)
            end
          end
          
          map[section][key] = entry
          
          -- reset consumed typename
          tomlTypeName = nil
        end
      end
    end
  end
  
  return section, tomlTypeName
end

--local map = toml.read('WackyMole.RareMagicPortal.cfg')

--local map = toml.parse(io.lines('WackyMole.RareMagicPortal.cfg'))
--[[
local map = toml.parse(io.open('WackyMole.RareMagicPortal.cfg', 'r'):read('*all'))

if true then
  print(map)
end--]]

return TOML