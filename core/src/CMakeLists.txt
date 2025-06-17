#set(AVLEDET_SERVER_INCLUDE_DIR ${AVLEDET_SERVER_DIR}/include)
target_sources(avledet_server
  # Source files
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp

  # Header files
  #PUBLIC
    #${AVLEDET_FRONTEND_API_INCLUDE_DIR}/Frontend.h
)

