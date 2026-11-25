add_library(bic_drv8833 INTERFACE)
target_sources(bic_drv8833 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/drv8833.c)
target_include_directories(bic_drv8833 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(bic_drv8833 INTERFACE hardware_pwm hardware_gpio hardware_clocks pico_time)
