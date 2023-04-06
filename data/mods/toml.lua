--require 'stringext'


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



table.size = function(self)
  if not self then return 0 end
  
  local count = 0
  for k, v in pairs(self) do
    count = count + 1
  end
  return count
end








-- global
CraftingTableType = {
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

BuildPieceCategory = {
  Misc = 0,
  Crafting = 1,
  Building = 2,
  Furniture = 3,
  All = 100,
  Custom = 101,
}

-- ItemManager.Item+DamageModifier
DamageModifier = {
	Normal = 0,
	Resistant = 1,
	Weak = 2,
	Immune = 3,
	Ignore = 4,
	VeryResistant = 5,
	VeryWeak = 6,
  None = 7,
}

local TOML = {}

TOML.FROM_NUMBER = function(raw)
  return tonumber(raw)
end

TOML.FROM_DEFAULT = function(raw)
  return raw
end

TOML.TO_NUMBER = function(number)
  return tostring(number)
end

TOML.ORDINAL_TO_ENUM = function(t, ord)
  for k, i in pairs(t) do
    if i == ord then return k end
  end
  return nil
end

TOML.CONVERTERS = {
  String = { from = TOML.FROM_DEFAULT },
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
  SByte =               { from = TOML.FROM_NUMBER   },
  Byte =                { from = TOML.FROM_NUMBER   },
  Int16 =               { from = TOML.FROM_NUMBER   },
  UInt16 =              { from = TOML.FROM_NUMBER   },
  Int32 =               { from = TOML.FROM_NUMBER   },
  UInt32 =              { from = TOML.FROM_NUMBER   },
  Int64 =               { from = TOML.FROM_NUMBER   },
  UInt64 =              { from = TOML.FROM_NUMBER   },
  Single =              { from = TOML.FROM_NUMBER   },
  Double =              { from = TOML.FROM_NUMBER   },
  CraftingTable =       { from = TOML.FROM_DEFAULT  },
  BuildPieceCategory =  { from = TOML.FROM_DEFAULT  },
  Color =               { 
    from = function(raw) 
      -- tryparsehtmlstring
      -- convert RRGGBBAA to { r g b a }
      if #raw ~= 8 then return nil end
      return {
        r = tonumber(raw:sub(1, 2), 16) / 255,
        g = tonumber(raw:sub(3, 4), 16) / 255,
        b = tonumber(raw:sub(5, 6), 16) / 255,
        a = tonumber(raw:sub(7, 8), 16) / 255,
      }
    end,
    to = function(value) 
      -- tohtmlstringrgba
      return string.format('%x%x%x%x', 
        math.floor(value.r * 255),
        math.floor(value.g * 255),
        math.floor(value.b * 255),
        math.floor(value.a * 255)
      )
    end
  },
  DamageModifier =      { from = TOML.FROM_DEFAULT },
}



TOML.read = function(file, tomlTypeConverters)
  local cfg = {}
    
  local temp = { ['tomlTypeConverters'] = tomlTypeConverters }
  for line in io.lines(file) do 
    TOML.parseline(line, cfg, temp)
  end
  
  return cfg
end

TOML.parse = function(str, tomlTypeConverters)
  return TOML.parselines(str:lines(), tomlTypeConverters)
end

TOML.parselines = function(lines, tomlTypeConverters)
  local cfg = {}
    
  local temp = { ['tomlTypeConverters'] = tomlTypeConverters }
  for i, line in pairs(lines) do 
    TOML.parseline(line, cfg, temp)
  end
  
  return cfg
end

--TOML.parseline = function(lineNumber, section, tomlTypeName, map, line)
TOML.parseline = function(line, cfg, temp)
  temp.lineNumber = (temp.lineNumber or 0) + 1
  
  line = line:trim()
  
  -- skip comments and blanks
  if #line > 0 then
    -- try to extract type hint information from comment
    if line:startswith('#') then
      local S = '# Setting type:'
      if line:startswith(S) then
        temp.tomlTypeName = line:sub(#S + 1):trim()
      end
    else
      if line:startswith('[') and line:endswith(']') then
        temp.section = line:sub(2, #line - 1)
        cfg[temp.section] = {}
      else
        local delim = line:find('=', 1, true)
        if delim then
          local key = line:sub(1, delim - 1):trim()
          local rawValue = line:sub(delim + 1, #line):trim()
          
          local entry = {}
          entry.tomlTypeName = temp.tomlTypeName
          
          if temp.tomlTypeName == nil then
            error('no type defined L' .. temp.lineNumber)
          else
            local cvt = TOML.CONVERTERS[temp.tomlTypeName] or (temp.tomlTypeConverters and temp.tomlTypeConverters[temp.tomlTypeName])
            if cvt then
              entry.value = assert(cvt.from(rawValue), 'failed to parse type L' .. temp.lineNumber)
            else              
              print('no toml converter for ' .. temp.tomlTypeName .. ' L' .. temp.lineNumber .. ' (using raw)')
              entry.value = TOML.FROM_DEFAULT(rawValue)
            end
          end
          
          cfg[temp.section][key] = entry
        end
      end
    end
  end
end

--local map = TOML.read('C:/Users/Rico/Documents/CLionProjects/Valhalla/out/build/x64-Debug/data/mods/PotionsPlus/com.odinplus.potionsplus.cfg')

--local map = toml.parse(io.lines('WackyMole.RareMagicPortal.cfg'))
--[[
local map = toml.parse(io.open('WackyMole.RareMagicPortal.cfg', 'r'):read('*all'))

if true then
  print(map)
end--]]

return TOML