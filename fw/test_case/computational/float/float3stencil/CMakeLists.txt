cmake_minimum_required(VERSION 3.10)
project(FloatTest CXX)

set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_compile_definitions(KDEBUG=on)

add_executable(float3 float3.cpp)
