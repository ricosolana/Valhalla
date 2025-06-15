--[[
    
    TestEnvironment1

        testing whether modifying the data from one env to the next carries over changes

        such as setting the global table to nil here, and it reflecting any changes in other mods
    
--]]



--if not _G['Vector3f'] then
--    print('Vector3f is nil')
--else
--    print('Vector3f is alive!')
--end

print('hello from Env1, avledet!')

--for k,v in pairs(_G) do
--    print("Global key", k, "value", v)
--end

print(os.date("today is %A, in %B"))

print("Vec3f.zero: " .. tostring(Vector3f.ZERO))

--print('plain unassociated x: ' .. tostring(Vector3f.x)) --fails because not userdata instance!

local vec = Vector3f.new(1, 2, 3)

print('vec \'x\':' .. vec.x)

Vector3f.hamburg = 82 --test...

print('vec forced key: "hamburg" :' .. tostring(vec.hamburg))

--print('Setting vec3f to nil now...')
--Vector3f = nil

print('Env1, out!')
