function Test()
	print('Hello world!')
end

function ServerStatus(doc)
	-- API call
	Alchyme.ServerStatus(
		doc:GetElementById('hostname-in'):GetAttribute('value')
	)
end

function ServerJoin(doc)
	-- API call
	Alchyme.ServerJoin(
		doc:GetElementById('hostname-in'):GetAttribute('value')
	)
end

function ServerDisconnect(doc)
	Alchyme.DisconnectFromServer()
end

function ServerLogin(doc)	
	print("Lua ForwardPeerInfo")
	
	local connectingDoc = {}
    for i,d in ipairs(rmlui.contexts["default"].documents) do
	    if d.title == "ConnectingMenu" then
			connectingDoc = d
		end
    end
	
	doc:Hide()
	connectingDoc:Show()
	
	Alchyme.SendLogin(
		doc:GetElementById('username-in'):GetAttribute('value'),
		doc:GetElementById('login-key-in'):GetAttribute('value')
	)
	
end
