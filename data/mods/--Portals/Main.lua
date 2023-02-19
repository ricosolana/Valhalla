--[[
    Vanilla portal mod to connect portals together throughout gameplay

    Created by crzi for use on the C++ Valhalla server

    I never knew this, but tags are not fully unique
        Once a portal is paired with another with a certain tag,
        another pair of portals can be paired with the same tag as the first pair

        my code below also exhibits this, (un)fortunately? so.
--]]

local PORTAL = VUtils.String.GetStableHashCode("portal_wood")

Valhalla.OnEvent("PeriodUpdate", function()
	local portalZdos = ZDOManager.GetZDOs(PORTAL)

	for i1=1, #portalZdos do
		local portalZdo1 = portalZdos[i1]
        local portal1 = Views.Portal.new(portalZdo1)
        
        local target1 = portal1.target
		local tag1 = portal1.tag
        
		-- if target portal assigned
        if target1 ~= ZDOID.none then
			local portalZdo2 = ZDOManager.GetZDO(target1)
            local portal2 = Views.Portal.new(portalZdo2)

			-- if target is missing from world, reset target
			if not portalZdo2 or portal2.tag ~= tag1 then
				portalZdo1:SetLocal()

				portal1.target = ZDOID.none
				ZDOManager.ForceSendZDO(portalZdo1.id);
			end
		else 
			-- find other portalZdos with the same tag
			for i2=i1, #portalZdos do
                if i2 ~= i1 then
                    
                    local portalZdo2 = portalZdos[i2]
                    local portal2 = Views.Portal.new(portalZdo2)
                    
                    -- connect unlinked portals
                    if portal2.target == ZDOID.none then

                        local tag2 = portal2.tag

                        -- link if same tag
                        if tag1 == portal2.tag then

                            print("linking portals")
                    
                            portalZdo1:SetLocal()
                            portalZdo2:SetLocal()
                            portal1.target = portalZdo2.id
                            portal2.target = portalZdo1.id
                            ZDOManager.ForceSendZDO(portalZdo1.id);
                            ZDOManager.ForceSendZDO(portalZdo2.id);
                        
                            break -- prevent portals from forming more than 1 link
                        end
                    end
                end
			end
		end
	end
end)
