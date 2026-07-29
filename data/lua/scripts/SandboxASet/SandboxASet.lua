--[[
    SandboxASet
        A userdata is being modified (globally), 
        then shall be read in another environment,
            But remain unmodified
--]]

local tests = { function() -- [1]
    -- Hmm...
    assert(Vector3f.nefarious == nil, 'Plugins might be loading out of order, fix this!')

    -- Main part
    print('Set userdata global "Vector3f.nefarious" to "bad"')
    Vector3f.nefarious = "bad"
end, function() -- [2]
    assert(Avledet.nefarious == nil, 'Plugins might be loading out of order, fix this!')

    -- Main part
    print('Set userdata global "Avledet.nefarious" to "bad"')
    Avledet.nefarious = "bad"
end }

-- Call test

assert(not pcall(tests[1]))
assert(not pcall(tests[2]))
