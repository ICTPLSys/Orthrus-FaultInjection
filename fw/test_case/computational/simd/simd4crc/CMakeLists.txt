cmake_minimum_required(VERSION 3.10)
project(SIMD4CRC CXX)

set(CMAKE_CXX_COMPILER clang++-16)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -mavx -mfma")

add_compile_definitions(KDEBUG=on)

add_executable(simd4 simd4.cpp)
