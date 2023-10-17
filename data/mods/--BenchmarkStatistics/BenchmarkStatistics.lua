--[[
    
    Benchmark mod
        record data on zdos
        Revision changes per second (primarily data, also owner)
        
        initial benchmarks:
            0 sfx_dodge
            0 sfx_eat
            0 sfx_unarmed_swing
            0 sfx_jump
            0 vfx_clubhit
            0 sfx_unarmed_hit
            62.638948626045 vfx_Cold
            519.88893974066 Player

        post benchmark:
            player: 
                min: 315/s, 
                max: 1600/s, 
                median: 345/s, 
                mean: 392/s,
                st.dev: 179/s,
        
        dont know how accurate these are 
        representative of real worst case-maximums (assuming all is correct)
        player deltas highest
        519/second is a lot,
        
        rather create a excel graph with actual values along with
            times + value
            
        alternative
            iterate/record all zdos per second
                timesteps will be mostly* consistent
                the only variance is change in dataRev
--]]

local dataRevsPlot = {}

local recordCount = 0

local my_periodic = function()
    -- seconds elapsed
    if #NetManager.peers == 0 then
        return
    end
    
    recordCount = recordCount + 1

    -- iterate all zdos
    local zdos = ZDOManager:GetZDOs()
    for i=1, #zdos do
        -- record
        local zdo = zdos[i]
        local id = tostring(zdo.id)
        local mapping = dataRevsPlot[id]
        if mapping then
            local rec = mapping.records
            rec[#rec + 1] = zdo.dataRev
        else
            mapping = { 
                prefab = zdo.prefab.name,
                records = {}
            }
            dataRevsPlot[id] = mapping
        end
    end
    
    local interval = 60
    
    if (recordCount % interval) == 0 then
        -- dump contents to new file
        local file, err = io.open(
            'records' .. (recordCount // interval) .. '.csv', 
            'w'
        )
        
        print('creating records file...')
        
        if file then
            for k, v in pairs(dataRevsPlot) do
                local rec = v.records
                file:write(v.prefab, ', ')
                for i=1, #rec do
                    file:write(tostring(rec[i]), ', ')
                end
                file:write('\n')
            end
            file:close()
        else
            print('recordCount file error', err)
        end
    end
end

Valhalla:Subscribe('Periodic', function()
    local status, err = pcall(my_periodic)
    if not status then
        print('error calling periodic', err)
    end
end)

--[[
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
end)--]]
