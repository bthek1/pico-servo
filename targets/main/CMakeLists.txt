add_executable(main_fw main.c)

target_link_libraries(main_fw
    led
    serial
    servo
)

pico_add_extra_outputs(main_fw)
pico_enable_stdio_usb(main_fw 1)
pico_enable_stdio_uart(main_fw 0)
