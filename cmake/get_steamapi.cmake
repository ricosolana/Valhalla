#set(STEAMAPI_ZIP_LOCATION "${CMAKE_BINARY_DIR}/cmake/steamworks_sdk.zip")
#set(STEAMAPI_LOCATION "${CMAKE_BINARY_DIR}/cmake/steamworks_sdk")
#set(STEAMAPI_BINARY_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.lib")
#set(STEAMAPI_SOURCE_DIR "${STEAMAPI_LOCATION}/sdk/public/steam")
#set(STEAMAPI_SHARED_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.dll")

set(STEAMAPI_LOCATION "/home/pi/steam-api")
set(STEAMAPI_BINARY_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.lib")
set(STEAMAPI_SOURCE_DIR "${STEAMAPI_LOCATION}/sdk/public/steam")
set(STEAMAPI_SHARED_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.dll")

#else ()
#	set(STEAMAPI_LOCATION "C:/Users/Rico/Documents/Visual Studio 2022/Libraries/steam-sdk")
#	set(STEAMAPI_BINARY_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.lib")
#	set(STEAMAPI_SOURCE_DIR "${STEAMAPI_LOCATION}/sdk/public/steam")
#	set(STEAMAPI_SHARED_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.dll")
#endif ()

#if(NOT (EXISTS ${STEAMAPI_ZIP_LOCATION}))
#  message(STATUS "Downloading Steam SDK to ${STEAMAPI_ZIP_LOCATION}")
#  file(DOWNLOAD
#       https://partner.steamgames.com/downloads/steamworks_sdk.zip
#       ${STEAMAPI_ZIP_LOCATION}
#  )
#endif()
#
#if(NOT(EXISTS ${STEAMAPI_LOCATION}))
#    message(STATUS "Unzipping Steam SDK to ${STEAMAPI_LOCATION}")
#    file(ARCHIVE_EXTRACT INPUT ${STEAMAPI_ZIP_LOCATION} DESTINATION ${STEAMAPI_LOCATION})
#endif()
