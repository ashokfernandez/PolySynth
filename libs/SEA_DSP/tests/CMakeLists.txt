cmake_minimum_required(VERSION 3.15)
project(SEA_DSP_Tests)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories("../include")
include_directories("../../../external/catch2")

# Source files
set(TEST_SOURCES
    main.cpp
    Test_Math.cpp
    Test_Platform.cpp
    Test_Oscillator.cpp
    Test_Filters.cpp
    Test_ADSR.cpp
    Test_LFO.cpp
)

add_executable(sea_tests ${TEST_SOURCES})

# Link against the library
target_link_libraries(sea_tests PRIVATE SEA_DSP)

# Add compilation definitions if needed
if(SEA_PLATFORM_EMBEDDED)
    target_compile_definitions(sea_tests PRIVATE SEA_PLATFORM_EMBEDDED)
endif()
