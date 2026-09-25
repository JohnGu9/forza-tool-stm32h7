cmake_minimum_required(VERSION 3.12)

set(CMAKE_LIBRARY_NAME st7735)

option(ST7735_WITH_FONTS "" ON)

set(ST7735_SOURCE
    ${CMAKE_CURRENT_LIST_DIR}/st7735.c
)

if(ST7735_WITH_FONTS)
    set(ST7735_SOURCE
        ${ST7735_SOURCE}
        ${CMAKE_CURRENT_LIST_DIR}/fonts.c
    )
endif()

add_library(${CMAKE_LIBRARY_NAME} OBJECT ${ST7735_SOURCE})

if(ST7735_WITH_FONTS)
    target_compile_definitions(${CMAKE_LIBRARY_NAME} INTERFACE ST7735_WITH_FONTS=1)
endif()

target_include_directories(${CMAKE_LIBRARY_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
