cmake_minimum_required(VERSION 3.13)

if (NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if (NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to a Pico SDK checkout.")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(bic_libs C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

include(bic_libs_import.cmake)

option(BIC_LIBS_BUILD_EXAMPLES "Build the library examples" OFF)
if (BIC_LIBS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
