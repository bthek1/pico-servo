add_executable(main main.c)

target_link_libraries(main
    led
    serial
    servo
)

pico_add_extra_outputs(main)
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)
