cmake_minimum_required(VERSION 3.8.2)
project("roa-discord-rpc" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

file(READ version ROA_DISCORD_RPC_VERSION)
add_compile_definitions("VERSION_STR=\"${ROA_DISCORD_RPC_VERSION}\"")

set(DISCORD_GAME_SDK_ROOT "${CMAKE_SOURCE_DIR}/vendor/discord-game-sdk")

file(GLOB_RECURSE CXX_SOURCE "src/*.cpp")
file(GLOB_RECURSE DISCORD_SDK_SOURCE "${DISCORD_GAME_SDK_ROOT}/cpp/*.cpp")

add_library(${PROJECT_NAME} SHARED ${CXX_SOURCE} ${DISCORD_SDK_SOURCE})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
target_include_directories(${PROJECT_NAME} PRIVATE "include")

# vendors
# roa-mod-loader
add_subdirectory("vendor/roa-mod-loader")
target_link_libraries(${PROJECT_NAME} PRIVATE roa-mod-loader)
target_include_directories(${PROJECT_NAME} PRIVATE "vendor/roa-mod-loader/loader/include")

# discord_game_sdk
set(DISCORD_SDK_BIN_LIB_DIR "${DISCORD_GAME_SDK_ROOT}/lib/x86")
set(DISCORD_SHARED_LIB "${DISCORD_SDK_BIN_LIB_DIR}/discord_game_sdk.dll")
set(DISCORD_LINKER_LIB "${DISCORD_SDK_BIN_LIB_DIR}/discord_game_sdk.dll.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE "${DISCORD_LINKER_LIB}")
target_include_directories(${PROJECT_NAME} PRIVATE "${DISCORD_GAME_SDK_ROOT}/cpp")

# steamworks
set(STEAM_API_LIB "${CMAKE_SOURCE_DIR}/vendor/steamworks_sdk_149/redistributable_bin/steam_api.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE "${STEAM_API_LIB}")
target_include_directories(${PROJECT_NAME} PRIVATE "vendor/steamworks_sdk_149/public/steam")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
    "${CMAKE_SOURCE_DIR}/details.yaml" 
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/info.yaml"
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DISCORD_SHARED_LIB}"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)