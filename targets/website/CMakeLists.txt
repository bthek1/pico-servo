add_executable(website website.c)

target_include_directories(website PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
)

pico_add_library(website_content NOFLAG)
pico_set_lwip_httpd_content(website_content INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/content/index.html
    ${CMAKE_CURRENT_LIST_DIR}/content/logo.svg
    ${CMAKE_CURRENT_LIST_DIR}/content/ok.txt
    ${CMAKE_CURRENT_LIST_DIR}/content/empty.txt
)

target_link_libraries(website
    wifi
    serial
    servo
    pico_lwip_http
    website_content
)

pico_add_extra_outputs(website)
pico_enable_stdio_usb(website 1)
pico_enable_stdio_uart(website 0)
