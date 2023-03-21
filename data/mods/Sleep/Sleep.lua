--[[
    
    Sleep mod
    
--]]

-- Constants
--  change these values:
local UNIVERSAL_SLEEP = true

-- Runtime variables
-- DO NOT CHANGE THESE
local sleeping = false
local sleepingUntil = 0

local SIG_SleepStart = MethodSig.new('SleepStart')
local SIG_SleepStop = MethodSig.new('SleepStop')

Valhalla:Subscribe('PeriodUpdate', function()
    
    if sleeping then
        if Valhalla.worldTime > sleepingUntil then
            if UNIVERSAL_SLEEP then
                -- invoke to all peers
                RouteManager:InvokeAll(SIG_SleepStop)
            else            
                -- invoke route only to in-bed peers
                assert(false, 'not implemented')
            end
            
            print('ending sleep')
            
            sleeping = false
            Valhalla.worldTimeMultiplier = 1
        end
    else
        if Valhalla.isAfternoon or Valhalla.isNight then
            print('afternoon / night')
        
            local zdos = ZDOManager:GetZDOs('Player')
            if #zdos == 0 then
                print('no peers online')
                return
            end
            
            local allInBed = true
            
            for i=1, #zdos do
                local zdo = zdos[i]
                if not zdo:GetBool('inBed') then
                    print('some not sleeping')
                    allInBed = false
                    return
                end
            end
            
            -- initiate sleep if all in bed
            if allInBed then
                print('starting sleep')
            
                sleeping = true
                
                sleepingUntil = Valhalla.tomorrowMorning

                Valhalla.worldTimeMultiplier = (sleepingUntil - Valhalla.worldTime) / 12
                
                if UNIVERSAL_SLEEP then
                    RouteManager:InvokeAll(SIG_SleepStart)
                else
                    assert(false, 'not implemented')                
                end
            end
            
        end
    end
    
end)
