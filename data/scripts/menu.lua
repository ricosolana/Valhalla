function Test()
	print('Hello world!')
end

function Connect(doc)
	-- API call
	local addr = doc:GetElementById('hostname-in'):GetAttribute('value')

	local host = nil
	local port = ""

	i = string.find(addr, ":")

	if i ~= nil and i < #addr then
	  -- sub appears to be inclusive on both bounds
	  host = string.sub(addr, 1, i-1)
	  port = string.sub(addr, i + 1)
	else
	  host = addr
	end

	Valhalla.Connect(
		host, port
	)
end

function Disconnect(doc)
	Valhalla.Disconnect()
end

-- login with password
function SendPeerInfo(doc)	
	print("login")
	
	local connectingDoc = {}
    for i,d in ipairs(rmlui.contexts["default"].documents) do
	    if d.title == "ConnectingMenu" then
			connectingDoc = d
		end
    end
	
	doc:Hide()
	connectingDoc:Show()
	
	Valhalla.SendPeerInfo(
		doc:GetElementById('password-in'):GetAttribute('value')
	)
	
end
