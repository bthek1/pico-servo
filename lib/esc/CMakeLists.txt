add_library(esc INTERFACE)
target_sources(esc INTERFACE esc.c)
target_include_directories(esc INTERFACE .)
target_link_libraries(esc INTERFACE pico_stdlib hardware_pwm hardware_clocks)
