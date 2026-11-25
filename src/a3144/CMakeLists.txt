add_library(bic_a3144 INTERFACE)
target_sources(bic_a3144 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/a3144.c)
target_include_directories(bic_a3144 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(bic_a3144 INTERFACE hardware_gpio)
