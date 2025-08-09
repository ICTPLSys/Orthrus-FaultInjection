cmake_minimum_required(VERSION 3.20)

# wget https://apt.llvm.org/llvm.sh
# chmod +x llvm.sh
# sudo ./llvm.sh 16
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(SCEE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")

if(NOT CMAKE_BUILD_TYPE)
    message("Build in Release Mode")
    set(CMAKE_BUILD_TYPE Release)
else()
    message("Build in ${CMAKE_BUILD_TYPE}")
endif()

find_package(Threads REQUIRED)
# https://github.com/microsoft/mimalloc
# find_package(mimalloc REQUIRED)
add_subdirectory(third/mimalloc)
find_package(Boost REQUIRED COMPONENTS program_options)

include_directories(include)

add_subdirectory(third/isal-crc)
include_directories(third/isal-crc/include)

add_subdirectory(third/HdrHistogram_c)
include_directories(third/HdrHistogram_c/include)

add_library(scee scee.cpp)
target_link_libraries(scee PRIVATE isal-crc mimalloc Boost::program_options)

set(LIBS scee Threads::Threads isal-crc mimalloc Boost::program_options)

add_subdirectory(examples)
add_subdirectory(benchmarks)
