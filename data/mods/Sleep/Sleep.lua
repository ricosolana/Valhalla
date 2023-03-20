--[[
    
    Sleep mod
    
--]]

-- Constants
--  change these values:
local UNIVERSAL_SLEEP = false

-- Runtime variables
-- DO NOT CHANGE THESE
local sleeping = false
local sleepingUntil = 0

Valhalla:Subscribe('PeriodUpdate', function()
    
    if sleeping then
        if UNIVERSAL_SLEEP then
            -- invoke route to select peers
            assert(false, 'not implemented')
        else
            -- invoke to all peers
        end
    else
    
    end
    
end)
