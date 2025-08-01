add_executable(lsmtree_throughput_raw lsmtree_throughput_raw.cpp)
target_link_libraries(lsmtree_throughput_raw PRIVATE
    rawlsmtree
    gflags::gflags
    ${LIBS_deps}
    profile_mem
)

add_executable(lsmtree_throughput_scee lsmtree_throughput_scee.cpp)
target_link_libraries(lsmtree_throughput_scee PRIVATE
    lsmtree
    gflags::gflags
    ${LIBS_deps}
    profile_mem
)

add_executable(lsmtree_throughput_rbv lsmtree_throughput_rbv.cpp)
target_link_libraries(lsmtree_throughput_rbv PRIVATE
    rawlsmtree
    gflags::gflags
    ${LIBS_deps}
    profile_mem
)
