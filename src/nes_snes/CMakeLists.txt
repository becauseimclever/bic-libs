add_library(bic_nes_snes INTERFACE)
target_sources(bic_nes_snes INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/reader.c
    ${CMAKE_CURRENT_LIST_DIR}/emulator.c)
target_include_directories(bic_nes_snes INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(bic_nes_snes INTERFACE hardware_gpio hardware_pio hardware_timer)
pico_generate_pio_header(bic_nes_snes ${CMAKE_CURRENT_LIST_DIR}/nes_snes_output.pio)
