add_library(steamylib STATIC)
target_include_directories(steamylib PUBLIC ${STEAMWORKS_SDK_SOURCE_DIR})
#target_compile_options(steamylib PUBLIC --bad-option)

${STEAMWORKS_SDK_SHARED_DIR}