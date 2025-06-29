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
    
    if Avledet.worldTime > sleepingUntil then
        local peers = NetManager.peers
    
        for i=1, #peers do
            local peer = peers[i]
            local zdo = peer.zdo
            if zdo and zdo:get_bool('inBed') then
                peer:route(SIG_SleepStop)
            end
        end
        
        print('ending sleep')
        
        sleeping = false
        Avledet.world_time_multiplier = 1
        
        event.unsubscribe()
    end
end

Avledet:subscribe('Periodic', function()
    if not sleeping then
        if Avledet.is_afternoon or Avledet.is_night then
            --print('afternoon / night')
            
            local peers = NetManager.peers
            
            if #peers == 0 then return end
            
            local sleepingPeers = {}
            
            for i=1, #peers do
                local peer = peers[i]
                local zdo = peer.zdo
                if zdo then
                    local inBed = zdo:get_bool('inBed')
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
            sleepingUntil = Avledet.next_morning
            Avledet.world_time_multiplier = (sleepingUntil - Avledet.world_time) / 12
            
            for i=1, #sleepingPeers do
                local peer = sleepingPeers[i]
                peer:route(SIG_SleepStart)
            end
            
            -- enable high accuracy sleep timings to not skip 1/12 of the day
            Avledet:subscribe('Update', UPDATE_SLEEP_FN)            
        end
    end    
end)
