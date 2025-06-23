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
