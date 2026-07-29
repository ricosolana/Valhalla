--[[
    SandboxBGet
        Now, we are reading from the "Vector3f.nefarious"
--]]





local do_test_number = 2

local tests = { function() -- [1]
    -- Hmm...
    print('Reading from "Vector3f.nefarious"')
    assert(Vector3f.nefarious == nil, 'Sandbox was escaped!')
end, function() -- [2]
    print('Reading from "Avledet.nefarious"')
    assert(Avledet.nefarious == nil, 'Sandbox was escaped!')
end }

-- Call test
tests[do_test_number]()
