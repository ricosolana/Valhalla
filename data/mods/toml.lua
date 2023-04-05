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

local TOML = {}

TOML.read = function(file)
  local map = {}
  
  local section = ''
  
  for line in io.lines(file) do 
    section = TOML.parseline(section, map, line)
  end
  
  return map
end

TOML.parse = function(str)
  return TOML.parselines(str:lines())
end

TOML.parselines = function(lines)
  local map = {}
  
  local section = ''
    
  for line in lines do 
    section = TOML.parseline(section, map, line)
  end
  
  return map
end

TOML.parseline = function(section, map, line)
  line = line:trim()
  -- skip comments and blanks
  if #line > 0 and not line:startswith('#') then
    --lines[#lines + 1] = line
    if line:startswith('[') and line:endswith(']') then
      section = line:sub(2, #line - 1)
      map[section] = {}
    else
      local delim = line:find('=', 1, true)
      if delim then
        local key = line:sub(1, delim - 1):trim()
        local value = line:sub(delim + 1, #line):trim()
        
        local pvalue = value
        
        if value == 'true' then
          pvalue = true
        elseif value == 'false' then
          pvalue = false
        else
          pvalue = tonumber(value) or pvalue
        end
        
        map[section][key] = pvalue
      end
    end
  end
  
  return section
end

--local map = toml.read('WackyMole.RareMagicPortal.cfg')

--local map = toml.parse(io.lines('WackyMole.RareMagicPortal.cfg'))
--[[
local map = toml.parse(io.open('WackyMole.RareMagicPortal.cfg', 'r'):read('*all'))

if true then
  print(map)
end--]]

return TOML