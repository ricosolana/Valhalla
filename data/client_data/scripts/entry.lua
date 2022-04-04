--print("Hello, Lua!")
--print(rmlui ~= nil and "not nil" or "is nil")

local mainScript = {
	onEnable = nil,
	onDisable = nil,
	onUpdate = nil,
	onLogin = nil
}

function mainScript.onEnable() 
	print("Main script successfully enabled!")
		
	rmlui:LoadFontFace("fonts/OpenSans-Regular.ttf")
	rmlui:LoadFontFace("fonts/Norse.otf")
	
	local con = rmlui.contexts["default"]
	
	local uiMainMenu = con:LoadDocument("ui/main-menu.rml")
	con:LoadDocument("ui/password-menu.rml")
	con:LoadDocument("ui/connecting-menu.rml")
	
    uiMainMenu:Show()
end

function mainScript.onLogin() 
	print("Lua Login called!")
	
	local mainMenuDoc = {}
    local passwordDoc = {}
    for i,d in ipairs(rmlui.contexts["default"].documents) do
	    if d.title == "MainMenu" then
			mainMenuDoc = d
        elseif d.title == "PasswordMenu" then
			passwordDoc = d
		end
    end
	
	print("Lua ok here!")
	
    passwordDoc:Show()
	mainMenuDoc:Hide()
	
	print("Lua ok here after!")
	
end

-- Register this for routine/scheduled event
Alchyme.RegisterScript(mainScript)

mainScript = nil