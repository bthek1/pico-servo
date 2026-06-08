add_executable(sweep main.c)

target_link_libraries(sweep
    servo
)

pico_add_extra_outputs(sweep)
pico_enable_stdio_usb(sweep 1)
pico_enable_stdio_uart(sweep 0)
