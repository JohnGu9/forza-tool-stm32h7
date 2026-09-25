cmake_minimum_required(VERSION 3.10)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    project(
        mf
        LANGUAGES C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS ON)
endif()

set(mf_src ${CMAKE_CURRENT_LIST_DIR}/mf.c)
add_library(mf OBJECT ${mf_src})
target_include_directories(mf PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    add_executable(mf_test ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(mf_test PRIVATE mf)
endif()
