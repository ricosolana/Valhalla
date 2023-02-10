--[[
    Implementation of the compress mod originally used on the Comfy Valheim server (2/9/2023)
    https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

    Ported by crzi for use on the C++ Valhalla server
--]]

local PORTAL = VUtils.String.GetStableHashCode("portal_wood")
--local (TARGETa, TARGETb) = ZDOManager.HashZDOID("target") -- VUtils.String.GetStableHashCode("target") -- use special zdoid hash
local TAG = VUtils.String.GetStableHashCode("tag")

Valhalla.OnEvent("PeriodUpdate", function()
    print("Linking portals")
    
	local portals = ZDOManager.GetZDOs(PORTAL)

	for i1=1, #portals do
        print("i1 " .. tostring(i1))
		local zdo1 = portals[i1]
        
        print("zdo1.owner " .. tostring(zdo1.owner))

		--local target1 = zdo1:GetZDOID(TARGETa, TARGETb)
        local target1 = zdo1:GetZDOID("target")
		local tag1 = zdo1:GetString(TAG, "")

        print("target1.uuid " .. tostring(target1.uuid))
        print("target1.id " .. tostring(target1.id))

		-- if portal has a set target
		--if target1 ~= ZDOID.new() then
        --if target1 then
        if target1 ~= ZDOID.none then
            print("has target")
			local zdo2 = ZDOManager.GetZDO(target1)
            --local tag2 = zdo2:GetString(TAG, "")
            
            --print("zdo2 tag " .. tag2)

			-- if portal target doesnt exist or different tags,
			-- then reset immediately
			if not zdo2 or zdo2:GetString(TAG, "") ~= tag1 then
                print("resetting portal...")
            
				zdo1:SetLocal()

				zdo1:Set("target", ZDOID.new());
				ZDOManager.ForceSendZDO(zdo1.id);
                
                print("reset portal...")
			end
		else 
            print("no target")
			-- find other portals with the same tag
			for i2=i1, #portals do
                print("i2 " .. tostring(i1))

				-- ignore k1 iterated portal
				--if i2 == i1 then do break end end -- continue
                
                -- skip checker portal
                local zdo2 = portals[i2]
                
                if i2 ~= i1 and zdo1:GetZDOID("target") == ZDOID.none then
                    local tag2 = zdo2:GetString(TAG, "")
                    
                    --print("target1.uuid " .. tostring(target1.uuid))
                    --print("target1.id " .. tostring(target1.id))
                    
                    print("zdo2 tag " .. tag2)

                    -- link portals with same tag
                    if tag1 == tag2 then
                        print("linking portals")
                    
                        zdo1:SetLocal()
                        zdo2:SetLocal()
                        zdo1:Set("target", zdo2.id);
                        zdo2:Set("target", zdo1.id);
                        ZDOManager.ForceSendZDO(zdo1.id);
                        ZDOManager.ForceSendZDO(zdo2.id);
                    end
                end
			end
		end
	end
end)
