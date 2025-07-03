target_sources(avledet_scripting_test
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSocket.h

  # Source files
  PRIVATE
    #${CMAKE_CURRENT_SOURCE_DIR}/SolPreventGlobal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NetworkTest.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/SolSingle.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/SolTest.cpp
)

