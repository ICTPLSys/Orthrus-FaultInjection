cmake_minimum_required(VERSION 3.10)
project(SCEE_Demo)

option(ENABLE_LSMTREE "Build LSMTree" OFF)

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

# find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# https://github.com/microsoft/mimalloc
# find_package(mimalloc REQUIRED)
add_subdirectory(third/mimalloc)
# add_library(mimalloc-static INTERFACE)

# git@github.com:HdrHistogram/HdrHistogram_c.git
# find_package(hdr_histogram REQUIRED)
add_subdirectory(third/HdrHistogram_c)
include_directories(third/HdrHistogram_c/include)

# add_subdirectory(third/isal-crc)
# include_directories(third/isal-crc/include)

link_directories(/usr/local/lib)
include_directories(include)

# add_subdirectory(third/HdrHistogram_c)

# add_library(profile profile.cpp)
# target_compile_definitions(profile PRIVATE ENABLE_PROFILE=true)
# target_link_libraries(profile PRIVATE hdr_histogram::hdr_histogram_static)

add_library(profile-disable profile.cpp)
target_compile_definitions(profile-disable PRIVATE ENABLE_PROFILE=false)
target_link_libraries(profile-disable PRIVATE hdr_histogram_static)

# add_library(profile_mem profile-mem.cpp)
# target_compile_definitions(profile_mem PRIVATE ENABLE_PROFILE=true)

set(LIBS_deps Threads::Threads mimalloc-static pthread)

# SCEE lib
set(TARGET scee)
add_library(${TARGET} scee.cpp)
target_link_libraries(${TARGET} PRIVATE mimalloc-static pthread)
set(LIBS ${TARGET} ${LIBS_deps} profile-disable)

# SCEE lib, but enable profile
# set(TARGET scee_profile)
# add_library(${TARGET} scee.cpp)
# target_link_libraries(${TARGET} PRIVATE isal-crc mimalloc-static profile)
# set(LIBS_profile ${TARGET} ${LIBS_deps} profile)

# SCEE lib disable check
# set(TARGET scee_nocheck)
# add_library(${TARGET} scee.cpp)
# target_link_libraries(${TARGET} PRIVATE isal-crc mimalloc-static profile-disable)
# target_compile_definitions(${TARGET} PUBLIC DISABLE_VALIDATION)
# set(LIBS_nocheck ${TARGET} ${LIBS_deps})

# add_subdirectory(examples)
# add_subdirectory(benchmarks)

if (ENABLE_LSMTREE)
    message("build lsmtree")

    find_package(PkgConfig REQUIRED)

    # set(GFLAGS_USE_TARGET_NAMESPACE ON)
    # find_package(gflags CONFIG REQUIRED)

    add_subdirectory(lib/lsmtree)
    add_subdirectory(lib/lsmtree.raw)
    add_subdirectory(tests/lsmtree)

    # add_subdirectory(tests/mem)
endif()
