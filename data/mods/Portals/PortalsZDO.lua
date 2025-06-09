--[[
    Vanilla portal mod to connect portals together throughout gameplay
    
        This is the raw View-less version (utilizes ZDOs get/set only)

    Created by crzi for use on the C++ Valhalla server

    I never knew this, but tags are not fully unique
        Once a portal is paired with another with a certain tag,
        another pair of portals can be paired with the same tag as the first pair

        my code below also exhibits this, (un)fortunately? so.
--]]

Valhalla:subscribe('Periodic', function()
	local portalZdos = ZDOManager:get_zdos('portal_wood')

	for i1=1, #portalZdos do
		local portalZdo1 = portalZdos[i1]
        --local portal1 = Views.Portal.new(portalZdo1)
                
        --local target1 = portal1.target
		--local tag1 = portal1.tag
        
        local target1 = portalZdo1:get_zdoid('target')
		local tag1 = portalZdo1:get_string('tag')
        
		-- if target portal assigned
        if target1 ~= ZDOID.NONE then
			local portalZdo2 = ZDOManager:get_zdo(target1)
            --local portal2 = portalZdo2 and Views.Portal.new(portalZdo2) or nil
            
			-- if target is missing from world, reset target
			--if not portalZdo2 or portal2.tag ~= tag1 then
            if not portalZdo2 or portalZdo2:get_string('tag') ~= tag1 then
				portalZdo1.is_local = true

				--portal1.target = ZDOID.NONE
                portalZdo1:set('target', ZDOID.NONE)
				ZDOManager:force_send_zdo(portalZdo1.id);
			end
		else 
			-- find other portalZdos with the same tag
			for i2=i1, #portalZdos do
                if i2 ~= i1 then
                    
                    local portalZdo2 = portalZdos[i2]
                    --local portal2 = Views.Portal.new(portalZdo2)
                    
                    -- connect unlinked portals
                    --if portal2.target == ZDOID.NONE then
                    if portalZdo2:get_zdoid('target') == ZDOID.NONE then
                        --local tag2 = portal2.tag

                        -- link if same tag
                        if tag1 == portalZdo2:get_string('tag') then

                            print("linking portals")
                    
                            portalZdo1.is_local = true
                            portalZdo2.is_local = true
                            --portal1.target = portalZdo2.id
                            --portal2.target = portalZdo1.id
                            portalZdo1:set('target', portalZdo2.id)
                            portalZdo2:set('target', portalZdo1.id)
                            
                            -- might be redundant; TeleportWorld requests ZDO
                            --ZDOManager:ForceSendZDO(portalZdo1.id);
                            --ZDOManager:ForceSendZDO(portalZdo2.id);
                        
                            break -- prevent portals from forming more than 1 link
                        end
                    end
                end
			end
		end
	end
end)

--debug.sethook(function(_, line)
--    print(line)
--end, "l")