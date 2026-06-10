cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_servo C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(lib/led)
add_subdirectory(lib/serial)
add_subdirectory(lib/servo)
add_subdirectory(lib/wifi)
add_subdirectory(targets/main)
add_subdirectory(targets/blink)
add_subdirectory(targets/sweep)
add_subdirectory(targets/webserver)
add_subdirectory(targets/website)
