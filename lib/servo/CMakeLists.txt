add_library(servo INTERFACE)

target_sources(servo INTERFACE servo.c)
target_include_directories(servo INTERFACE .)
target_link_libraries(servo INTERFACE
    pico_stdlib
    hardware_pwm
    hardware_clocks
)
