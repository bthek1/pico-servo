add_library(wifi INTERFACE)

target_sources(wifi INTERFACE wifi.c)
target_include_directories(wifi INTERFACE .)
target_link_libraries(wifi INTERFACE
    pico_stdlib
    pico_cyw43_arch_lwip_poll
)
