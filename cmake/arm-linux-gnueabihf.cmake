# CMake toolchain file for ARM Linux hard-float cross-compilation.
# Used to cross-compile embedded demo targets with real ARM single-precision
# float arithmetic, then run under QEMU user-mode (qemu-arm-static).
#
# Usage:
#   cmake -S tests -B tests/build_arm \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake \
#     -G Ninja
#
# Requires: apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
