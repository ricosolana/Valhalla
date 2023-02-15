--[[
    Vanilla portal mod to connect portals together throughout gameplay

    Created by crzi for use on the C++ Valhalla server

    I never knew this, but tags are not fully unique
        Once a portal is paired with another with a certain tag,
        another pair of portals can be paired with the same tag as the first pair

        my code below also exhibits this, (un)fortunately? so.
--]]

local PORTAL = VUtils.String.GetStableHashCode("portal_wood")
--local (TARGETa, TARGETb) = ZDOManager.HashZDOID("target") -- VUtils.String.GetStableHashCode("target") -- use special zdoid hash
local TAG = VUtils.String.GetStableHashCode("tag")

Valhalla.OnEvent("PeriodUpdate", function()
	local portals = ZDOManager.GetZDOs(PORTAL)

	for i1=1, #portals do
		local zdo1 = portals[i1]
        
        local target1 = zdo1:GetZDOID("target")
		local tag1 = zdo1:GetString(TAG, "")

		-- if target portal assigned
        if target1 ~= ZDOID.none then
			local zdo2 = ZDOManager.GetZDO(target1)

			-- if target is missing from world, reset target
			if not zdo2 or zdo2:GetString(TAG, "") ~= tag1 then
				zdo1:SetLocal()

				zdo1:Set("target", ZDOID.none);
				ZDOManager.ForceSendZDO(zdo1.id);
			end
		else 
			-- find other portals with the same tag
			for i2=i1, #portals do
                if i2 ~= i1 then
                    
                    local zdo2 = portals[i2]

                    local target2 = zdo2:GetZDOID("target")

                    -- connect unlinked portals
                    if target2 == ZDOID.none then

                        local tag2 = zdo2:GetString(TAG, "")

                        -- link if same tag
                        if tag1 == tag2 then

                            print("linking portals")
                    
                            zdo1:SetLocal()
                            zdo2:SetLocal()
                            zdo1:Set("target", zdo2.id);
                            zdo2:Set("target", zdo1.id);
                            ZDOManager.ForceSendZDO(zdo1.id);
                            ZDOManager.ForceSendZDO(zdo2.id);
                        
                            break -- prevent portals from forming more than 1 link
                        end
                    end
                end
			end
		end
	end
end)
