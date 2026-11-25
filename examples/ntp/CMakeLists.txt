add_executable(bic_ntp_example ntp_client.c)
target_compile_definitions(bic_ntp_example PRIVATE
    WIFI_SSID="${WIFI_SSID}"
    WIFI_PASSWORD="${WIFI_PASSWORD}"
)
target_include_directories(bic_ntp_example PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(bic_ntp_example PRIVATE bic_ntp pico_stdlib pico_cyw43_arch_lwip_poll)
pico_enable_stdio_usb(bic_ntp_example 1)
pico_enable_stdio_uart(bic_ntp_example 0)
pico_add_extra_outputs(bic_ntp_example)
