cmake_minimum_required(VERSION 3.4...3.18)
project(como_quickjs)

set(OBJ_DIR $ENV{OUT_PATH})
set(BIN_DIR $ENV{BIN_PATH})
set(INC_DIR ${BIN_DIR}/inc)

find_library(LIB_comort comort.so "${BIN_DIR}")
message(STATUS "LIB_comort = ${LIB_comort}")

set(quickjs_version 2020-01-19)
set(quickjs_sources_root .)
set(quickjs_sources
    ${quickjs_sources_root}/quickjs.h
    ${quickjs_sources_root}/quickjs-libc.h
    ${quickjs_sources_root}/quickjs.c
    ${quickjs_sources_root}/libregexp.c
    ${quickjs_sources_root}/libunicode.c
    ${quickjs_sources_root}/libbf.c
    ${quickjs_sources_root}/cutils.c
    ${quickjs_sources_root}/quickjs-libc.c
)

set(get_quickjs_cmake_in ${CMAKE_CURRENT_LIST_DIR}/get_quickjs.cmake.in)
set(get_quickjs_cmake ${CMAKE_CURRENT_BINARY_DIR}/get_quickjs.cmake)
configure_file(
    ${get_quickjs_cmake_in}
    ${get_quickjs_cmake}
    @ONLY
)

include_directories(
    ./
    ${INC_DIR})

add_library(${PROJECT_NAME}
    ${quickjs_sources_root}/src/como_bridge.cpp
    ${quickjs_sources_root}/src/utils.cpp
    ${LIB_comort}
    ${quickjs_sources}
)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CONFIG_VERSION="${quickjs_version}"
)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${quickjs_sources_root}
)

target_link_libraries(como_quickjs PRIVATE
    ${LIB_comort})
