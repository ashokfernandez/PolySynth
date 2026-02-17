cmake_minimum_required(VERSION 3.15)
project(SEA_Util)

add_library(SEA_Util INTERFACE)

target_include_directories(SEA_Util INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
