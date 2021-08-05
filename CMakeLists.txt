cmake_minimum_required(VERSION 3.4...3.18)
project(como_quickjs)

set(OBJ_DIR $ENV{OUT_PATH})
set(BIN_DIR $ENV{BIN_PATH})
set(INC_DIR ${BIN_DIR}/inc)

find_library(LIB_comort comort.so "${BIN_DIR}")
message(STATUS "LIB_comort = ${LIB_comort}")

include_directories(
    ./
    ${INC_DIR})

add_library(como_quickjs MODULE
    src/como_bridge.cpp
    src/utils.cpp
    ${LIB_comort})

target_link_libraries(como_quickjs PRIVATE
    ${LIB_comort})
