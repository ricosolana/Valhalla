if (NOT DEFINED STEAMWORKS_SDK_LOCATION)
	message(FATAL_ERROR "STEAMWORKS_SDK_LOCATION was not found, set the path to SteamworksSDK in cmake/VHSettings.cmake")
endif()

set(STEAMWORKS_SDK_BINARY_DIR "${STEAMWORKS_SDK_LOCATION}/sdk/redistributable_bin/win64/steam_api64.lib")
set(STEAMWORKS_SDK_SOURCE_DIR "${STEAMWORKS_SDK_LOCATION}/sdk/public/steam")
set(STEAMWORKS_SDK_SHARED_DIR "${STEAMWORKS_SDK_LOCATION}/sdk/redistributable_bin/win64/steam_api64.dll")
