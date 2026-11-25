add_library(bic_ntp INTERFACE)
target_sources(bic_ntp INTERFACE ${CMAKE_CURRENT_LIST_DIR}/ntp.c)
target_include_directories(bic_ntp INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(bic_ntp INTERFACE pico_lwip_core)
