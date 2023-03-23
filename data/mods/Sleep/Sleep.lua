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

local SIG_SleepStart = MethodSig.new('SleepStart')
local SIG_SleepStop = MethodSig.new('SleepStop')

local UPDATE_SLEEP_FN = function() 
    assert(sleeping)
    
    if Valhalla.worldTime > sleepingUntil then
        local peers = NetManager.peers
    
        for i=1, #peers do
            local peer = peers[i]
            local zdo = peer.zdo
            if zdo and zdo:GetBool('inBed') then
                peer:Route(SIG_SleepStop)
            end
        end
        
        print('ending sleep')
        
        sleeping = false
        Valhalla.worldTimeMultiplier = 1
        
        event.Unsubscribe()
    end
end

Valhalla:Subscribe('PeriodUpdate', function()
    if not sleeping then
        if Valhalla.isAfternoon or Valhalla.isNight then
            --print('afternoon / night')
            
            local peers = NetManager.peers
            
            if #peers == 0 then return end
            
            local sleepingPeers = {}
            
            for i=1, #peers do
                local peer = peers[i]
                local zdo = peer.zdo
                if zdo then
                    local inBed = zdo:GetBool('inBed')
                    if inBed then
                        table.insert(sleepingPeers, peer)
                    else
                        if UNIVERSAL_SLEEP then 
                            return
                        end
                    end
                end
            end
            
            -- peers who are joining during afternoon/night might trigger sleep
            --  this prevents that
            if #sleepingPeers == 0 then return end
            
            -- initiate sleep if all in bed
            print('starting sleep')
            
            sleeping = true
            sleepingUntil = Valhalla.tomorrowMorning
            Valhalla.worldTimeMultiplier = (sleepingUntil - Valhalla.worldTime) / 12
            
            for i=1, #sleepingPeers do
                local peer = sleepingPeers[i]
                peer:Route(SIG_SleepStart)
            end
            
            -- enable high accuracy sleep timings to not skip 1/12 of the day
            Valhalla:Subscribe('Update', UPDATE_SLEEP_FN)            
        end
    end    
end)
