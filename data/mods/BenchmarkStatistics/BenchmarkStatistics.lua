--[[
    
    Benchmark mod
        record data on zdos
        Revision changes per second (primarily data, also owner)
        
        0 sfx_dodge
        0 sfx_eat
        0 sfx_unarmed_swing
        0 sfx_jump
        0 vfx_clubhit
        0 sfx_unarmed_hit
        62.638948626045 vfx_Cold
        519.88893974066 Player
        
        dont know how accurate these are 
        representative of real worst case-maximums (assuming all is correct)
        player deltas highest
        519/second is a lot,
        
        rather create a excel graph with actual values along with
            times + value
--]]

local dataRevsMax = { }

Valhalla:Subscribe('ZDOUnpacked', function(peer, zdo)
    -- determine greatest change
    
    local id = tostring(zdo.id)
    
    local cur_time = assert(Valhalla.time)
    local prefab = zdo.prefab.name
    local dataRev = zdo.dataRev
    
    local mapping = dataRevsMax[id]
    if mapping then
        -- determine 
        
        -- get the delta since last time and find the avg change in dataRev
        -- if the delta time is really small, change can appear large
        local delta_detaRev = (dataRev - mapping.prev_dataRev) / (cur_time - mapping.prev_time)
        
        if delta_detaRev > mapping.max_delta_dataRev then
            mapping.max_delta_dataRev = delta_detaRev
        end
    else
        mapping = { 
            max_delta_dataRev = 0,
            ['prefab'] = prefab
        }
        dataRevsMax[id] = mapping
    end
    
    mapping.prev_time = cur_time
    mapping.prev_dataRev = dataRev
end)

Valhalla:Subscribe('Periodic', function()
    local prefabs = {  }

    local sorted = {}

    for k, mapping in pairs(dataRevsMax) do
        local prefab = mapping.prefab
        if not prefabs[prefab] then
            sorted[#sorted + 1] = mapping
            prefabs[prefab] = true
        end
    end    

    -- descending
    --table.sort(sorted, function(a, b) return a < b end)
    table.sort(sorted, function(a, b) return a.max_delta_dataRev < b.max_delta_dataRev end)

    local max = #sorted
    if max > 10 then max = 10 end

    print('-----------------')

    for i=1, max do
        local mapping = sorted[i]
        print(mapping.max_delta_dataRev, mapping.prefab)
    end
end)
