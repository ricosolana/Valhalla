#option (AVL_ENABLE_SCRIPTING "Enable Lua modding subsystem" OFF)
#option (AVL_USE_PREFABS "Enable Prefab support" ON)
#option (AVL_ZONE_GENERATION "Generate zones/features/vegetation" ON) # requires AVL_USE_PREFABS default or ON
option (AVL_DISCORD_INTEGRATION "Enable Discord command bot" ON)

option (AVL_LEGACY_WORLD_LOADING "Enable very old world loading" OFF)

# Download it here https://partner.steamgames.com/downloads/steamworks_sdk.zip
#set	(STEAMWORKS_SDK_LOCATION "C:/Users/rico/Documents/Visual Studio 2022/Libraries/steam-sdk")
#set (STEAMWORKS_SDK_LOCATION "$ENV{HOME}/.local/bin/steamsdk")
