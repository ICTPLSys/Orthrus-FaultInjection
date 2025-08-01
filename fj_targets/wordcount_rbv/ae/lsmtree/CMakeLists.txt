add_executable(lsmtree_fj lsmtree_fj.cpp)
target_link_libraries(lsmtree_fj PRIVATE lsmtree ${LIBS})

add_subdirectory(throughput)
add_subdirectory(latency)
