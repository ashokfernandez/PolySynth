cmake_minimum_required(VERSION 3.15)
project(SEATest)

# Add the library
add_subdirectory(../../libs/SEA_DSP SEA_DSP)

# Executable
add_executable(sea_test main.cpp)

# Link
target_link_libraries(sea_test PRIVATE SEA_DSP)

# Standard C++
set_target_properties(sea_test PROPERTIES CXX_STANDARD 17)
