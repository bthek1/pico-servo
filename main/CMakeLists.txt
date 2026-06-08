add_executable(pico_servo main.c)

target_link_libraries(pico_servo
    pico_stdlib
)

pico_add_extra_outputs(pico_servo)
pico_enable_stdio_usb(pico_servo 1)
pico_enable_stdio_uart(pico_servo 0)
