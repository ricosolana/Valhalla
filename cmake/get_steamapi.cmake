set(STEAMAPI_ZIP_LOCATION "${CMAKE_BINARY_DIR}/cmake/steamworks_sdk.zip")
set(STEAMAPI_LOCATION "${CMAKE_BINARY_DIR}/cmake/steamworks_sdk")
set(STEAMAPI_BINARY_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.lib")
set(STEAMAPI_SOURCE_DIR "${STEAMAPI_LOCATION}/sdk/public/steam")
set(STEAMAPI_SHARED_DIR "${STEAMAPI_LOCATION}/sdk/redistributable_bin/win64/steam_api64.dll")

#set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${STEAMAPI_ZIP_LOCATION}))
  message(STATUS "Downloading Steam SDK to ${STEAMAPI_ZIP_LOCATION}")
  file(DOWNLOAD
       https://partner.steamgames.com/downloads/steamworks_sdk.zip
       ${STEAMAPI_ZIP_LOCATION}
  )
endif()

# allow for zip to finalize?
#ctest_sleep(3)

if(NOT(EXISTS ${STEAMAPI_LOCATION}))
    message(STATUS "Unzipping Steam SDK to ${STEAMAPI_LOCATION}")
    file(ARCHIVE_EXTRACT INPUT ${STEAMAPI_ZIP_LOCATION} DESTINATION ${STEAMAPI_LOCATION})
endif()

#include(${STEAMAPI_ZIP_LOCATION})