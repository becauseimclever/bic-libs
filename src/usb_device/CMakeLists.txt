add_library(bic_usb_device INTERFACE)
target_sources(bic_usb_device INTERFACE ${CMAKE_CURRENT_LIST_DIR}/usb_device.c)
target_include_directories(bic_usb_device INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

foreach(profile IN ITEMS xinput dinput keyboard mouse)
    add_library(bic_usb_${profile} INTERFACE)
    target_sources(bic_usb_${profile} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/${profile}.c)
    target_link_libraries(bic_usb_${profile} INTERFACE bic_usb_device)
endforeach()
