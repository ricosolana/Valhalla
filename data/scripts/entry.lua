--print("Hello, Lua!")
--print(rmlui ~= nil and "not nil" or "is nil")

local mainScript = {
	onEnable = nil,
	onDisable = nil,
	onUpdate = nil,
	onPreLogin = nil
}

function mainScript.onEnable() 
	print("Main script enabled!")
end

function mainScript.onPreLogin() 
	print("Lua Login called!")	
end

print("about to register")

-- Register this for routine/scheduled event
Valhalla.RegisterScript(mainScript)

mainScript = nil