add_executable(webserver webserver.c)

target_include_directories(webserver PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
)

pico_add_library(webserver_content NOFLAG)
pico_set_lwip_httpd_content(webserver_content INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/content/index.html
    ${CMAKE_CURRENT_LIST_DIR}/content/ok.txt
)

target_link_libraries(webserver
    wifi
    serial
    pico_lwip_http
    webserver_content
)

pico_add_extra_outputs(webserver)
pico_enable_stdio_usb(webserver 1)
pico_enable_stdio_uart(webserver 0)
