cmake_minimum_required(VERSION 3.10)
project(SCEE)

option(ENABLE_LSMTREE "Build LSMTree" OFF)
option(ENABLE_FJ "Only Fault Injection (WIP)" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-unused-function
)

message("Build in ${CMAKE_BUILD_TYPE}")

# set(SANITIZE_FLAG "-fsanitize=address -DMI_TRACK_ASAN=1 -g")
# add_compile_definitions(MI_TRACK_ASAN=1)

set(CMAKE_C_FLAGS_RELEASE "-O3 ${SANITIZE_FLAG} -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${SANITIZE_FLAG} -pthread")
set(CMAKE_LD_FLAGS_RELEASE "${SANITIZE_FLAG} -pthread")

# add_compile_options(-g -Wno-format)
include_directories(include)

find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# https://github.com/microsoft/mimalloc
find_package(mimalloc REQUIRED)
# add_library(mimalloc-static INTERFACE)

# https://github.com/HdrHistogram/HdrHistogram_c.git
find_package(hdr_histogram REQUIRED)

find_package(Boost REQUIRED COMPONENTS program_options)

add_subdirectory(third/isal-crc)
include_directories(third/isal-crc/include)

add_subdirectory(third/HdrHistogram_c)

add_library(profile profile.cpp)
target_compile_definitions(profile PRIVATE ENABLE_PROFILE=true)
target_link_libraries(profile PRIVATE hdr_histogram::hdr_histogram_static)

add_library(profile-disable profile.cpp)
target_compile_definitions(profile-disable PRIVATE ENABLE_PROFILE=false)
target_link_libraries(profile-disable PRIVATE hdr_histogram::hdr_histogram_static)

add_library(profile_mem profile-mem.cpp)
target_compile_definitions(profile_mem PRIVATE ENABLE_PROFILE=true)

set(LIBS_deps Threads::Threads isal-crc mimalloc-static)

# SCEE lib
set(TARGET scee)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE  mimalloc-static)
set(LIBS ${TARGET} ${LIBS_deps} profile-disable)

# SCEE lib, but enable profile
set(TARGET scee_profile)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE  mimalloc-static profile)
set(LIBS_profile ${TARGET} ${LIBS_deps} profile)

# SCEE lib disable check
set(TARGET scee_nocheck)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE  mimalloc-static profile-disable)
target_compile_definitions(${TARGET} PUBLIC DISABLE_VALIDATION)
set(LIBS_nocheck ${TARGET} ${LIBS_deps})

# SCEE lib disable scee
set(TARGET scee_disabled)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE  mimalloc-static profile-disable)
target_compile_definitions(${TARGET} PRIVATE DISABLE_SCEE)
set(LIBS_disabled scee_disabled Threads::Threads  mimalloc-static profile-disable)

set(TARGET scee_disabled_profile)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE  mimalloc-static profile)
target_compile_definitions(${TARGET} PRIVATE DISABLE_SCEE)
set(LIBS_disabled_profile scee_disabled_profile Threads::Threads  mimalloc-static profile)

set(TARGET scee_sampling)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE mimalloc-static profile-disable)
target_compile_definitions(${TARGET} PRIVATE SAMPLING)
set(LIBS_sampling scee_sampling Threads::Threads  mimalloc-static profile-disable)

set(TARGET scee_sampling_profile)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE mimalloc-static profile)
target_compile_definitions(${TARGET} PRIVATE SAMPLING)
set(LIBS_sampling_profile scee_sampling_profile Threads::Threads  mimalloc-static profile)

add_subdirectory(ae)
# add_subdirectory(examples)
# add_subdirectory(benchmarks)

# if (ENABLE_LSMTREE)
#     message("build lsmtree")

#     find_package(PkgConfig REQUIRED)

#     set(GFLAGS_USE_TARGET_NAMESPACE ON)
#     find_package(gflags CONFIG REQUIRED)

#     add_subdirectory(lib/lsmtree)
#     add_subdirectory(lib/lsmtree.raw)
#     add_subdirectory(ae/lsmtree)
# endif()
