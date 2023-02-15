--[[
    Experimental rpc logging mod
    
    see which WNT rpcs and what-not cause damage to a player-protected area
--]]

local PREFAB_ZDO_MEMBERS = {
    VUtils.String.GetStableHashCode()
}

local WARD_HASH = VUtils.String.GetStableHashCode("guard_stone")

Valhalla.OnEvent("ZDOChange", function(original, modified)
    local creator = original.GetLong("creator", 0)
    
    -- for is evaluated once at the start, so this is fine
    for i=1, original.GetInt("permitted") do
        
    end
    
    ZDOManager.GetZDOs(original.pos, WARD_HASH, 32)
    
end)
