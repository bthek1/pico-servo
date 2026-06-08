add_executable(main main.c)

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(main
    wifi
    serial
    servo
)

pico_add_extra_outputs(main)
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)
