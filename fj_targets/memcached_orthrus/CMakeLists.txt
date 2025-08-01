cmake_minimum_required(VERSION 3.20)
project(SCEE_Demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wno-unused-variable -Wno-unused-but-set-variable -O3)

if(NOT CMAKE_BUILD_TYPE)
    message("Build in Release Mode")
    set(CMAKE_BUILD_TYPE Release)
else()
    message("Build in ${CMAKE_BUILD_TYPE}")
endif()

find_package(Threads REQUIRED)
# https://github.com/microsoft/mimalloc
find_package(mimalloc REQUIRED)

add_subdirectory(third/isal-crc)

include_directories(include)
include_directories(third/isal-crc/include)

add_library(scee scee.cpp)
target_link_libraries(scee PRIVATE isal-crc mimalloc-static)

set(LIBS scee Threads::Threads isal-crc mimalloc-static)

add_subdirectory(examples)
add_subdirectory(benchmarks)
