--[[
    Experimental rpc logging mod
    
    see which WNT rpcs and what-not cause damage to a player-protected area
--]]

local WARD_PREFAB = PrefabManager.GetPrefab("guard_stone")

Valhalla.OnEvent("ZDOChange", function(original, modified)
    print("ZDOChange")

    local any = ZDOManager.AnyZDO(original.value.pos, 32, WARD_PREFAB.hash)
    
    print("ZDOChange1 " .. ((any ~= nil) and 'true' or 'false'))
    
    -- first determine whether there any nearby wards
    if any ~= nil then
        print("ZDOChange2")
    
        -- prevent the action
        local ward = Views.Ward.new(any)
        
        --if ward.creator.IsPermitted
        print("ZDOChange3")
        -- just try blocking anyways
        event.Cancel()
    end    
end)
