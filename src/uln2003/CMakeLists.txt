add_library(bic_uln2003 INTERFACE)
target_sources(bic_uln2003 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/uln2003.c)
target_include_directories(bic_uln2003 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(bic_uln2003 INTERFACE hardware_gpio hardware_timer)
