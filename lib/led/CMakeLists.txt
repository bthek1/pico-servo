add_library(led INTERFACE)

target_sources(led INTERFACE led.c)
target_include_directories(led INTERFACE .)
target_link_libraries(led INTERFACE
    pico_stdlib
    pico_cyw43_arch_none
)
