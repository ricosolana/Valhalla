modInfo = {
	name        		= "My example mod",
	version     		= "1.0",
	api_version 		= 1, 
	description 		= "This is a test mod to demonstrate the API",
	authors     		= { "crazicrafter1" },
	website 			= "www.github.com/PeriodicSeizures/Valhalla/",
	--depend						= {  }
	onEnable = function() 
		print("example print from mod woo " .. Valhalla.Version)
	end
}