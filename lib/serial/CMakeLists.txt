add_library(serial INTERFACE)

target_sources(serial INTERFACE serial.c)
target_include_directories(serial INTERFACE .)
target_link_libraries(serial INTERFACE pico_stdlib)
