--[[
    Implementation of the compress mod originally used on the Comfy Valheim server (2/9/2023)
    https://github.com/redseiko/ComfyMods/blob/main/Compress/Compress.cs

    Ported by crzi for use on the C++ Valhalla server
--]]

local peers = {}

local portalHash = VUtils.String.GetStableHashCode("portal_wood")

Valhalla.OnEvent("FixedUpdate", function()
    print("Linking portals")
    
	local portals = ZDOManager.GetZDOs(portalHash)

	for k=1, #portals do
		local zdo = portals[k]
		local target = zdo:GetZDOID("target")
		local tag = zdo:GetString("tag", "")

		if target != nil then
			local zdo2 = ZDOManager.GetZDO(target)
			if zdo2 ~= nil or zdo2:GetString("tag", "") ~= tag then
				zdo:SetLocal()
				
				
				
				-- todo impl:



				zdo:Set("target", ZDOID.None);
				ZDOMan.instance.ForceSendZDO(zdo.m_uid);
			end
		}
	}
	foreach (ZDO zdo3 in this.m_tempPortalList)
	{
		string string2 = zdo3.GetString("tag", "");
		if (zdo3.GetZDOID("target").IsNone())
		{
			ZDO zdo4 = this.FindRandomUnconnectedPortal(this.m_tempPortalList, zdo3, string2);
			if (zdo4 != null)
			{
				zdo3.SetOwner(ZDOMan.instance.GetMyID());
				zdo4.SetOwner(ZDOMan.instance.GetMyID());
				zdo3.Set("target", zdo4.m_uid);
				zdo4.Set("target", zdo3.m_uid);
				ZDOMan.instance.ForceSendZDO(zdo3.m_uid);
				ZDOMan.instance.ForceSendZDO(zdo4.m_uid);
			}
		}
	}
	yield return new WaitForSeconds(5f);
}
	yield break;
}
