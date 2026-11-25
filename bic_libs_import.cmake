# Include after pico_sdk_init() to make the libraries available to an application.
if (NOT TARGET bic_ntp)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/bic_libs")
endif()
