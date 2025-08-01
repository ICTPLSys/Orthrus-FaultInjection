add_executable(memtest memory-test.cpp)
target_link_libraries(memtest PRIVATE mimalloc-static profile_mem Threads::Threads)

add_executable(memtest_raw memory-test.cpp)
target_compile_definitions(memtest_raw PRIVATE NOMIMALLOC)
target_link_libraries(memtest_raw PRIVATE profile_mem Threads::Threads)
