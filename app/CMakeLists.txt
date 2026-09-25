cmake_minimum_required(VERSION 3.22)

add_subdirectory(st7735)

add_subdirectory(lvgl)

add_subdirectory(tinyusb/src)
tinyusb_target_add(${CMAKE_PROJECT_NAME})
aux_source_directory(tinyusb/src/portable/synopsys/dwc2 tinyusb_portable_src)
aux_source_directory(tinyusb/lib/networking tinyusb_networking_src)

set(LWIP_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/lwip/src/include
    ${CMAKE_CURRENT_LIST_DIR}/tinyusb/src
    ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(lwip)

add_subdirectory(mf)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${LWIP_INCLUDE_DIRS}
)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} app_src)
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/widget widget_src)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${app_src}
    ${widget_src}
    ${tinyusb_portable_src}
    ${tinyusb_networking_src}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    mf
    st7735
    lvgl
    lwipcore
    lwipallapps
    lwipmbedtls
)
